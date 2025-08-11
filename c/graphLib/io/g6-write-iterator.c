/*
Copyright (c) 1997-2025, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#include <stdlib.h>
#include <string.h>

#include "g6-write-iterator.h"
#include "g6-api-utilities.h"

int allocateG6WriteIterator(G6WriteIteratorP *ppG6WriteIterator, graphP pGraph)
{
    if (ppG6WriteIterator != NULL && (*ppG6WriteIterator) != NULL)
    {
        ErrorMessage("G6WriteIterator is not NULL and therefore can't be "
                     "allocated.\n");
        return NOTOK;
    }

    // numGraphsWritten, graphOrder, numCharsForGraphOrder,
    // numCharsForGraphEncoding, and currGraphBuffSize all set to 0
    (*ppG6WriteIterator) = (G6WriteIteratorP)calloc(1, sizeof(G6WriteIterator));
    if ((*ppG6WriteIterator) == NULL)
    {
        ErrorMessage("Unable to allocate memory for G6WriteIterator.\n");
        return NOTOK;
    }

    (*ppG6WriteIterator)->g6Output = NULL;
    (*ppG6WriteIterator)->currGraphBuff = NULL;
    (*ppG6WriteIterator)->columnOffsets = NULL;

    if (pGraph == NULL || gp_GetN(pGraph) <= 0)
    {
        ErrorMessage("[ERROR] Graph to write must be allocated and initialized "
                     "with an order greater than 0 to use the G6WriteIterator."
                     "\n");
        if (freeG6WriteIterator(ppG6WriteIterator) != OK)
            ErrorMessage("Unable to free the G6WriteIterator.\n");
        return NOTOK;
    }
    else
        (*ppG6WriteIterator)->currGraph = pGraph;

    return OK;
}

bool _isG6WriteIteratorAllocated(G6WriteIteratorP pG6WriteIterator)
{
    if (pG6WriteIterator == NULL)
    {
        ErrorMessage("G6WriteIterator is NULL.\n");
        return false;
    }
    else
    {
        if (sf_ValidateStrOrFile(pG6WriteIterator->g6Output) != OK)
        {
            ErrorMessage("G6WriteIterator's g6Output is not valid.\n");
            return false;
        }
        if (pG6WriteIterator->currGraphBuff == NULL)
        {
            ErrorMessage("G6WriteIterator's currGraphBuff is NULL.\n");
            return false;
        }
        if (pG6WriteIterator->columnOffsets == NULL)
        {
            ErrorMessage("G6WriteIterator's columnOffsets is NULL.\n");
            return false;
        }
        if (pG6WriteIterator->currGraph == NULL)
        {
            ErrorMessage("G6WriteIterator's currGraph is NULL.\n");
            return false;
        }
        if (gp_GetN(pG6WriteIterator->currGraph) == 0)
        {
            ErrorMessage("G6WriteIterator's currGraph does not contain a valid "
                         "graph.\n");
            return false;
        }
    }

    return true;
}

int getNumGraphsWritten(G6WriteIteratorP pG6WriteIterator, int *pNumGraphsRead)
{
    if (!_isG6WriteIteratorAllocated(pG6WriteIterator))
    {
        ErrorMessage("G6WriteIterator is not allocated, so unable to get "
                     "numGraphsWritten.\n");
        return NOTOK;
    }

    (*pNumGraphsRead) = pG6WriteIterator->numGraphsWritten;

    return OK;
}

// FIXME: This accessor's name doesn't exactly match the name of the attribute
// from the G6WriteIterator struct, i.e. graphOrder
int getOrderOfGraphToWrite(G6WriteIteratorP pG6WriteIterator, int *pGraphOrder)
{
    if (!_isG6WriteIteratorAllocated(pG6WriteIterator))
    {
        ErrorMessage("G6WriteIterator is not allocated, so unable to get graph "
                     "order.\n");
        return NOTOK;
    }

    (*pGraphOrder) = pG6WriteIterator->graphOrder;

    return OK;
}

// FIXME: This accessor's name doesn't exactly match the name of the attribute
// from the G6WriteIterator struct, i.e. currGraphBuff
int getGraphBuff(G6WriteIteratorP pG6WriteIterator, char **ppCurrGraphBuff)
{
    if (!_isG6WriteIteratorAllocated(pG6WriteIterator))
    {
        ErrorMessage("G6WriteIterator is not allocated, so unable to get "
                     "currGraphBuff.\n");
        return NOTOK;
    }

    (*ppCurrGraphBuff) = pG6WriteIterator->currGraphBuff;

    return OK;
}

// FIXME: This accessor's name doesn't exactly match the name of the attribute
// from the G6WriteIterator struct, i.e. currGraph
int getPointerToGraphToWrite(G6WriteIteratorP pG6WriteIterator, graphP *ppGraph)
{
    if (!_isG6WriteIteratorAllocated(pG6WriteIterator))
    {
        ErrorMessage("[ERROR] G6WriteIterator is not allocated, so unable to "
                     "get pointer to graph to write.\n");
        return NOTOK;
    }

    (*ppGraph) = pG6WriteIterator->currGraph;

    return OK;
}

int beginG6WriteIterationToG6String(G6WriteIteratorP pG6WriteIterator)
{
    return beginG6WriteIterationToG6StrOrFile(
        pG6WriteIterator,
        sf_New(NULL, NULL, WRITETEXT));
}

int beginG6WriteIterationToG6FilePath(G6WriteIteratorP pG6WriteIterator, char *outputFilename)
{
    return beginG6WriteIterationToG6StrOrFile(
        pG6WriteIterator,
        sf_New(NULL, outputFilename, WRITETEXT));
}

int beginG6WriteIterationToG6StrOrFile(G6WriteIteratorP pG6WriteIterator, strOrFileP outputContainer)
{
    if (pG6WriteIterator == NULL)
    {
        ErrorMessage("G6WriteIterator is not allocated.\n");
        return NOTOK;
    }

    // TODO: Should I actually error out if we've already written to the given
    // strOrFile container, i.e. the sb_GetSize(theStrOrFile->theStr) > 0?
    // This is because in _beginG6WriteIteration() I always write the header
    // >>graph6<<... Two other alternatives are that I could *NOT* write the
    // header, or I could only write the header if the strOrFile container is
    // empty (N.B. that using pattern of sf_getc(), checking that char isn't
    // EOF, followed by sf_ungetc() will *not* work currently due to sf_ungetc()
    // erroring out if you try to call it on an output container).
    if (sf_ValidateStrOrFile(outputContainer) != OK)
    {
        ErrorMessage("Invalid strOrFile output container provided.\n");
        return NOTOK;
    }

    pG6WriteIterator->g6Output = outputContainer;

    return _beginG6WriteIteration(pG6WriteIterator);
}

int _beginG6WriteIteration(G6WriteIteratorP pG6WriteIterator)
{
    char const *g6Header = ">>graph6<<";

    // TODO: See comment in beginG6WriteIterationToG6StrOrFile()
    if (sf_fputs(g6Header, pG6WriteIterator->g6Output) < 0)
    {
        ErrorMessage("Unable to fputs header to g6Output.\n");
        return NOTOK;
    }

    pG6WriteIterator->graphOrder = gp_GetN(pG6WriteIterator->currGraph);

    pG6WriteIterator->columnOffsets = (int *)calloc(pG6WriteIterator->graphOrder + 1, sizeof(int));

    if (pG6WriteIterator->columnOffsets == NULL)
    {
        ErrorMessage("Unable to allocate memory for column offsets.\n");
        return NOTOK;
    }

    _precomputeColumnOffsets(pG6WriteIterator->columnOffsets, pG6WriteIterator->graphOrder);

    pG6WriteIterator->numCharsForGraphOrder = _getNumCharsForGraphOrder(pG6WriteIterator->graphOrder);
    pG6WriteIterator->numCharsForGraphEncoding = _getNumCharsForGraphEncoding(pG6WriteIterator->graphOrder);
    // Must add 3 bytes for newline, possible carriage return, and null terminator
    pG6WriteIterator->currGraphBuffSize = pG6WriteIterator->numCharsForGraphOrder + pG6WriteIterator->numCharsForGraphEncoding + 3;
    pG6WriteIterator->currGraphBuff = (char *)calloc(pG6WriteIterator->currGraphBuffSize, sizeof(char));

    if (pG6WriteIterator->currGraphBuff == NULL)
    {
        ErrorMessage("Unable to allocate memory for currGraphBuff.\n");
        return NOTOK;
    }

    return OK;
}

void _precomputeColumnOffsets(int *columnOffsets, int graphOrder)
{
    if (columnOffsets == NULL)
    {
        ErrorMessage("Must allocate columnOffsets memory before "
                     "precomputation.\n");
        return;
    }

    columnOffsets[0] = 0;
    columnOffsets[1] = 0;
    for (int i = 2; i <= graphOrder; i++)
        columnOffsets[i] = columnOffsets[i - 1] + (i - 1);
}

int writeGraphUsingG6WriteIterator(G6WriteIteratorP pG6WriteIterator)
{
    if (_encodeAdjMatAsG6(pG6WriteIterator) != OK)
    {
        ErrorMessage("Error converting adjacency matrix to g6 format.\n");
        return NOTOK;
    }

    if (_printEncodedGraph(pG6WriteIterator) != OK)
    {
        ErrorMessage("Unable to output g6 encoded graph to string-or-file container.\n");
        return NOTOK;
    }

    return OK;
}

int _encodeAdjMatAsG6(G6WriteIteratorP pG6WriteIterator)
{
    int exitCode = OK;

    if (!_isG6WriteIteratorAllocated(pG6WriteIterator))
    {
        ErrorMessage("Unable to encode graph with invalid G6WriteIterator\n");
        return NOTOK;
    }

    char *g6Encoding = pG6WriteIterator->currGraphBuff;
    int *columnOffsets = pG6WriteIterator->columnOffsets;
    graphP pGraph = pG6WriteIterator->currGraph;

    // memset ensures all bits are zero, which means we only need to set the bits
    // that correspond to an edge; this also takes care of padding zeroes for us
    memset(pG6WriteIterator->currGraphBuff, 0, (pG6WriteIterator->currGraphBuffSize) * sizeof(char));

    int graphOrder = pG6WriteIterator->graphOrder;
    int numCharsForGraphOrder = pG6WriteIterator->numCharsForGraphOrder;
    int numCharsForGraphEncoding = pG6WriteIterator->numCharsForGraphEncoding;
    int totalNumCharsForOrderAndGraph = numCharsForGraphOrder + numCharsForGraphEncoding;

    if (graphOrder > 62)
    {
        g6Encoding[0] = 126;
        // bytes 1 through 3 will be populated with the 18-bit representation of the graph order
        int intermediate = -1;
        for (int i = 0; i < 3; i++)
        {
            intermediate = graphOrder >> (6 * i);
            g6Encoding[3 - i] = intermediate & 63;
            g6Encoding[3 - i] += 63;
        }
    }
    else if (graphOrder > 1 && graphOrder < 63)
    {
        g6Encoding[0] = (char)(graphOrder + 63);
    }

    int u = NIL, v = NIL, e = NIL;
    exitCode = _getFirstEdge(pGraph, &e, &u, &v);

    if (exitCode != OK)
    {
        ErrorMessage("Unable to fetch first edge in graph.\n");
        return exitCode;
    }

    int charOffset = 0;
    int bitPositionPower = 0;
    while (u != NIL && v != NIL)
    {
        // The internal graph representation is usually 1-based, but may be 0-based, so
        // one must subtract the index of the first vertex (i.e. result of gp_GetFirstVertex)
        // because the .g6 format is 0-based
        u -= gp_GetFirstVertex(theGraph);
        v -= gp_GetFirstVertex(theGraph);

        // The columnOffset machinery assumes that we are traversing the edges represented in
        // the upper-triangular matrix. Since we are dealing with simple graphs, if (v, u)
        // exists, then (u, v) exists, and so the edge is indicated by a 1 in row = min(u, v)
        // and col = max(u, v) in the upper-triangular adjacency matrix.
        if (v < u)
        {
            int tempVert = v;
            v = u;
            u = tempVert;
        }

        // (columnOffsets[v] + u) describes the bit index of the current edge
        // given the column and row in the adjacency matrix representation;
        // the byte is floor((columnOffsets[v] + u) / 6) and the we determine which
        // bit to set in that byte by left-shifting 1 by (5 - ((columnOffsets[v] + u) % 6))
        // (transforming the ((columnOffsets[v] + u) % 6)th bit from the left to the
        // (5 - ((columnOffsets[v] + u) % 6))th bit from the right)
        charOffset = numCharsForGraphOrder + ((columnOffsets[v] + u) / 6);
        bitPositionPower = 5 - ((columnOffsets[v] + u) % 6);

        g6Encoding[charOffset] |= (1u << bitPositionPower);

        exitCode = _getNextEdge(pGraph, &e, &u, &v);

        if (exitCode != OK)
        {
            ErrorMessage("Unable to fetch next edge in graph.\n");
            free(columnOffsets);
            free(g6Encoding);
            return exitCode;
        }
    }

    // Bytes corresponding to graph order have already been modified to
    // correspond to printable ascii character (i.e. by adding 63); must
    // now do the same for bytes corresponding to edge lists
    for (int i = numCharsForGraphOrder; i < totalNumCharsForOrderAndGraph; i++)
        g6Encoding[i] += 63;

    return exitCode;
}

int _getFirstEdge(graphP theGraph, int *e, int *u, int *v)
{
    if (theGraph == NULL)
        return NOTOK;

    if ((*e) >= gp_EdgeInUseIndexBound(theGraph))
    {
        ErrorMessage("First edge is outside bounds.");
        return NOTOK;
    }

    (*e) = gp_GetFirstEdge(theGraph);

    return _getNextInUseEdge(theGraph, e, u, v);
}

int _getNextEdge(graphP theGraph, int *e, int *u, int *v)
{
    if (theGraph == NULL)
        return NOTOK;

    (*e) += 2;

    return _getNextInUseEdge(theGraph, e, u, v);
}

int _getNextInUseEdge(graphP theGraph, int *e, int *u, int *v)
{
    // FIXME: the exitCode never changes from OK, so this is not needed. Should
    // there be additional checks of the macro expansions, or just remove?
    int exitCode = OK;
    int EsizeOccupied = gp_EdgeInUseIndexBound(theGraph);

    (*u) = NIL;
    (*v) = NIL;

    if ((*e) < EsizeOccupied)
    {
        while (!gp_EdgeInUse(theGraph, (*e)))
        {
            (*e) += 2;
            if ((*e) >= EsizeOccupied)
                break;
        }

        if ((*e) < EsizeOccupied && gp_EdgeInUse(theGraph, (*e)))
        {
            (*u) = gp_GetNeighbor(theGraph, (*e));
            (*v) = gp_GetNeighbor(theGraph, gp_GetTwinArc(theGraph, (*e)));
        }
    }

    return exitCode;
}

int _printEncodedGraph(G6WriteIteratorP pG6WriteIterator)
{
    if (!_isG6WriteIteratorAllocated(pG6WriteIterator))
    {
        ErrorMessage("Unable to print encoded graph using invalid "
                     "G6WriteIterator.\n");
        return NOTOK;
    }

    if (strlen(pG6WriteIterator->currGraphBuff) == 0)
    {
        ErrorMessage("Unable to print; g6 encoding is empty.\n");
        return NOTOK;
    }

    if (sf_fputs(pG6WriteIterator->currGraphBuff, pG6WriteIterator->g6Output) < 0)
    {
        ErrorMessage("Failed to output all characters of g6 encoding.\n");
        return NOTOK;
    }

    if (sf_fputs("\n", pG6WriteIterator->g6Output) < 0)
    {
        ErrorMessage("Failed to put line terminator after g6 encoding.\n");
        return NOTOK;
    }

    return OK;
}

int endG6WriteIteration(G6WriteIteratorP pG6WriteIterator)
{
    // FIXME: the exitCode never changes from OK, so this is not needed. Should
    // there be additional checks, such as adding a return value for sf_Free()?
    // As long as you pass the same address to free() as you did for malloc(),
    // there's no real way to check that free() succeeded.
    int exitCode = OK;

    if (pG6WriteIterator != NULL)
    {
        if (pG6WriteIterator->g6Output != NULL)
            sf_Free(&(pG6WriteIterator->g6Output));

        if (pG6WriteIterator->currGraphBuff != NULL)
        {
            free(pG6WriteIterator->currGraphBuff);
            pG6WriteIterator->currGraphBuff = NULL;
        }

        if (pG6WriteIterator->columnOffsets != NULL)
        {
            free((pG6WriteIterator->columnOffsets));
            pG6WriteIterator->columnOffsets = NULL;
        }
    }

    return exitCode;
}

int freeG6WriteIterator(G6WriteIteratorP *ppG6WriteIterator)
{
    // FIXME: See comment in endG6WriteIteration() about exitCode.
    int exitCode = OK;

    if (ppG6WriteIterator != NULL && (*ppG6WriteIterator) != NULL)
    {
        if ((*ppG6WriteIterator)->g6Output != NULL)
            sf_Free(&((*ppG6WriteIterator)->g6Output));

        (*ppG6WriteIterator)->numGraphsWritten = 0;
        (*ppG6WriteIterator)->graphOrder = 0;

        if ((*ppG6WriteIterator)->currGraphBuff != NULL)
        {
            free((*ppG6WriteIterator)->currGraphBuff);
            (*ppG6WriteIterator)->currGraphBuff = NULL;
        }

        if ((*ppG6WriteIterator)->columnOffsets != NULL)
        {
            free(((*ppG6WriteIterator)->columnOffsets));
            (*ppG6WriteIterator)->columnOffsets = NULL;
        }

        // N.B. The G6WriteIterator doesn't "own" the graph, so we don't free it.
        (*ppG6WriteIterator)->currGraph = NULL;

        free((*ppG6WriteIterator));
        (*ppG6WriteIterator) = NULL;
    }

    return exitCode;
}

int _WriteGraphToG6FilePath(graphP pGraph, char *g6OutputFilename)
{
    strOrFileP outputContainer = sf_New(NULL, g6OutputFilename, WRITETEXT);
    if (outputContainer == NULL)
    {
        ErrorMessage("Unable to allocate outputContainer to which to write.\n");
        return NOTOK;
    }

    return _WriteGraphToG6StrOrFile(pGraph, outputContainer, NULL);
}

int _WriteGraphToG6String(graphP pGraph, char **g6OutputStr)
{
    strOrFileP outputContainer = sf_New(NULL, NULL, WRITETEXT);
    if (outputContainer == NULL)
    {
        ErrorMessage("Unable to allocate outputContainer to which to write.\n");
        return NOTOK;
    }

    // N.B. If g6OutputStr is a pointer to a pointer to a block of memory that
    // has been allocated, i.e. if g6OutputStr != NULL && (*g6OutputStr) != NULL
    // then an error will be emitted by _WriteGraphToG6StrOrFile().
    // N.B. Once the graph is successfully written, the string is taken from
    // the G6WriteIterator's outputContainer and assigned to (*g6OutputStr)
    // before ending G6 write iteration
    return _WriteGraphToG6StrOrFile(pGraph, outputContainer, g6OutputStr);
}

int _WriteGraphToG6StrOrFile(graphP pGraph, strOrFileP outputContainer, char **outputStr)
{
    int exitCode = OK;

    G6WriteIteratorP pG6WriteIterator = NULL;

    if (sf_ValidateStrOrFile(outputContainer) != OK)
    {
        ErrorMessage("Invalid G6 output container.\n");
        return NOTOK;
    }

    if (outputContainer->theStr != NULL && (outputStr == NULL))
    {
        ErrorMessage("If writing G6 to string, must provide pointer-pointer "
                     "to allow _WriteGraphToG6StrOrFile() to assign the address "
                     "of the output string.\n");
        return NOTOK;
    }

    if (outputStr != NULL && (*outputStr) != NULL)
    {
        ErrorMessage("(*outputStr) should not point to allocated memory.");
        return NOTOK;
    }

    exitCode = allocateG6WriteIterator(&pG6WriteIterator, pGraph);
    if (exitCode != OK)
    {
        ErrorMessage("Unable to allocate G6WriteIterator.\n");
        freeG6WriteIterator(&pG6WriteIterator);
        return exitCode;
    }

    exitCode = beginG6WriteIterationToG6StrOrFile(pG6WriteIterator, outputContainer);
    if (exitCode != OK)
    {
        ErrorMessage("Unable to begin G6 write iteration.\n");
        freeG6WriteIterator(&pG6WriteIterator);
        return exitCode;
    }

    exitCode = writeGraphUsingG6WriteIterator(pG6WriteIterator);
    if (exitCode != OK)
        ErrorMessage("Unable to write graph using G6WriteIterator.\n");
    else
    {
        if (outputStr != NULL && pG6WriteIterator->g6Output->theStr != NULL)
            (*outputStr) = sf_takeTheStr(pG6WriteIterator->g6Output);
    }

    int endG6WriteIterationCode = endG6WriteIteration(pG6WriteIterator);
    if (endG6WriteIterationCode != OK)
    {
        ErrorMessage("Unable to end G6 write iteration.\n");
        exitCode = endG6WriteIterationCode;
    }

    int freeG6WriteIteratorCode = freeG6WriteIterator(&pG6WriteIterator);
    if (freeG6WriteIteratorCode != OK)
    {
        ErrorMessage("Unable to free G6Writer.\n");
        exitCode = freeG6WriteIteratorCode;
    }

    return exitCode;
}
