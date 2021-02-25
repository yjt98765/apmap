/*
* Copyright (c) 2019, Delft University of Technology
*
* partition.c
*
* Functions related to graph partition
*/

#include "apmapbin.h"

/*
* Decide the portion of parts according to the size of the remaining tile
*/
void SetPartSizeTarget(float *tpwgts, int npart, int minsize)
{
  int total = (npart - 1) * TILE_SIZE + minsize;
  float normal = (float)TILE_SIZE / total;
  int i;

  for (i=0; i<npart-1; i++) {
    tpwgts[i] = normal;
  }
  tpwgts[npart-1] = (float)minsize / total;
}

/*
* Decide the portion of parts according to headsize and tailsize
*/
void SetPartSize(float *tpwgts, int npart, int headsize, int tailsize)
{
  int total = (npart - 2) * TILE_SIZE + headsize + tailsize;
  int i;

  tpwgts[0] = (float)headsize / total;
  for (i=1; i<npart-1; i++) {
    tpwgts[i] = (float)TILE_SIZE / total;
  }
  tpwgts[npart-1] = (float)tailsize / total;
}

/*
* Calculate the total overhead caused by input/output constraints
*/
int CalcBoundaryOverhead(int *nin, int *nout, int npart, char has_g4)
{
  int in, out;
  int overhead = 0;
  int i;
  int max_inout = has_g4? GLOBAL_NUM * 2 + 8: GLOBAL_NUM * 2;

  for (i=0; i<npart; i++) {
    in = (nin[i] + max_inout - 1) / max_inout;
    in = in < 1? 1: in;
    out = (nout[i] + max_inout - 1) / max_inout;
    out = out < 1? 1: out;
    overhead += in * out - 1;
  }
  return overhead;
}

/*
* Write partiton results to a file. Only used for debugging
*/
void WritePartitionToFile(const char* fname, int *part, int n)
{
  FILE *fp = fopen(fname, "w");
  int i;

  for (i=0; i<n; i++) {
    fprintf(fp, "%d\n", part[i]);
  }
  fclose(fp);
}

/*
* Call Metis for partitioning a graph.
* Returns whether a partition satisfies the tile size constraint 
*/
char MetisWrapper(graph_t *graph, float *tpwgts, int headsize, int *part)
{
  int options[METIS_NOPTIONS];
  int npart = graph->npart;
  int nvtxs = graph->nvtxs;
  int ncon = 1;
  int status, objval;
  int max = 0;
  int *size;
  char result;
  int i;

  options[METIS_OPTION_PTYPE]   = METIS_PTYPE_KWAY;
  options[METIS_OPTION_OBJTYPE] = METIS_OBJTYPE_CUT;
  options[METIS_OPTION_CTYPE]   = METIS_CTYPE_SHEM;
  options[METIS_OPTION_IPTYPE]  = METIS_IPTYPE_METISRB;
  options[METIS_OPTION_RTYPE]   = METIS_RTYPE_GREEDY;
  options[METIS_OPTION_NCUTS] = 1;
  options[METIS_OPTION_NSEPS] = 1;
  options[METIS_OPTION_NUMBERING] = 0;
  options[METIS_OPTION_NITER] = 10;
  options[METIS_OPTION_SEED] = -1;
  options[METIS_OPTION_MINCONN] = 0;
  options[METIS_OPTION_NO2HOP]  = 0;
  options[METIS_OPTION_CONTIG]  = 0;
  options[METIS_OPTION_COMPRESS] = 0;
  options[METIS_OPTION_CCORDER] = 0;
  options[METIS_OPTION_PFACTOR] = 0;
  options[METIS_OPTION_UFACTOR] = -1;
  options[METIS_OPTION_DBGLVL]  = 0;

  status = METIS_PartGraphKway(&nvtxs, &ncon, graph->xadj, 
                   graph->adjncy, NULL, NULL, NULL, 
                   &npart, tpwgts, NULL, options, 
                   &objval, part);
  if (status != METIS_OK) {
    errexit("Metis error: %d\n", status);
  }
  
  /* Validate the partitioning results */
  size = (int*)malloc(npart * sizeof(int));
  for (i=0; i<npart; i++) {
    size[i] = 0;
  }
  for (i=0; i<nvtxs; i++) {
    size[part[i]]++;
  }
  for (i=0; i<npart; i++) {
    max = (size[i]>max)? size[i]: max;
  }

  if (size[0] > headsize) {
    result = -1;
  }
  else if (max > TILE_SIZE) {
    result = 0;
  }
  else {
    result = 1;
  }
  free(size);
  return result;
}

/*
* Partition a given graph
*/
char PartitionGraph(graph_t *ungraph, graph_t *graph, int headsize, list_t *choice, int has_g4, int no_opt)
{
  int nvtxs = graph->nvtxs;
  int *nin = (int*)malloc(TILE_NUM * sizeof(int));
  int *nout = (int*)malloc(TILE_NUM * sizeof(int));
  float *tpwgts = (float*)malloc(TILE_NUM * sizeof(float));
  int cost, initcost, mincost, minpart, tailsize, mintail;
  int valid = 0;

  EmptyList(choice);
  ungraph->npart = (nvtxs - headsize - 1) / TILE_SIZE + 2;
  tailsize = (nvtxs - headsize - 1) % TILE_SIZE;
  mincost = TILE_NUM + 1;
  while (ungraph->npart <= mincost && (!no_opt || valid != 1)) {
    /* Increase the tailsize in each try */
    tailsize += 1;
    if (tailsize > TILE_SIZE) {
      tailsize -= TILE_SIZE;
      ungraph->npart++;
      if (ungraph->npart > TILE_NUM) {
        errexit("Cannot partition graph with %d states\n", nvtxs);
      }
    }

    SetPartSize(tpwgts, ungraph->npart, headsize, tailsize);
    valid = MetisWrapper(ungraph, tpwgts, headsize, graph->where);
    if (valid == 1) {
      graph->cost = graph->npart = ungraph->npart;
      CountBoundaryNodes(graph, nin, nout);
      cost = CalcBoundaryOverhead(nin, nout, ungraph->npart, has_g4);
      valid = cost + 1;
      if (!no_opt && ungraph->npart + cost < mincost) {
        if (minpart < TILE_NUM) {
          ListAdd(choice, minpart);
          ListAdd(choice, mintail);
        }
        minpart = ungraph->npart;
        mincost = minpart + cost;
        mintail = tailsize;
      }
      else if (ungraph->npart+cost == mincost) {
        ListAdd(choice, ungraph->npart);
        ListAdd(choice, tailsize);
      }
    }
  }

  /* Make sure the result is optimal */
  if (!no_opt && (ungraph->npart!=minpart || tailsize!=mintail)) {
    SetPartSize(tpwgts, minpart, headsize, mintail);
    ungraph->npart = minpart;
    MetisWrapper(ungraph, tpwgts, headsize, graph->where);
    graph->npart = minpart;
    graph->cost = mincost;
    CountBoundaryNodes(graph, nin, nout);
  }
  free(nin);
  free(nout);
  free(tpwgts);
  return 1;
}

/*
* Partition a graph using given parameters
*/
void RePartitionGraph(graph_t *ungraph, graph_t *graph, list_t *choice, char has_g4)
{
  int tail = ListPop(choice);
  int npart = ListPop(choice);
  float *tpwgts = NULL;
  int *nin = (int*)malloc(TILE_NUM * sizeof(int));
  int *nout = (int*)malloc(TILE_NUM * sizeof(int));

  ungraph->npart = npart;
  if (tail < TILE_SIZE) {
    tpwgts = (float*)malloc(npart * sizeof(float));
    SetPartSizeTarget(tpwgts, npart, tail);
  }
  assert(MetisWrapper(ungraph, tpwgts, TILE_SIZE, graph->where));
  CountBoundaryNodes(graph, nin, nout);
  graph->npart = ungraph->npart;
  graph->cost = CalcBoundaryOverhead(nin, nout, ungraph->npart, has_g4) + graph->npart;
  free(nin);
  free(nout);

  free(tpwgts);
}

