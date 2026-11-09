/*
Copyright (c) 1997-2026, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#include "../../../planarityApp/planarity.h"

// NOTE: Even though the G6ReadIterator is available through the include of
// planarity.h via its include of graphLib.h, we may choose to make it private.
#include "../../../graphLib/io/d6-read-iterator.h"
#include "../../../graphLib/io/g6-read-iterator.h"
#include "../../../graphLib/io/s6-read-iterator.h"

/* Imported functions */
extern char *copySparse6TestString(char const *s6Str);

/* Private function declarations (exported within system) */
int runFileIteratorsTests(void);

/* Private functions */
int runGeneralReadIteratorTests(void);

int runFileIteratorsTests(void)
{
    int Result = OK;

    gp_Message("Starting File Iterator Tests");

    if (runGeneralReadIteratorTests() != OK)
        Result = NOTOK;

    if (Result == OK)
        gp_Message("Successfully finished File Iterator Tests.\n");
    else
        gp_Message("Error detected in File Iterator Tests.\n");

    return Result;
}

int runGeneralReadIteratorTests(void)
{
    int Result = OK;
    unsigned origQuietMode = gp_GetQuietMode();
    size_t i = 0;

    // {input string, encoding of the first graph it contains}, in graph6
    // except for the digraph6 cases, whose graphs graph6 cannot hold
    char const *acceptCases[][2] = {
        // graph6, with and without its header
        {"D?{\n", "D?{"},
        {">>graph6<<D?{\n", "D?{"},
        // The order-1 graph has an empty adjacency matrix, with or without a
        // final line terminator
        {"@\n", "@"},
        {"@", "@"},
        // sparse6 and incremental sparse6, the latter with the whole graph
        // that its second line modifies
        {":Fa@x^\n", "Fw??G"},
        {":D\n;oN\n", "D??"},
        {">>sparse6<<:Fa@x^\n", "Fw??G"},
        // digraph6, with and without its header: the arc 1 -> 0, then on the
        // second line the arc 0 -> 1
        {"&AG\n&AO\n", "&AG"},
        {">>digraph6<<&AG\n", "&AG"},
    };

    // Inputs in none of the formats the iterator reads
    char const *rejectCases[] = {
        // A '&' without the order that digraph6 puts after it
        "&\n",
        // GraphML, whose first character is below the graph6 range
        "<graphml>\n",
        // An adjacency list, which gp_Read() recognizes by its "N=" but a
        // file of graphs cannot contain: its first byte is in the graph6
        // range and its second is not
        "N=3\n1: 2 3 0\n2: 1 0\n3: 1 0\n",
    };

    // graph6 marks its lines with nothing but the order, so a first line that
    // begins like graph6 is handed to the graph6 reader, which then fails on
    // the content of the line: here the order says 5 vertices but the line
    // ends before their edges do.
    char const *readFailCases[] = {
        "D?\n",
        // The order byte alone is a complete line only when its graph has no
        // adjacency-matrix bytes
        "D",
    };

    char const *g6Header = ">>graph6<<";
    char const *d6Header = ">>digraph6<<";
    graphP theGraph = NULL;
    GPReadIteratorP theReadIterator = NULL;
    char *actualEncoding = NULL;

    // {first line, expected s6_IsSparse6Input(), expected g6_IsGraph6Input(),
    // expected d6_IsDigraph6Input()}
    struct
    {
        char const *firstLine;
        int isSparse6;
        int isGraph6;
        int isDigraph6;
    } predicateCases[] = {
        {">>sparse6<<:Fa@x^", TRUE, FALSE, FALSE},
        {":Fa@x^", TRUE, FALSE, FALSE},
        {";oN", TRUE, FALSE, FALSE},
        {">>graph6<<D?{", FALSE, TRUE, FALSE},
        {"D?{", FALSE, TRUE, FALSE},
        // The order byte spans 63 to 126, and 126 introduces the longer
        // order encodings; a graph of order 0 or 1 is its order byte alone,
        // with or without the line terminator sf_fgets() leaves on it
        {"?", FALSE, TRUE, FALSE},
        {"@\n", FALSE, TRUE, FALSE},
        {"~~~~", FALSE, TRUE, FALSE},
        // digraph6, with and without its header, its order byte at either end
        // of the range, and a '&' followed by no order byte
        {">>digraph6<<&AG", FALSE, FALSE, TRUE},
        {"&AG", FALSE, FALSE, TRUE},
        {"&?", FALSE, FALSE, TRUE},
        {"&~??~", FALSE, FALSE, TRUE},
        {"&", FALSE, FALSE, FALSE},
        {"&\n", FALSE, FALSE, FALSE},
        {"& ", FALSE, FALSE, FALSE},
        // GraphML, and a line whose first byte is below the range
        {"<graphml>", FALSE, FALSE, FALSE},
        // An adjacency list: the first byte is in the range, the second is not
        {"N=3", FALSE, FALSE, FALSE},
        // The byte after the range, and one with the high bit set
        {"\x7f", FALSE, FALSE, FALSE},
        {"\xff", FALSE, FALSE, FALSE},
        {"\n", FALSE, FALSE, FALSE},
        {"", FALSE, FALSE, FALSE},
        {NULL, FALSE, FALSE, FALSE},
    };

    gp_Message("Starting Generalized Read Iterator Tests");

    for (i = 0; i < (sizeof(predicateCases) / sizeof(predicateCases[0])); i++)
    {
        if (s6_IsSparse6Input(predicateCases[i].firstLine) != predicateCases[i].isSparse6 ||
            g6_IsGraph6Input(predicateCases[i].firstLine) != predicateCases[i].isGraph6 ||
            d6_IsDigraph6Input(predicateCases[i].firstLine) != predicateCases[i].isDigraph6)
        {
            gp_ErrorMessage("Format of \"%s\" was decided as sparse6=%d graph6=%d digraph6=%d.",
                            predicateCases[i].firstLine == NULL ? "(null)" : predicateCases[i].firstLine,
                            s6_IsSparse6Input(predicateCases[i].firstLine),
                            g6_IsGraph6Input(predicateCases[i].firstLine),
                            d6_IsDigraph6Input(predicateCases[i].firstLine));
            return NOTOK;
        }
    }

    // A reader without a graph to read into is refused
    gp_SetQuietMode(QUIETMODE_ALL);

    if (gp_NewReader((&theReadIterator), NULL) == OK)
    {
        gp_SetQuietMode(origQuietMode);
        gp_ErrorMessage("Reader was created without a graph.");
        gp_FreeReader((&theReadIterator));
        return NOTOK;
    }

    gp_SetQuietMode(origQuietMode);

    for (i = 0; Result == OK && i < (sizeof(acceptCases) / sizeof(acceptCases[0])); i++)
    {
        char *inputStr = copySparse6TestString(acceptCases[i][0]);

        if (inputStr == NULL)
            return NOTOK;

        if ((theGraph = gp_New()) == NULL ||
            gp_NewReader((&theReadIterator), theGraph) != OK ||
            gp_InitReaderWithString(theReadIterator, inputStr) != OK ||
            gp_ReadGraph(theReadIterator) != OK ||
            gp_EndReached(theReadIterator))
        {
            gp_ErrorMessage("Unable to read the first graph of accept case %d.", (int)i);
            Result = NOTOK;
        }
        else if (gp_WriteToString(theGraph, (&actualEncoding),
                                  acceptCases[i][1][0] == '&' ? WRITE_DIGRAPH6 : WRITE_GRAPH6) != OK ||
                 actualEncoding == NULL ||
                 strncmp(actualEncoding + strlen(acceptCases[i][1][0] == '&' ? d6Header : g6Header),
                         acceptCases[i][1], strlen(acceptCases[i][1])) != 0)
        {
            gp_ErrorMessage("Graph of accept case %d is \"%s\" rather than \"%s\".",
                            (int)i, actualEncoding == NULL ? "" : actualEncoding,
                            acceptCases[i][1]);
            Result = NOTOK;
        }
        // A reader carries one input, so a second initialization is refused,
        // and refusing it leaves the reader reading the input it has
        else
        {
            GPReadIteratorP theSameReadIterator = theReadIterator;

            gp_SetQuietMode(QUIETMODE_ALL);

            if (gp_InitReaderWithString(theReadIterator, inputStr) == OK)
            {
                gp_SetQuietMode(origQuietMode);
                gp_ErrorMessage("Reader of accept case %d was initialized twice.", (int)i);
                Result = NOTOK;
            }
            // Allocating over an existing reader would lose it
            else if (gp_NewReader((&theSameReadIterator), theGraph) == OK)
            {
                gp_SetQuietMode(origQuietMode);
                gp_ErrorMessage("Reader of accept case %d was allocated over.", (int)i);
                Result = NOTOK;
            }
            else if (gp_ReadGraph(theReadIterator) != OK)
            {
                gp_SetQuietMode(origQuietMode);
                gp_ErrorMessage("Reader of accept case %d stopped reading after "
                                "the refusals.",
                                (int)i);
                Result = NOTOK;
            }

            gp_SetQuietMode(origQuietMode);
        }

        if (actualEncoding != NULL)
        {
            free(actualEncoding);
            actualEncoding = NULL;
        }

        gp_FreeReader((&theReadIterator));
        gp_Free(&theGraph);
        free(inputStr);
    }

    for (i = 0; Result == OK && i < (sizeof(rejectCases) / sizeof(rejectCases[0])); i++)
    {
        char *inputStr = copySparse6TestString(rejectCases[i]);

        if (inputStr == NULL)
            return NOTOK;

        if ((theGraph = gp_New()) == NULL ||
            gp_NewReader((&theReadIterator), theGraph) != OK)
            Result = NOTOK;
        else
        {
            gp_SetQuietMode(QUIETMODE_ALL);

            if (gp_InitReaderWithString(theReadIterator, inputStr) == OK)
            {
                gp_SetQuietMode(origQuietMode);
                gp_ErrorMessage("Reject case %d was accepted by the reader.", (int)i);
                Result = NOTOK;
            }

            gp_SetQuietMode(origQuietMode);
        }

        gp_FreeReader((&theReadIterator));
        gp_Free(&theGraph);
        free(inputStr);
    }

    for (i = 0; Result == OK && i < (sizeof(readFailCases) / sizeof(readFailCases[0])); i++)
    {
        char *inputStr = copySparse6TestString(readFailCases[i]);

        if (inputStr == NULL)
            return NOTOK;

        gp_SetQuietMode(QUIETMODE_ALL);

        if ((theGraph = gp_New()) == NULL ||
            gp_NewReader((&theReadIterator), theGraph) != OK ||
            gp_InitReaderWithString(theReadIterator, inputStr) != OK)
        {
            gp_SetQuietMode(origQuietMode);
            gp_ErrorMessage("Read-failure case %d was refused before it was read.", (int)i);
            Result = NOTOK;
        }
        else if (gp_ReadGraph(theReadIterator) == OK)
        {
            gp_SetQuietMode(origQuietMode);
            gp_ErrorMessage("Read-failure case %d was read as a graph.", (int)i);
            Result = NOTOK;
        }

        gp_SetQuietMode(origQuietMode);

        gp_FreeReader((&theReadIterator));
        gp_Free(&theGraph);
        free(inputStr);
    }

    // The same decision from a file, where the first line is read and pushed
    // back before the reader of that format is given the input
    if (Result == OK)
    {
        // The first graph of each sample is the edgeless graph of order 5,
        // whatever the format of the file
        char const *fileCases[][2] = {
            {"N5-all.g6", "D??"},
            {"N5-all.s6", "D??"},
            {"N5-all.inc.s6", "D??"},
        };

        for (i = 0; Result == OK && i < (sizeof(fileCases) / sizeof(fileCases[0])); i++)
        {
            if ((theGraph = gp_New()) == NULL ||
                gp_NewReader((&theReadIterator), theGraph) != OK ||
                gp_InitReaderWithFileName(theReadIterator, fileCases[i][0]) != OK ||
                gp_ReadGraph(theReadIterator) != OK ||
                gp_EndReached(theReadIterator))
            {
                gp_ErrorMessage("Unable to read the first graph of \"%s\".", fileCases[i][0]);
                Result = NOTOK;
            }
            else if (gp_WriteToString(theGraph, (&actualEncoding), WRITE_GRAPH6) != OK ||
                     actualEncoding == NULL ||
                     strncmp(actualEncoding + strlen(g6Header), fileCases[i][1],
                             strlen(fileCases[i][1])) != 0)
            {
                gp_ErrorMessage("First graph of \"%s\" is \"%s\" rather than \"%s\".",
                                fileCases[i][0], actualEncoding == NULL ? "" : actualEncoding,
                                fileCases[i][1]);
                Result = NOTOK;
            }

            else
            {
                // The samples hold every graph of order 5, and a reader that
                // never advances or never ends would not count them
                int numGraphsRead = 1;

                while (Result == OK)
                {
                    if (gp_ReadGraph(theReadIterator) != OK)
                    {
                        gp_ErrorMessage("Unable to read graph %d of \"%s\".",
                                        numGraphsRead + 1, fileCases[i][0]);
                        Result = NOTOK;
                    }
                    else if (gp_EndReached(theReadIterator))
                        break;
                    else
                        numGraphsRead++;
                }

                if (Result == OK && numGraphsRead != 34)
                {
                    gp_ErrorMessage("Read %d graphs of \"%s\" rather than 34.",
                                    numGraphsRead, fileCases[i][0]);
                    Result = NOTOK;
                }
            }

            if (actualEncoding != NULL)
            {
                free(actualEncoding);
                actualEncoding = NULL;
            }

            gp_FreeReader((&theReadIterator));
            gp_Free(&theGraph);
        }
    }

    if (Result == OK)
        gp_Message("Successfully finished Generalized Read Iterator Tests.\n");
    else
        gp_Message("Error detected in Generalized Read Iterator Tests.\n");

    return Result;
}
