/*
Copyright (c) 1997-2025, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#include "planarity.h"

/****************************************************************************
 MENU-DRIVEN PROGRAM
 ****************************************************************************/
int TransformGraphMenu(void);
int TestAllGraphsMenu(void);

int menu(void)
{
    int Result = OK;

    char lineBuff[MAXLINE + 1];

    int numCharsToReprCOMMANDSTRINGMAXLENGTH = 0;
    char const *choiceStringFormatFormat = " %%%ds";
    char *choiceStringFormat = NULL;
    char choiceString[COMMANDSTRINGMAXLENGTH + 1];

    char *commandString = NULL, *secondOutfile = NULL;

    char command = '\0';

    memset(lineBuff, '\0', (MAXLINE + 1));
    memset(choiceString, '\0', (COMMANDSTRINGMAXLENGTH + 1));

    if (GetNumCharsToReprInt(COMMANDSTRINGMAXLENGTH, &numCharsToReprCOMMANDSTRINGMAXLENGTH) != OK)
    {
        ErrorMessage("Unable to determine number of characters required to represent COMMANDSTRINGMAXLENGTH.\n");
        Result = NOTOK;
    }
    else
    {
        choiceStringFormat = (char *)malloc((strlen(choiceStringFormatFormat) + numCharsToReprCOMMANDSTRINGMAXLENGTH + 1) * sizeof(char));
        if (choiceStringFormat == NULL)
        {
            ErrorMessage("Unable to allocate memory for choice string format string.\n");
            Result = NOTOK;
        }
    }

    if (Result == OK)
    {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
        sprintf(choiceStringFormat, choiceStringFormatFormat, COMMANDSTRINGMAXLENGTH);
#pragma GCC diagnostic pop

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
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
            if (GetLineFromStdin(lineBuff, MAXLINE) != OK ||
                strlen(lineBuff) == 0 || strlen(lineBuff) > 2 ||
                sscanf(lineBuff, choiceStringFormat, choiceString) != 1)
            {
                ErrorMessage("Invalid input; please retry.\n");
                continue;
            }
#pragma GCC diagnostic pop

            if (strncmp(choiceString, "h", 1) == 0)
                helpMessage(NULL);

            else if (strncmp(choiceString, "r", 1) == 0)
            {
                if (Reconfigure() != OK)
                    ErrorMessage("Invalid user input for reconfigure; press 'r' again to retry reconfiguration.\n");

                continue;
            }

            else if (strncmp(choiceString, "x", 1) == 0)
            {
                // TODO: Should I even still emit an error at the menu
                // level, since there's other layers of error messaging?
                if ((Result = TransformGraphMenu()) != OK)
                    ErrorMessage("Transform Graph Menu emitted an error.\n");
            }

            else if (strncmp(choiceString, "t", 1) == 0)
            {
                // TODO: Should I even still emit an error at the menu
                // level, since there's other layers of error messaging?
                if ((Result = TestAllGraphsMenu()) != OK)
                    ErrorMessage("Test All Graphs Menu emitted an error.\n");
            }

            else if (strncmp(choiceString, "q", 1) != 0)
            {
                commandString = choiceString;
                if (GetCommandAndOptionalModifier(commandString, &command, NULL) != OK)
                {
                    Message("Unable to extract command from choice, please retry.\n");
                    commandString = NULL;

                    continue;
                }

                if (command == 'p' || command == 'd' || command == 'o')
                    secondOutfile = (char *)"";

                if (!strchr(GetAlgorithmChoices(), command))
                    Message("Invalid algorithm command choice, please retry.\n");

                else
                {
                    switch (tolower(Mode))
                    {
                    case 's':
                        Result = SpecificGraph(commandString, NULL, NULL, secondOutfile, NULL, NULL, NULL);
                        break;
                    case 'r':
                        Result = RandomGraphs(commandString, 0, 0, NULL);
                        break;
                    case 'm':
                        Result = RandomGraph(commandString, 0, 0, NULL, NULL);
                        break;
                    case 'n':
                        Result = RandomGraph(choiceString, 1, 0, NULL, NULL);
                        break;
                    default:
                        break;
                    }
                }
            }

            if (strcspn(choiceString, "rq") == strlen(choiceString))
            {
                Prompt("\nPress return key to continue...");
                if (GetLineFromStdin(lineBuff, MAXLINE) != OK)
                {
                    // TODO: How should I handle this error, if not to exit out
                    // of menu?
                    ErrorMessage("Unable to fetch from stdin; exiting.\n");
                    Result = NOTOK;
                    break;
                }

                Message("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
                FlushConsole(stdout);
            }
        } while (strncmp(choiceString, "q", 1) != 0);
    }

    // Certain debuggers don't terminate correctly with pending output content
    FlushConsole(stdout);
    FlushConsole(stderr);

    if (choiceStringFormat != NULL)
        free(choiceStringFormat);

    // NOTE: Translates internal planarity codes to appropriate exit codes
    return Result == OK ? 0 : (Result == NONEMBEDDABLE ? 1 : -1);
}

int TransformGraphMenu(void)
{
    int Result = OK;

    int numCharsToReprFILENAMEMAXLENGTH = 0;
    char const *fileNameFormatFormat = " %%%d[^\r\n]";
    char *fileNameFormat = NULL;
    char lineBuff[MAXLINE + 1];
    char infileName[FILENAMEMAXLENGTH + 1];
    char outfileName[FILENAMEMAXLENGTH + 1];
    char commandStr[COMMANDSTRINGMAXLENGTH + 1];
    char outputFormat = '\0';

    memset(lineBuff, '\0', (MAXLINE + 1));
    memset(infileName, '\0', (FILENAMEMAXLENGTH + 1));
    memset(outfileName, '\0', (FILENAMEMAXLENGTH + 1));
    memset(commandStr, '\0', (COMMANDSTRINGMAXLENGTH + 1));

    if (GetNumCharsToReprInt(FILENAMEMAXLENGTH, &numCharsToReprFILENAMEMAXLENGTH) != OK)
    {
        ErrorMessage("Unable to determine number of characters required to represent FILENAMEMAXLENGTH.\n");
        return NOTOK;
    }

    fileNameFormat = (char *)malloc((strlen(fileNameFormatFormat) + numCharsToReprFILENAMEMAXLENGTH + 1) * sizeof(char));
    if (fileNameFormat == NULL)
    {
        ErrorMessage("Unable to allocate memory for filename format string.\n");
        return NOTOK;
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
    sprintf(fileNameFormat, fileNameFormatFormat, FILENAMEMAXLENGTH);
#pragma GCC diagnostic pop

    do
    {
        Prompt("Enter input filename:\n");
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
        if (GetLineFromStdin(lineBuff, MAXLINE) != OK ||
            strlen(lineBuff) == 0 ||
            sscanf(lineBuff, fileNameFormat, infileName) != 1)

        {
            ErrorMessage("Unable to read input filename.\n");
            continue;
        }
#pragma GCC diagnostic pop

        if (strncmp(infileName, "stdin", strlen("stdin")) == 0)
        {
            ErrorMessage("\n\tPlease choose an input file path: stdin not supported from menu.\n\n");
            memset(infileName, '\0', (FILENAMEMAXLENGTH + 1));
        }
    } while (strlen(infileName) == 0);

    do
    {
        Prompt("Enter output filename, or type \"stdout\" to output to console:\n");
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
        if (GetLineFromStdin(lineBuff, MAXLINE) != OK ||
            strlen(lineBuff) == 0 ||
            sscanf(lineBuff, fileNameFormat, outfileName) != 1)
        {
            ErrorMessage("Unable to read output filename.\n");
            continue;
        }
#pragma GCC diagnostic pop
    } while (strlen(outfileName) == 0);

    do
    {
        Message(GetSupportedOutputChoices());
        Prompt("Enter output format: ");
        if (GetLineFromStdin(lineBuff, MAXLINE) != OK ||
            strlen(lineBuff) != 1 ||
            sscanf(lineBuff, " %c", &outputFormat) != 1 ||
            !strchr(GetSupportedOutputFormats(), tolower(outputFormat)))
        {
            ErrorMessage("Invalid choice for output format.\n");
            continue;
        }

        sprintf(commandStr, "-%c", (char)tolower(outputFormat));
    } while (strlen(commandStr) == 0);

    Result = TransformGraph(commandStr, infileName, NULL, NULL, outfileName, NULL);

    if (fileNameFormat != NULL)
        free(fileNameFormat);

    return Result;
}

int TestAllGraphsMenu(void)
{
    int Result = OK;

    char lineBuff[MAXLINE + 1];

    int numCharsToReprFILENAMEMAXLENGTH = 0;
    char const *fileNameFormatFormat = " %%%d[^\r\n]";
    char *fileNameFormat = NULL;
    char infileName[FILENAMEMAXLENGTH + 1];
    char outfileName[FILENAMEMAXLENGTH + 1];

    int numCharsToReprCOMMANDSTRINGMAXLENGTH = 0;
    char const *commandStringFormatFormat = " %%%ds";
    char *commandStringFormat = NULL;
    char commandString[COMMANDSTRINGMAXLENGTH + 1];

    memset(lineBuff, '\0', (MAXLINE + 1));
    memset(infileName, '\0', (FILENAMEMAXLENGTH + 1));
    memset(outfileName, '\0', (FILENAMEMAXLENGTH + 1));
    memset(commandString, '\0', (COMMANDSTRINGMAXLENGTH + 1));

    if (GetNumCharsToReprInt(COMMANDSTRINGMAXLENGTH, &numCharsToReprCOMMANDSTRINGMAXLENGTH) != OK)
    {
        ErrorMessage("Unable to determine number of characters required to represent COMMANDSTRINGMAXLENGTH.\n");
        return NOTOK;
    }

    commandStringFormat = (char *)malloc((strlen(commandStringFormatFormat) + numCharsToReprCOMMANDSTRINGMAXLENGTH + 1) * sizeof(char));
    if (commandStringFormat == NULL)
    {
        ErrorMessage("Unable to allocate memory for command string format string.\n");
        return NOTOK;
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
    sprintf(commandStringFormat, commandStringFormatFormat, COMMANDSTRINGMAXLENGTH);
#pragma GCC diagnostic pop

    if (GetNumCharsToReprInt(FILENAMEMAXLENGTH, &numCharsToReprFILENAMEMAXLENGTH) != OK)
    {
        ErrorMessage("Unable to determine number of characters required to represent FILENAMEMAXLENGTH.\n");
        return NOTOK;
    }

    fileNameFormat = (char *)malloc((strlen(fileNameFormatFormat) + numCharsToReprFILENAMEMAXLENGTH + 1) * sizeof(char));
    if (fileNameFormat == NULL)
    {
        ErrorMessage("Unable to allocate memory for filename format string.\n");
        return NOTOK;
    }
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
    sprintf(fileNameFormat, fileNameFormatFormat, FILENAMEMAXLENGTH);
#pragma GCC diagnostic pop

    do
    {
        Prompt("Enter input filename:\n");
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
        if (GetLineFromStdin(lineBuff, MAXLINE) != OK ||
            strlen(lineBuff) == 0 ||
            sscanf(lineBuff, fileNameFormat, infileName) != 1)
        {
            ErrorMessage("Unable to read input filename.\n");
            continue;
        }
#pragma GCC diagnostic pop

        if (strncmp(infileName, "stdin", strlen("stdin")) == 0)
        {
            ErrorMessage("\n\tPlease choose an input file path: stdin not supported from menu.\n\n");
            memset(infileName, '\0', (FILENAMEMAXLENGTH + 1));
        }
    } while (strlen(infileName) == 0);

    do
    {
        Prompt("Enter output filename, or type \"stdout\" to output to console:\n");
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
        if (GetLineFromStdin(lineBuff, MAXLINE) != OK ||
            strlen(lineBuff) == 0 ||
            sscanf(lineBuff, fileNameFormat, outfileName) != 1)
        {
            ErrorMessage("Unable to read output filename.\n");
            continue;
        }
#pragma GCC diagnostic pop
    } while (strlen(outfileName) == 0);

    do
    {
        Message(GetAlgorithmSpecifiers());
        Prompt("Enter algorithm specifier (with optional modifier): ");
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
        if (GetLineFromStdin(lineBuff, MAXLINE) != OK ||
            strlen(lineBuff) == 0 || strlen(lineBuff) > 2 ||
            sscanf(lineBuff, commandStringFormat, commandString) != 1)
        {
            ErrorMessage("Unable to command and optional modifier.\n");
            continue;
        }
#pragma GCC diagnostic pop
    } while (strlen(commandString) == 0);

    Result = TestAllGraphs(commandString, infileName, outfileName, NULL);

    if (commandStringFormat != NULL)
        free(commandStringFormat);

    if (fileNameFormat != NULL)
        free(fileNameFormat);

    return Result;
}
