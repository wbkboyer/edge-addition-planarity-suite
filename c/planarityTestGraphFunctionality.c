/*
Copyright (c) 1997-2024, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#include "planarity.h"
#include "graph.h"
#include "platformTime.h"
#include "g6-read-iterator.h"
#include "strOrFile.h"

typedef struct {
	int numGraphsRead;
	int numOK;
	int numNONEMBEDDABLE;
	int errorFlag;
} testAllStats;

typedef testAllStats * testAllStatsP;

int transformFile(graphP theGraph, char *infileName);
int transformString(graphP theGraph, char *inputStr);
int testAllGraphs(graphP theGraph, char command, char *inputStr, testAllStatsP stats);

int _getNumCharsToReprInt(int theNum);

/****************************************************************************
 TestGraphFunctionality()
 commandString - command to run; e.g. `-t(gam)` to transform graph to .g6, adjacency list, or
 adjacency matrix format, or `-(pdo234)` to perform the corresponding algorithm on each graph in
 a .g6 file
 infileName - name of file to read, or NULL to cause the program to prompt the user for a filename
 inputStr - string containing input graph, or NULL to cause the program to fall back on reading from file
 outputBase - pointer to the flag set for whether output is 0- or 1-based
 outputFormat - output format; currently only supports WRITE_ADJLIST
 outfileName - name of primary output file, or NULL to construct an output filename based on the input
 outputStr - pointer to string which we wish to use to store the transformation output
 ****************************************************************************/
int TestGraphFunctionality(char *commandString, char *infileName, char *inputStr, int *outputBase, char *outfileName, char **outputStr)
{
	int Result = OK;
	platform_time start, end;
	double duration;

	int charsAvailForFilename = 0;
	char *messageFormat = NULL;
	char messageContents[MAXLINE + 1];

	graphP theGraph;

	// Create the graph and, if needed, attach the correct algorithm to it
	theGraph = gp_New();

	int outputFormat = -1;

	if (commandString[0] == '-')
	{
		if (commandString[1] == 't')
		{
			if (commandString[2] == 'g')
				outputFormat = WRITE_G6;
			else if (commandString[2] == 'a')
				outputFormat = WRITE_ADJLIST;
			else if (commandString[2] == 'm')
				outputFormat = WRITE_ADJMATRIX;
			else
			{
				ErrorMessage("Invalid argument; currently, only -t(gam) is allowed.\n");
				return NOTOK;
			}

			if (inputStr)
				Result = transformString(theGraph, inputStr);
			else
				Result = transformFile(theGraph, infileName);

			if (Result != OK) {
				ErrorMessage("Unable to transform input graph.\n");
			}
			else
			{
				// Want to know whether the output is 0- or 1-based; will always be 
				// 0-based for transformations of .g6 input
				if (outputBase != NULL)
					(*outputBase) = (theGraph->internalFlags & FLAGS_ZEROBASEDIO) ? 1 : 0;

				if (outputStr != NULL)
					Result = gp_WriteToString(theGraph, outputStr, outputFormat);
				else
					Result = gp_Write(theGraph, outfileName, outputFormat);
				
				if (Result != OK)
					ErrorMessage("Unable to write graph.\n");
			}
		}
		else if (strchr(GetAlgorithmChoices(), commandString[1]))
		{
			if (inputStr != NULL)
			{
				ErrorMessage("TestGraphFunctionality only supports applying chosen algorithm to graphs read from file at this time.\n");
				Result = NOTOK;
			}
			else
			{
				if (infileName == NULL)
				{
					ErrorMessage("No input file provided.\n");
					Result = NOTOK;
				}
				else
				{
					messageFormat = "Start testing all graphs in \"%.*s\".\n";
					charsAvailForFilename = (int) (MAXLINE - strlen(messageFormat));
					sprintf(messageContents, messageFormat, charsAvailForFilename, infileName);
					Message(messageContents);
					
					// Start the timer
					platform_GetTime(start);

					inputStr = ReadTextFileIntoString(infileName);

					strOrFileP testOutput = NULL;

					if (outfileName != NULL)
					{
						FILE *outputFileP = fopen(outfileName, "w");
						if (outputFileP == NULL)
						{
							charsAvailForFilename = (int) (MAXLINE - strlen(outfileName));
							messageFormat = "Unable to open file \"%.*s\" for output.\n";
							sprintf(messageContents, messageFormat, charsAvailForFilename, outfileName);
							ErrorMessage(messageContents);
							free(inputStr);
							inputStr = NULL;
							gp_Free(&theGraph);
						}
						
						testOutput = sf_New(outputFileP, NULL);
					}
					else
					{
						if ((*outputStr) == NULL)
							(*outputStr) = (char *) malloc(1 * sizeof(char));
						
						testOutput = sf_New(NULL, (*outputStr));
					}

					if (testOutput == NULL)
					{
						ErrorMessage("Unable to set up string-or-file container for test output.\n");
						free(inputStr);
						inputStr = NULL;
						gp_Free(&theGraph);
						sf_Free(&testOutput);
						return NOTOK;
					}

					char *finalSlash = strrchr(infileName, FILE_DELIMITER);
					char *infileBasename = finalSlash ? (finalSlash + 1) : infileName;

					char *headerFormat = "FILENAME=\"%s\" DURATION=\"%.3lf\"\n";
					char *headerStr = (char *) malloc(
														(
															strlen(headerFormat) +
															strlen(infileBasename) +
															strlen("-1.7976931348623158e+308") + // -DBL_MAX from float.h
															3
														) * sizeof(char));
					if (headerStr == NULL)
					{
						ErrorMessage("Unable allocate memory for output file header.\n");

						free(inputStr);
						inputStr = NULL;

						gp_Free(&theGraph);

						sf_Free(&testOutput);

						return NOTOK;
					}

					testAllStats stats;
					memset(&stats, 0, sizeof(testAllStats));
					
					char command = commandString[1];
					Result = testAllGraphs(theGraph, command, inputStr, &stats);

					// Stop the timer
					platform_GetTime(end);
					duration = platform_GetDuration(start, end);
					sprintf(messageContents, "\nDone testing all graphs (%.3lf seconds).\n", duration);
					Message(messageContents);

					sprintf(headerStr, headerFormat, infileBasename, duration);
					sf_fputs(headerStr, testOutput);
					free(headerStr);
					headerStr = NULL;

					char *resultsStr = (char *) malloc(
														(
															3 +_getNumCharsToReprInt(stats.numGraphsRead) +
															1 + _getNumCharsToReprInt(stats.numOK) +
															1 + _getNumCharsToReprInt(stats.numNONEMBEDDABLE)+
															1 + 8 + // either ERROR or SUCCESS, so the longer of which is 7 + 1 chars
															3
														) * sizeof(char));
					if (resultsStr == NULL)
					{
						ErrorMessage("Unable allocate memory for results string.\n");

						free(inputStr);
						inputStr = NULL;

						gp_Free(&theGraph);

						sf_Free(&testOutput);

						return NOTOK;
					}

					sprintf(resultsStr, "-%c %d %d %d %s\n",
										command, stats.numGraphsRead, stats.numOK, stats.numNONEMBEDDABLE, stats.errorFlag ? "ERROR" : "SUCCESS");
					sf_fputs(resultsStr, testOutput);

					free(resultsStr);
					resultsStr = NULL;

					if (inputStr != NULL)
					{
						free(inputStr);
						inputStr = NULL;
					}
					
					if (outputStr != NULL)
						(*outputStr) = sf_getTheStr(testOutput);
					else if (sf_getFile(testOutput) != NULL)
						fclose(sf_getFile(testOutput));

					sf_Free(&testOutput);
				}
			}
		}
		else
		{
			ErrorMessage("Invalid argument; only -(pdo234)|-t(gam) is allowed.\n");
			return NOTOK;
		}
	}
	else
	{
		ErrorMessage("Invalid argument; must start with '-'.\n");
		return NOTOK;
	}

	gp_Free(&theGraph);
	return Result;
}

/*
	TRANSFORM GRAPH
*/

int transformFile(graphP theGraph, char *infileName)
{
	if (infileName == NULL)
	{
		if ((infileName = ConstructInputFilename(infileName)) == NULL)
			return NOTOK;
	}

	return gp_Read(theGraph, infileName);
}

int transformString(graphP theGraph, char *inputStr)
{
	if (inputStr == NULL || strlen(inputStr) == 0)
	{
		ErrorMessage("Input string is null or empty.\n");
		return NOTOK;
	}
	
	return gp_ReadFromString(theGraph, inputStr);
}

/*
	TEST ALL GRAPHS IN .G6
*/

int testAllGraphs(graphP theGraph, char command, char *inputStr, testAllStatsP stats)
{
	int exitCode = OK;

	char *messageFormat = NULL;
	char messageContents[MAXLINE + 1];

	graphP copyOfOrigGraph = NULL;
	int embedFlags = GetEmbedFlags(command);
	int numGraphsRead = 0, numOK = 0, numNONEMBEDDABLE = 0, errorFlag = FALSE;

	G6ReadIterator *pG6ReadIterator = NULL;
	exitCode = allocateG6ReadIterator(&pG6ReadIterator, theGraph);

	if (exitCode != OK)
	{
		ErrorMessage("Unable to allocate G6ReadIterator.\n");
		return exitCode;
	}

	exitCode = beginG6ReadIterationFromG6String(pG6ReadIterator, inputStr);

	if (exitCode != OK)
	{
		ErrorMessage("Unable to begin .g6 read iteration.\n");
		freeG6ReadIterator(&pG6ReadIterator);
		return exitCode;
	}

	AttachAlgorithm(pG6ReadIterator->currGraph, command);

	copyOfOrigGraph = gp_New();
	if (copyOfOrigGraph == NULL)
	{
		ErrorMessage("Unable to allocate graph to store copy of original graph before embedding.\n");
		return NOTOK;
	}

	exitCode = gp_InitGraph(copyOfOrigGraph, pG6ReadIterator->graphOrder);
	if (exitCode != OK)
	{
		ErrorMessage("Unable to initialize graph datastructure to store copy of original graph before embedding.\n");
		gp_Free(&copyOfOrigGraph);
		freeG6ReadIterator(&pG6ReadIterator);
		return exitCode;
	}

	AttachAlgorithm(copyOfOrigGraph, command);

	while (true)
	{
		exitCode = readGraphUsingG6ReadIterator(pG6ReadIterator);

		if (exitCode != OK)
		{
			messageFormat = "Unable to read graph on line %d from .g6 read iterator.\n";
			sprintf(messageContents, messageFormat, pG6ReadIterator->numGraphsRead + 1);
			ErrorMessage(messageContents);
			break;
		}

		if (pG6ReadIterator->currGraph == NULL)
			break;
		
		gp_CopyGraph(copyOfOrigGraph, pG6ReadIterator->currGraph);

		exitCode = gp_Embed(pG6ReadIterator->currGraph, embedFlags);

		if (gp_TestEmbedResultIntegrity(pG6ReadIterator->currGraph, copyOfOrigGraph, exitCode) != exitCode)
			exitCode = NOTOK;
		
		if (exitCode == OK)
			numOK++;
		else if (exitCode == NONEMBEDDABLE)
			numNONEMBEDDABLE++;
		else
		{
			messageFormat = "Error applying algorithm '%c' to graph on line %d.\n";
			sprintf(messageContents, messageFormat, command, pG6ReadIterator->numGraphsRead + 1);
			ErrorMessage(messageContents);
			errorFlag = TRUE;
			break;
		}

		gp_ReinitializeGraph(copyOfOrigGraph);
	}

	stats->numGraphsRead = pG6ReadIterator->numGraphsRead;
	stats->numOK = numOK;
	stats->numNONEMBEDDABLE = numNONEMBEDDABLE;
	stats->errorFlag = errorFlag;

	if (endG6ReadIteration(pG6ReadIterator) != OK)
		ErrorMessage("Unable to end G6ReadIterator.\n");

	if (freeG6ReadIterator(&pG6ReadIterator) != OK)
		ErrorMessage("Unable to free G6ReadIterator.\n");
	
	return exitCode;
}

int _getNumCharsToReprInt(int theNum)
{
	int numCharsRequired = 1;

	while(theNum /= 10)
		numCharsRequired++;

	return numCharsRequired;
}
