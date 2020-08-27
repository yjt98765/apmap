/*
* Copyright (c) 2019, Delft University of Technology
*
* proto.h
*
* Function declarations
*/

#ifndef _PROTOBIN_H_
#define _PROTOBIN_H_

/* chip.c */
void ChipInit(chip_t *chip);
char MapGraphToChip(chip_t *chip, graph_t *graph, graph_t *ungraph);
void ResolveConstraint(tile_t* tile, graph_t *graph, int *remain);
void EmitChip(chip_t *chip, FILE* fp);
void FreeChip(chip_t *chip);

/* global.c */
void InitGlobal(global_t *global);
char MapGlobal(global_t global[GLOBAL_NUM], g4_t *g4, graph_t *graph, tile_t tile[TILE_NUM], int *curtile);
void CopyGlobal(global_t dest[GLOBAL_NUM], global_t src[GLOBAL_NUM]);
void CopyG4(g4_t *dest, g4_t *src);
void EmitGlobal(global_t global[GLOBAL_NUM], tile_t tile[TILE_NUM], FILE *fp);
void EmitG4(g4_t *g4, tile_t tile[TILE_NUM], FILE *fp);

/* graph.c */
graph_t *CreateGraph(int nvtxs, int nedges, char extra);
void FreeGraph(graph_t **r_graph, int nvtxs);
void *GetUndiGraph(graph_t *digraph, graph_t *graph);
void CountBoundaryNodes(graph_t* graph, int *nin, int *nout);
void InsertDuplicate(graph_t *graph, int pos, int num);

/* io.c */
automata_t *ReadMapFile(FILE *fpin, int *ngraph, char ***gfname);
void ReadGraphFile(graph_t *graph, const char *file, int nvtxs, int nedges);

/* list.c */
list_t *CreateList(int size);
void InitList(list_t *list, int size);
char ListAddNew(list_t *list, int num);
int ListAdd(list_t *list, int num);
int ListPop(list_t *list);
char ListChange(list_t *list, int origin, int current);
void ListCopy(list_t *dest, list_t *src);
int SwapByPosValue(int *list, int size, int value, int pos);
void EmptyList(list_t *list);
void FreeList(list_t *list);

/* partition.c */
void SetPartSizeTarget(float *tpwgts, int npart, int minsize);
int CalcBoundaryOverhead(int *nin, int *nout, int npart);
void WritePartitionToFile(const char* fname, int *part, int n);
char PartitionGraph(graph_t *ungraph, graph_t *graph, int remain, list_t *choice);
void RePartitionGraph(graph_t *ungraph, graph_t *graph, list_t *choice);

/* tile.c */
void ResetTile(tile_t *tile);
void InitTile(tile_t *tile);
void MapTile(tile_t *tile, graph_t *graph, int *remain);
void CopyGraphToTile(chip_t *chip, graph_t *graph, int curtile);
void CopySmallGraphToTile(tile_t *tile, graph_t *graph);
void EmitTile(tile_t *tile, FILE *fp);
void FreeTile(tile_t *tile);

/* util.c */
void errexit(char *f_str,...);

#endif
