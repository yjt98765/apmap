/*
* Copyright (c) 2019, Delft University of Technology
*
* graph.c
*
* Functions related to graph struct
*/
#include "apmapbin.h"

/*************************************************************************/
/*! This function creates and initializes a graph_t data structure */
/*************************************************************************/
graph_t *CreateGraph(int nvtxs, int nedges, char extra)
{
  graph_t *graph = (graph_t *)malloc(sizeof(graph_t));
  int i;

  memset((void *)graph, 0, sizeof(graph_t));

  /* graph size constants */
  graph->nvtxs     = nvtxs;

  /* memory for the graph structure */
  graph->xadj      = (int*)malloc((nvtxs+1) * sizeof(int));
  graph->adjncy    = (int*)malloc(nedges * sizeof(int));

  if (extra) {
    graph->ste       = (unsigned*)malloc(8 * nvtxs * sizeof(unsigned));
    graph->start     = (char*)malloc(nvtxs);
    graph->report    = (char*)malloc(nvtxs);
    graph->where     = (int*)malloc(nvtxs * sizeof(int));
    graph->pos       = (int*)malloc(nvtxs * sizeof(int));
    graph->ext = (list_t**)malloc(nvtxs * sizeof(list_t*));
    graph->name = (char**)malloc(nvtxs * sizeof(char*));;
    for (i=0; i<nvtxs; i++) {
      graph->ext[i] = NULL;
    }

    graph->first = NULL;
    graph->current = NULL;
    graph->next = NULL;
    graph->from = NULL;
  }
  else {
    graph->ste = NULL;
    graph->start = NULL;
    graph->report = NULL;
    graph->where = NULL;
    graph->pos = NULL;
    graph->ext = NULL;
    graph->name = NULL;

    graph->first = (int*)malloc(nvtxs * sizeof(int));
    graph->current = (int*)malloc(nvtxs * sizeof(int));
    graph->next = (int*)malloc(nedges * sizeof(int));
    graph->from = (int*)malloc(nedges * sizeof(int));
  }

  return graph;
}

/*************************************************************************/
/*! This function deallocates any memory stored in a graph */
/*************************************************************************/
void FreeGraph(graph_t **r_graph, int nvtxs) 
{
  graph_t *graph = *r_graph;
  int i;

  /* free graph structure */

  if (graph->where) {
    for (i=0; i<nvtxs; i++) {
      FreeList(graph->ext[i]);
    }
    free(graph->ext);
  }

  free(graph->xadj);
  free(graph->adjncy);
  free(graph->where);
  free(graph->pos);
  free(graph->name);
  free(graph->ste);
  free(graph->start);
  free(graph->report);

  free(graph->first);
  free(graph->current);
  free(graph->next);
  free(graph->from);
  free(graph);

  *r_graph = NULL;
}

/*
* Generate an undirected graph based on the given directed graph
*/
void *GetUndiGraph(graph_t *digraph, graph_t *graph)
{
  int nvtxs = digraph->nvtxs;
  int *dixadj = digraph->xadj;
  int *diadjncy = digraph->adjncy;
  int dinedges = dixadj[nvtxs];

  int *from = graph->from;
  int *current = graph->current;
  int *first = graph->first;
  int *next = graph->next;
  int *xadj = graph->xadj;
  int *adjncy = graph->adjncy;
  int to, index;
  char exist;
  int i, j, k;

  for (i=0; i<nvtxs; i++) {
    first[i] = -1;
    current[i] = -1;
  }
  for (i=0; i<dinedges; i++) {
    next[i] = -1;
  }

  /* make the link */
  for (i=0; i<nvtxs; i++) {
    for (j=dixadj[i]; j<dixadj[i+1]; j++) {
      to = diadjncy[j];

      exist = 0;
      for (k=dixadj[to]; k<dixadj[to+1]; k++) {
        if (diadjncy[k] == i) {
          exist = 1;
          break;
        }
      }
      if (exist) {
        continue;
      }

      from[j] = i;
      if (first[to] == -1) {
        first[to] = j;
        current[to] = j;
      }
      else {
        next[current[to]] = j;
        current[to] = j;
      }
    }
  }

  graph->nvtxs = nvtxs;

  k = 0;
  for (i=0; i<nvtxs; i++) {
    xadj[i] = k;
    for (j=dixadj[i]; j<dixadj[i+1]; j++) {
      if (diadjncy[j] != i) {
        adjncy[k] = diadjncy[j];
        k++;
      }
    }
    for (index=first[i]; index!=-1; index=next[index]) {
      if (from[index] != i) {
        adjncy[k] = from[index];
        k++;
      }
    }
  }
  xadj[nvtxs] = k;
}

/*
* Count the # of the boundary nodes in every part.
* Also fill the ext pointer array of the graph struct
*/
void CountBoundaryNodes(graph_t* graph, int *nin, int *nout)
{
  int *xadj = graph->xadj;
  int *adjncy = graph->adjncy;
  int *where = graph->where;
  list_t **ext = graph->ext;
  int index, to_tile;
  int i, j;

  for (i=0; i<graph->npart; i++) {
    nin[i] = 0;
    nout[i] = 0;
  }

  for (i=0; i<graph->nvtxs; i++) {
    index = where[i];
    if (ext[i]) {
      EmptyList(ext[i]);
    }

    for (j=xadj[i]; j<xadj[i+1]; j++) {
      to_tile = where[adjncy[j]];
      if (to_tile != index) {
        if (!ext[i]) {
          ext[i] = CreateList(TILE_NUM - 1);
        }
        if (ListAddNew(ext[i], to_tile)) {
          nin[to_tile]++;
        }
      }
    }
    if (ext[i]) {
      if (ext[i]->size > 0) {
        nout[index]++;
      }
    }
  }
}

/*
* Adjust *graph->ext* and *graph->where* as if the *pos* part is duplicated for *num* times
*/
void InsertDuplicate(graph_t *graph, int pos, int num)
{
  list_t **ext = graph->ext;
  int* where = graph->where;
  int i, j;

  for (i=0; i<graph->nvtxs; i++) {
    if (where[i] > pos) {
      where[i] += num;
    }
    if (ext[i]) {
      for (j=0; j<ext[i]->size; j++) {
        if (ext[i]->value[j] > pos) {
          ext[i]->value[j] += num;
        }
      }
    }
  }
  graph->npart += num; 
}
