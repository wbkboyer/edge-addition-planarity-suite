/*
Planarity-Related Graph Algorithms Project
Copyright (c) 1997-2009, John M. Boyer
All rights reserved. Includes a reference implementation of the following:

* John M. Boyer. "Simplified O(n) Algorithms for Planar Graph Embedding,
  Kuratowski Subgraph Isolation, and Related Problems". Ph.D. Dissertation,
  University of Victoria, 2001.

* John M. Boyer and Wendy J. Myrvold. "On the Cutting Edge: Simplified O(n)
  Planarity by Edge Addition". Journal of Graph Algorithms and Applications,
  Vol. 8, No. 3, pp. 241-273, 2004.

* John M. Boyer. "A New Method for Efficiently Generating Planar Graph
  Visibility Representations". In P. Eades and P. Healy, editors,
  Proceedings of the 13th International Conference on Graph Drawing 2005,
  Lecture Notes Comput. Sci., Volume 3843, pp. 508-511, Springer-Verlag, 2006.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice, this
  list of conditions and the following disclaimer in the documentation and/or
  other materials provided with the distribution.

* Neither the name of the Planarity-Related Graph Algorithms Project nor the names
  of its contributors may be used to endorse or promote products derived from this
  software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdlib.h>

#include "graphColorVertices.private.h"
#include "graphColorVertices.h"

/* Forward declarations of local functions */

void _ColorVertices_ClearStructures(ColorVerticesContext *context);
int  _ColorVertices_CreateStructures(ColorVerticesContext *context);
int  _ColorVertices_InitStructures(ColorVerticesContext *context);

/* Forward declarations of overloading functions */

void _ColorVertices_InitGraphNode(graphP theGraph, int I);
void _ColorVertices_InitVertexRec(graphP theGraph, int I);
void _InitDrawGraphNode(ColorVerticesContext *context, int I);
void _InitDrawVertexRec(ColorVerticesContext *context, int I);

int  _ColorVertices_InitGraph(graphP theGraph, int N);
void _ColorVertices_ReinitializeGraph(graphP theGraph);

int  _ColorVertices_ReadPostprocess(graphP theGraph, void *extraData, long extraDataSize);
int  _ColorVertices_WritePostprocess(graphP theGraph, void **pExtraData, long *pExtraDataSize);

void _ColorVertices_HideEdge(graphP theGraph, int e);
int  _ColorVertices_IdentifyVertices(graphP theGraph, int u, int v, int eBefore);
int  _ColorVertices_RestoreVertex(graphP theGraph);

/* Forward declarations of functions used by the extension system */

void *_ColorVertices_DupContext(void *pContext, void *theGraph);
void _ColorVertices_FreeContext(void *);

/****************************************************************************
 * COLORVERTICES_ID - the variable used to hold the integer identifier for this
 * extension, enabling this feature's extension context to be distinguished
 * from other features' extension contexts that may be attached to a graph.
 ****************************************************************************/

int COLORVERTICES_ID = 0;

/****************************************************************************
 gp_AttachColorVertices()

 This function adjusts the graph data structure to attach the planar graph
 drawing feature.

 To activate this feature during gp_Embed(), use EMBEDFLAGS_ColorVertices.

 This method may be called immediately after gp_New() in the case of
 invoking gp_Read().  For generating graphs, gp_InitGraph() can be invoked
 before or after this enabling method.  This method detects if the core
 graph has already been initialized, and if so, it will initialize the
 additional data structures specific to planar graph drawing.  This makes
 it possible to invoke gp_New() and gp_InitGraph() together, and then attach
 this feature only if it is requested at run-time.

 Returns OK for success, NOTOK for failure.
 ****************************************************************************/

int  gp_AttachColorVertices(graphP theGraph)
{
     ColorVerticesContext *context = NULL;

     // If the drawing feature has already been attached to the graph,
     // then there is no need to attach it again
     gp_FindExtension(theGraph, COLORVERTICES_ID, (void *)&context);
     if (context != NULL)
     {
         return OK;
     }

     // Allocate a new extension context
     context = (ColorVerticesContext *) malloc(sizeof(ColorVerticesContext));
     if (context == NULL)
     {
         return NOTOK;
     }

     // First, tell the context that it is not initialized
     context->initialized = 0;

     // Save a pointer to theGraph in the context
     context->theGraph = theGraph;

     // Put the overload functions into the context function table.
     // gp_AddExtension will overload the graph's functions with these, and
     // return the base function pointers in the context function table
     memset(&context->functions, 0, sizeof(graphFunctionTable));

     context->functions.fpInitGraph = _ColorVertices_InitGraph;
     context->functions.fpReinitializeGraph = _ColorVertices_ReinitializeGraph;

     context->functions.fpReadPostprocess = _ColorVertices_ReadPostprocess;
     context->functions.fpWritePostprocess = _ColorVertices_WritePostprocess;

     context->functions.fpHideEdge = _ColorVertices_HideEdge;
     context->functions.fpIdentifyVertices = _ColorVertices_IdentifyVertices;
     context->functions.fpRestoreVertex = _ColorVertices_RestoreVertex;

     _ColorVertices_ClearStructures(context);

     // Store the Draw context, including the data structure and the
     // function pointers, as an extension of the graph
     if (gp_AddExtension(theGraph, &COLORVERTICES_ID, (void *) context,
                         _ColorVertices_DupContext, _ColorVertices_FreeContext,
                         &context->functions) != OK)
     {
         _ColorVertices_FreeContext(context);
         return NOTOK;
     }

     // Create the Draw-specific structures if the size of the graph is known
     // Attach functions are typically invoked after gp_New(), but if a graph
     // extension must be attached before gp_Read(), then the attachment
     // also happens before gp_InitGraph() because gp_Read() invokes init only
     // after it reads the order N of the graph.  Hence, this attach call would
     // occur when N==0 in the case of gp_Read().
     // But if a feature is attached after gp_InitGraph(), then N > 0 and so we
     // need to create and initialize all the custom data structures
     if (theGraph->N > 0)
     {
         if (_ColorVertices_CreateStructures(context) != OK ||
             _ColorVertices_InitStructures(context) != OK)
         {
             _ColorVertices_FreeContext(context);
             return NOTOK;
         }
     }

     return OK;
}

/********************************************************************
 gp_DetachColorVertices()
 ********************************************************************/

int gp_DetachColorVertices(graphP theGraph)
{
    return gp_RemoveExtension(theGraph, COLORVERTICES_ID);
}

/********************************************************************
 _ColorVertices_ClearStructures()
 ********************************************************************/

void _ColorVertices_ClearStructures(ColorVerticesContext *context)
{
    if (!context->initialized)
    {
        // Before initialization, the pointers are stray, not NULL
        // Once NULL or allocated, free() or LCFree() can do the job
        context->degLists = NULL;
        context->degListHeads = NULL;
        context->color = NULL;

        context->initialized = 1;
    }
    else
    {
        if (context->degLists != NULL)
        {
            LCFree(&context->degLists);
        }
        if (context->degListHeads != NULL)
        {
            free(context->degListHeads);
            context->degListHeads = NULL;
        }
        if (context->color != NULL)
        {
            free(context->color);
            context->color = NULL;
        }
    }
}

/********************************************************************
 _ColorVertices_CreateStructures()
 Create uninitialized structures for the vertex and graph node
 levels, and initialized structures for the graph level
 ********************************************************************/
int  _ColorVertices_CreateStructures(ColorVerticesContext *context)
{
     int I, N = context->theGraph->N;
     //int Gsize = ((graphP) theGraph)->edgeOffset + ((graphP) theGraph)->arcCapacity;

     if (N <= 0)
         return NOTOK;

     if ((context->degLists = LCNew(N)) == NULL
    	 (context->degListHeads = (int *) malloc(N*sizeof(int))) == NULL ||
         (context->color = (int *) malloc(N*sizeof(int))) == NULL
        )
     {
         return NOTOK;
     }

     for (I=0; I<N; I++)
     {
    	 context->degListHeads[I] = NIL;
    	 context->color[I] = -1;
     }

     return OK;
}

/********************************************************************
 _ColorVertices_InitStructures()
 Intended to be called when N>0.
 Initializes vertex and graph node levels only.  Graph level is
 initialized by _CreateStructures().
 ********************************************************************/
int  _ColorVertices_InitStructures(ColorVerticesContext *context)
{
     return OK;
}

/********************************************************************
 _ColorVertices_DupContext()
 ********************************************************************/

void *_ColorVertices_DupContext(void *pContext, void *theGraph)
{
     ColorVerticesContext *context = (ColorVerticesContext *) pContext;
     ColorVerticesContext *newContext = (ColorVerticesContext *) malloc(sizeof(ColorVerticesContext));

     if (newContext != NULL)
     {
         int I, N = ((graphP) theGraph)->N;
         //int Gsize = ((graphP) theGraph)->edgeOffset + ((graphP) theGraph)->arcCapacity;

         *newContext = *context;

         newContext->theGraph = (graphP) theGraph;

         newContext->initialized = 0;
         _ColorVertices_ClearStructures(newContext);
         if (N > 0)
         {
             if (_ColorVertices_CreateStructures(newContext) != OK)
             {
                 _ColorVertices_FreeContext(newContext);
                 return NULL;
             }

             // Initialize custom data structures by copying
             LCCopy(newcontext->degLists, context->degLists);
             for (I=0; I<N; I++)
             {
            	 newcontext->degListHeads[I] = context->degListHeads[I];
            	 newcontext->color[I] = context->color[I];
             }
         }
     }

     return newContext;
}

/********************************************************************
 _ColorVertices_FreeContext()
 ********************************************************************/

void _ColorVertices_FreeContext(void *pContext)
{
     ColorVerticesContext *context = (ColorVerticesContext *) pContext;

     _ColorVertices_ClearStructures(context);
     free(pContext);
}

/********************************************************************
 ********************************************************************/

int  _ColorVertices_InitGraph(graphP theGraph, int N)
{
    ColorVerticesContext *context = NULL;
    gp_FindExtension(theGraph, COLORVERTICES_ID, (void *)&context);

    if (context == NULL)
    {
        return NOTOK;
    }
    else
    {
    	int I;

        theGraph->N = N;
        theGraph->edgeOffset = 2*N;
        if (theGraph->arcCapacity == 0)
        	theGraph->arcCapacity = 2*DEFAULT_EDGE_LIMIT*N;

        // Create custom structures, initialized at graph level,
        // uninitialized at vertex and graph node levels.
        if (_ColorVertices_CreateStructures(context) != OK)
        {
            return NOTOK;
        }

        // This call initializes the base graph structures, but it also
        // initializes the custom graphnode and vertex level structures
        // due to the overloads of InitGraphNode and InitVertexRec
        context->functions.fpInitGraph(theGraph, N);
    }

    return OK;
}

/********************************************************************
 ********************************************************************/

void _ColorVertices_ReinitializeGraph(graphP theGraph)
{
    ColorVerticesContext *context = NULL;
    gp_FindExtension(theGraph, COLORVERTICES_ID, (void *)&context);

    if (context != NULL)
    {
    	int I;

		// Some extensions attempt to unhook overloads of fpInitGraphNode() and
    	// fpInitVertexRec() before calling this method, when possible, but this
    	// extension doesn't overload those functions so we just reinitialize
		context->functions.fpReinitializeGraph(theGraph);

        // Graph level reintializations
        LCReset(context->degLists);
        for (I=0; I<N; I++)
        {
          	 context->degListHeads[I] = NIL;
          	 context->color[I] = -1;
        }
    }
}

/********************************************************************
 ********************************************************************/

int  _ColorVertices_ReadPostprocess(graphP theGraph, void *extraData, long extraDataSize)
{
    ColorVerticesContext *context = NULL;
    gp_FindExtension(theGraph, COLORVERTICES_ID, (void *)&context);

    if (context != NULL)
    {
        if (context->functions.fpReadPostprocess(theGraph, extraData, extraDataSize) != OK)
            return NOTOK;

        else if (extraData != NULL && extraDataSize > 0)
        {
            int I, tempInt;
            char line[64], tempChar;

            sprintf(line, "<%s>", COLORVERTICES_NAME);

            // Find the start of the data for this feature
            extraData = strstr(extraData, line);
            if (extraData == NULL)
                return NOTOK;

            // Advance past the start tag
            extraData = (void *) ((char *) extraData + strlen(line)+1);

            // Read the N lines of vertex information
            for (I = 0; I < theGraph->N; I++)
            {
                sprintf(line, "%d: %d\n", I, context->color[I]);
                sscanf(extraData, " %d%c %d", &tempInt, &tempChar, &context->color[I]);

                extraData = strchr(extraData, '\n') + 1;
            }
        }

        return OK;
    }

    return NOTOK;
}

/********************************************************************
 ********************************************************************/

int  _ColorVertices_WritePostprocess(graphP theGraph, void **pExtraData, long *pExtraDataSize)
{
    ColorVerticesContext *context = NULL;
    gp_FindExtension(theGraph, COLORVERTICES_ID, (void *)&context);

    if (context != NULL)
    {
        if (context->functions.fpWritePostprocess(theGraph, pExtraData, pExtraDataSize) != OK)
            return NOTOK;
        else
        {
            char line[32];
            int maxLineSize = 32, extraDataPos = 0, I;
            int GSize = theGraph->edgeOffset + theGraph->arcCapacity;
            char *extraData = (char *) malloc((GSize + 2) * maxLineSize * sizeof(char));

            if (extraData == NULL)
                return NOTOK;

            // Bit of an unlikely case, but for safety, a bigger maxLineSize
            // and line array size are needed to handle very large graphs
            if (theGraph->N > 2000000000)
            {
                free(extraData);
                return NOTOK;
            }

            sprintf(line, "<%s>\n", COLORVERTICES_NAME);
            strcpy(extraData+extraDataPos, line);
            extraDataPos += (int) strlen(line);

            for (I = 0; I < theGraph->N; I++)
            {
                sprintf(line, "%d: %d\n", I, context->color[I]);
                strcpy(extraData+extraDataPos, line);
                extraDataPos += (int) strlen(line);
            }

            sprintf(line, "</%s>\n", COLORVERTICES_NAME);
            strcpy(extraData+extraDataPos, line);
            extraDataPos += (int) strlen(line);

            *pExtraData = (void *) extraData;
            *pExtraDataSize = extraDataPos * sizeof(char);
        }

        return OK;
    }

    return NOTOK;
}

/********************************************************************
 _ColorVertices_HideEdge()

 An overload to perform the degree list updates for the edge endpoints.
 This routine also covers the work done by _HideVertex() and part of
 the work done by _ContractEdge() and _IdentifyVertices().
 ********************************************************************/

void _ColorVertices_HideEdge(graphP theGraph, int e)
{
    ColorVerticesContext *context = NULL;
    gp_FindExtension(theGraph, COLORVERTICES_ID, (void *)&context);

    if (context != NULL)
    {
    	// Get the endpoint vertices of the edge

    	// Get the degrees of the vertices

    	// Remove them from the degree lists that contain them

    	// Hide the edge
        context->functions.fpHideEdge(theGraph, e);

        // Decrement the degrees of the endpoint vertices

        // Add them to the new degree lists

        return OK;
    }

    return NOTOK;
}

/********************************************************************
 _ColorVertices_IdentifyVertices()

 An overload to perform degree list updates corresponding to the
 transfer of v's adjacency list into u.  Since the common edges are
 removed using _HideEdge(), this routine only accounts for those
 edges of v that create new neighbors for u.  These are indicated on
 the top of the stack after the base routine is called.
 This routine also does part of the work of _ContractEdge(), which
 simply combines _HideEdge() and _IdentifyVertices().
 ********************************************************************/

int _ColorVertices_IdentifyVertices(graphP theGraph, int u, int v, int eBefore)
{
    ColorVerticesContext *context = NULL;
    gp_FindExtension(theGraph, COLORVERTICES_ID, (void *)&context);

    if (context != NULL)
    {
        if (context->functions.fpIdentifyVertices(theGraph, u, v, eBefore) != OK)
            return NOTOK;

        return OK;
    }

    return NOTOK;
}

/********************************************************************
 _ColorVertices_RestoreVertex()

 An overload to color the vertex distinctly from its neighbors once
 it is restored.
 ********************************************************************/

int _ColorVertices_RestoreVertex(graphP theGraph)
{
    ColorVerticesContext *context = NULL;
    gp_FindExtension(theGraph, COLORVERTICES_ID, (void *)&context);

    if (context != NULL)
    {
    	// Read the stack to figure out which vertex is being restored

    	// Restore the vertex
        if (context->functions.fpRestoreVertex(theGraph) != OK)
            return NOTOK;

        // Give the restored vertex a color distinct from its neighbors

        return OK;
    }

    return NOTOK;
}