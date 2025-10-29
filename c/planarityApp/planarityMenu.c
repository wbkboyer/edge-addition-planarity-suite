/*
Copyright (c) 1997-2025, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#include "planarity.h"

/****************************************************************************
 MENU-DRIVEN PROGRAM
 ****************************************************************************/
void TransformGraphMenu(void);
void TestAllGraphsMenu(void);

int menu(void)
{
    char lineBuff[MAXLINE + 1];
    char commandString[3];
    char command = '\0';

    do
    {
        Message(GetProjectTitle());

        Message(GetAlgorithmSpecifiers());

        Message(
            "X. Transform single graph in supported file to .g6, adjacency list, or adjacency matrix\n"
            "T. Perform an algorithm test on all graphs in .g6 input file\n"
            "H. Help message for command line version\n"
            "R. Reconfigure options\n"
            "Q. Quit\n"
            "\n");

        Prompt("Enter Choice: ");
        if (GetLineFromStdin(lineBuff, MAXLINE) != OK ||
            strlen(lineBuff) == 0 || strlen(lineBuff) > 2 ||
            sscanf(lineBuff, " %2s", commandString) != 1)
        {
            ErrorMessage("Invalid input; please retry.\n");
            continue;
        }

        if (GetCommandAndOptionalModifier(commandString, &command, NULL) != OK)
        {
            Message("Unable to extract command from choice, please try again.\n");
            continue;
        }

        if (command == 'h')
            helpMessage(NULL);

        else if (command == 'r')
            Reconfigure();

        else if (command == 'x')
            TransformGraphMenu();

        else if (command == 't')
            TestAllGraphsMenu();

        else if (command != 'q')
        {
            char *secondOutfile = NULL;
            if (command == 'p' || command == 'd' || command == 'o')
                secondOutfile = (char *)"";

            if (!strchr(GetAlgorithmChoices(), command))
            {
                Message("Invalid menu choice, please try again.\n");
            }

            else
            {
                switch (tolower(Mode))
                {
                case 's':
                    SpecificGraph(commandString, NULL, NULL, secondOutfile, NULL, NULL, NULL);
                    break;
                case 'r':
                    RandomGraphs(commandString, 0, 0, NULL);
                    break;
                case 'm':
                    RandomGraph(commandString, 0, 0, NULL, NULL);
                    break;
                case 'n':
                    RandomGraph(commandString, 1, 0, NULL, NULL);
                    break;
                default:
                    break;
                }
            }
        }

        if (command != 'r' && command != 'q')
        {
            Prompt("\nPress return key to continue...");
            if (GetLineFromStdin(lineBuff, MAXLINE) != OK)
            {
                ErrorMessage("Unable to fetch from stdin; exiting.\n");
                return -1;
            }

            Message("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
            FlushConsole(stdout);
        }
    } while (command != 'q');

    // Certain debuggers don't terminate correctly with pending output content
    FlushConsole(stdout);
    FlushConsole(stderr);

    return 0;
}

void TransformGraphMenu(void)
{
    int Result = OK;

    int numCharsToReprFILENAMEMAXLENGTH = 0;
    char const *fileNameFormatFormat = " %%%d[^\r\n]";
    char *fileNameFormat = NULL;
    char infileName[FILENAMEMAXLENGTH + 1];
    char outfileName[FILENAMEMAXLENGTH + 1];
    char outputFormat = '\0';
    char commandStr[4];
    char lineBuff[MAXLINE + 1];

    infileName[0] = outfileName[0] = commandStr[0] = '\0';

    if (GetNumCharsToReprInt(FILENAMEMAXLENGTH, &numCharsToReprFILENAMEMAXLENGTH) != OK)
    {
        ErrorMessage("Unable to determine number of characters required to represent FILENAMEMAXLENGTH.\n");
        return;
    }

    fileNameFormat = (char *)malloc((strlen(fileNameFormatFormat) + numCharsToReprFILENAMEMAXLENGTH + 1) * sizeof(char));
    if (fileNameFormat == NULL)
    {
        ErrorMessage("Unable to allocate memory for filename format string.\n");
        return;
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
    sprintf(fileNameFormat, fileNameFormatFormat, FILENAMEMAXLENGTH);
#pragma GCC diagnostic pop

    do
    {
        Prompt("Enter input filename:\n");
        if (GetLineFromStdin(lineBuff, MAXLINE) != OK ||
            strlen(lineBuff) == 0 ||
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
            sscanf(lineBuff, fileNameFormat, infileName) != 1)
#pragma GCC diagnostic pop
        {
            ErrorMessage("Unable to read input filename.\n");
            continue;
        }

        if (strncmp(infileName, "stdin", strlen("stdin")) == 0)
        {
            ErrorMessage("\n\tPlease choose an input file path: stdin not supported from menu.\n\n");
            infileName[0] = '\0';
        }
    } while (strlen(infileName) == 0);

    do
    {
        Prompt("Enter output filename, or type \"stdout\" to output to console:\n");
        if (GetLineFromStdin(lineBuff, MAXLINE) != OK ||
            strlen(lineBuff) == 0 ||
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
            sscanf(lineBuff, fileNameFormat, outfileName) != 1)
#pragma GCC diagnostic pop
        {
            ErrorMessage("Unable to read output filename.\n");
            continue;
        }
    } while (strlen(outfileName) == 0);

    do
    {
        Message(GetSupportedOutputChoices());
        Prompt("Enter output format: ");
        if (GetLineFromStdin(lineBuff, MAXLINE) != OK ||
            strlen(lineBuff) != 1 ||
            sscanf(lineBuff, " %c", &outputFormat) != 1)
        {
            ErrorMessage("Invalid choice for output format.\n");
            continue;
        }
        outputFormat = (char)tolower(outputFormat);
        if (strchr(GetSupportedOutputFormats(), outputFormat))
            sprintf(commandStr, "-%c", outputFormat);
    } while (strlen(commandStr) == 0);

    Result = TransformGraph(commandStr, infileName, NULL, NULL, outfileName, NULL);
    if (Result != OK)
        ErrorMessage("Failed to perform transformation.\n");

    if (fileNameFormat != NULL)
        free(fileNameFormat);
}

void TestAllGraphsMenu(void)
{
    int Result = OK;

    int numCharsToReprFILENAMEMAXLENGTH = 0;
    char const *fileNameFormatFormat = " %%%d[^\r\n]";
    char *fileNameFormat = NULL;
    char infileName[MAXLINE + 1];
    char outfileName[MAXLINE + 1];
    char commandString[3];
    char lineBuff[MAXLINE + 1];

    infileName[0] = outfileName[0] = '\0';

    if (GetNumCharsToReprInt(FILENAMEMAXLENGTH, &numCharsToReprFILENAMEMAXLENGTH) != OK)
    {
        ErrorMessage("Unable to determine number of characters required to represent FILENAMEMAXLENGTH.\n");
        return;
    }

    fileNameFormat = (char *)malloc((strlen(fileNameFormatFormat) + numCharsToReprFILENAMEMAXLENGTH + 1) * sizeof(char));
    if (fileNameFormat == NULL)
    {
        ErrorMessage("Unable to allocate memory for filename format string.\n");
        return;
    }
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
    sprintf(fileNameFormat, fileNameFormatFormat, FILENAMEMAXLENGTH);
#pragma GCC diagnostic pop

    do
    {
        Prompt("Enter input filename:\n");
        if (GetLineFromStdin(lineBuff, MAXLINE) != OK ||
            strlen(lineBuff) == 0 ||
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
            sscanf(lineBuff, fileNameFormat, infileName) != 1)
#pragma GCC diagnostic pop
        {
            ErrorMessage("Unable to read input filename.\n");
            continue;
        }

        if (strncmp(infileName, "stdin", strlen("stdin")) == 0)
        {
            ErrorMessage("\n\tPlease choose an input file path: stdin not supported from menu.\n\n");
            infileName[0] = '\0';
        }
    } while (strlen(infileName) == 0);

    do
    {
        Prompt("Enter output filename, or type \"stdout\" to output to console:\n");
        if (GetLineFromStdin(lineBuff, MAXLINE) != OK ||
            strlen(lineBuff) == 0 ||
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
            sscanf(lineBuff, fileNameFormat, outfileName) != 1)
#pragma GCC diagnostic pop
        {
            ErrorMessage("Unable to read output filename.\n");
            continue;
        }
    } while (strlen(outfileName) == 0);

    do
    {
        Message(GetAlgorithmSpecifiers());
        Prompt("Enter algorithm specifier (with optional modifier): ");
        if (GetLineFromStdin(lineBuff, MAXLINE) != OK ||
            strlen(lineBuff) == 0 || strlen(lineBuff) > 2 ||
            sscanf(lineBuff, " %2s", commandString) != 1)
        {
            ErrorMessage("Unable to command and optional modifier.\n");
            continue;
        }
    } while (strlen(commandString) == 0);

    Result = TestAllGraphs(commandString, infileName, outfileName, NULL);
    if (Result != OK)
        ErrorMessage("Algorithm test on all graphs in .g6 input file failed.\n");

    if (fileNameFormat != NULL)
        free(fileNameFormat);
}
