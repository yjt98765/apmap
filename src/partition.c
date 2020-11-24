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
  if (size[npart-1] == 0) {
    graph->npart--;
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
char PartitionGraph(graph_t *ungraph, graph_t *graph, int headsize, list_t *choice, char has_g4)
{
  int nvtxs = graph->nvtxs;
  int *nin = (int*)malloc(TILE_NUM * sizeof(int));
  int *nout = (int*)malloc(TILE_NUM * sizeof(int));
  float *tpwgts;
  int cost, initcost, mincost, minpart, tailsize, mintail;
  char valid = 0;
  int i, j, k;

  int *xadj = graph->xadj;
  int *uxadj = ungraph->xadj;
  int *adjncy = graph->adjncy;
  int *where = graph->where;
  char *report = graph->report;
  int *fromi, *state;
  int cur, curstate, ninper;
  list_t **from, *out, *stack, *lfrom;
  char *visited, use;

  EmptyList(choice);
  ungraph->npart = nvtxs / TILE_SIZE;
  while (valid != 1) {
    ungraph->npart++;
    valid = MetisWrapper(ungraph, NULL, TILE_SIZE, graph->where);
  };

  graph->npart = ungraph->npart;
  CountBoundaryNodes(graph, nin, nout);
  initcost = CalcBoundaryOverhead(nin, nout, ungraph->npart, has_g4);
  mincost = graph->npart + initcost;
  minpart = graph->npart;
  ListAdd(choice, ungraph->npart);
  ListAdd(choice, TILE_SIZE);
  for (i=0; i<initcost; i++) {
    ungraph->npart++;
    valid = MetisWrapper(ungraph, NULL, TILE_SIZE, graph->where);
    if (valid == 1) {
      graph->npart = ungraph->npart;
      CountBoundaryNodes(graph, nin, nout);
      cost = CalcBoundaryOverhead(nin, nout, ungraph->npart, has_g4) + graph->npart;
      if (cost < mincost) {
        mincost = cost;
        minpart = graph->npart;
        ListAdd(choice, minpart);
        ListAdd(choice, TILE_SIZE);
      }
    }
  }

  tpwgts = (float*)malloc((minpart+1) * sizeof(float));
  tailsize = nvtxs - (minpart - 1) * TILE_SIZE;
  tailsize = (tailsize<=0)? minpart/2: tailsize;
  mintail = TILE_SIZE;

  while (tailsize < TILE_SIZE) {
    ungraph->npart = minpart;
    SetPartSizeTarget(tpwgts, minpart, tailsize);
    valid = MetisWrapper(ungraph, tpwgts, TILE_SIZE, graph->where);
    if (valid == 1) {
      graph->npart = ungraph->npart;
      CountBoundaryNodes(graph, nin, nout);
      cost = CalcBoundaryOverhead(nin, nout, ungraph->npart, has_g4) + ungraph->npart;
      if (cost <= mincost) {
        mincost = cost;
        mintail = tailsize;
        ListAdd(choice, minpart);
        ListAdd(choice, tailsize);
        break;
      }
    }
    tailsize += minpart;
  }

  if (headsize >= TILE_SIZE) {
    tailsize = ListPop(choice);
    ungraph->npart = ListPop(choice);
    SetPartSizeTarget(tpwgts, ungraph->npart, tailsize);
    assert(MetisWrapper(ungraph, tpwgts, TILE_SIZE, graph->where)==1);
    graph->npart = ungraph->npart;
    CountBoundaryNodes(graph, nin, nout);
    graph->cost = CalcBoundaryOverhead(nin+1, nout+1, ungraph->npart-1, has_g4) + ungraph->npart;
    free(nin);
    free(nout);
    free(tpwgts);
    return 1;
  }

  //considering the remainning part
  if (mincost < minpart) {
    tailsize = TILE_SIZE - headsize;
  }
  else {
    tailsize = mintail - headsize;
    if (tailsize <= 0) {
      tailsize += TILE_SIZE;
    }
    else {
      minpart++;
    }
  }
  valid = 0;
  while (valid != 1) {
    tailsize += minpart;
    if (tailsize >= TILE_SIZE) {
      tailsize = ListPop(choice);
      ungraph->npart = ListPop(choice);
      graph->cost = mincost;
      SetPartSizeTarget(tpwgts, ungraph->npart, tailsize);
      assert(MetisWrapper(ungraph, tpwgts, TILE_SIZE, graph->where)==1);
      use = 0;
      break;
    }
    
    ungraph->npart = minpart;
    SetPartSize(tpwgts, minpart, headsize, tailsize);
    valid = MetisWrapper(ungraph, tpwgts, headsize, graph->where);
    if (valid == -1) {
      headsize -= minpart;
    }
  }

  graph->npart = ungraph->npart;
  if (valid == 1) {
    use = 1;
    CountBoundaryNodes(graph, nin, nout);
    graph->cost = CalcBoundaryOverhead(nin+1, nout+1, ungraph->npart-1, has_g4) + ungraph->npart;
  }
  free(nin);
  free(nout);
  free(tpwgts);
  return use;
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
  int i;

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

