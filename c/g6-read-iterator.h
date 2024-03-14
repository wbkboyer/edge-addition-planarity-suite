/*
Copyright (c) 1997-2024, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#ifndef G6_READ_ITERATOR
#define G6_READ_ITERATOR

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdbool.h>

#include "graph.h"
#include "strOrFile.h"

typedef struct {
	// FILE *g6Infile;
	strOrFileP g6Input;
	bool fileOwnerFlag;
	int numGraphsRead;

	int graphOrder;
	int numCharsForGraphOrder;
	int numCharsForGraphEncoding;
	int currGraphBuffSize;
	char *currGraphBuff;

	graphP currGraph;
} G6ReadIterator;

int allocateG6ReadIterator(G6ReadIterator **, graphP);
bool _isG6ReadIteratorAllocated(G6ReadIterator *pG6ReadIterator);

int getNumGraphsRead(G6ReadIterator *, int *);
int getOrderOfGraphToRead(G6ReadIterator *, int *);
int getPointerToGraphReadIn(G6ReadIterator *, graphP *);

int beginG6ReadIterationFromG6FilePath(G6ReadIterator *, char *);
int beginG6ReadIterationFromG6FilePointer(G6ReadIterator *, FILE *);
int beginG6ReadIterationFromG6String(G6ReadIterator *pG6ReadIterator, char *g6InputStr);
int _beginG6ReadIteration(G6ReadIterator *pG6ReadIterator);
int _processAndCheckHeader(strOrFileP g6Input);
bool _firstCharIsValid(char, const int);
int _getGraphOrder(strOrFileP g6Input, int *);

int readGraphUsingG6ReadIterator(G6ReadIterator *);
int _checkGraphOrder(char *, int);
int _validateGraphEncoding(char *, const int, const int);
int _decodeGraph(char *, const int, const int, graphP);

int endG6ReadIteration(G6ReadIterator *);

int freeG6ReadIterator(G6ReadIterator **);

int _ReadGraphFromG6FilePath(graphP, char *);
int _ReadGraphFromG6FilePointer(graphP pGraphToRead, FILE *g6Infile);
int _ReadGraphFromG6String(graphP, char *);

#ifdef __cplusplus
}
#endif

#endif /* G6_READ_ITERATOR */
