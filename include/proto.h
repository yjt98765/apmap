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
void ChipInit(chip_t *chip, char has_g4);
char MapGraphToChip(chip_t *chip, graph_t *graph, graph_t *ungraph, int no_opt);
void EmitChip(chip_t *chip, FILE* fp);
void FreeChip(chip_t *chip);

/* global.c */
void InitGlobal(global_t *global);
char MapGlobal(chip_t *chip, graph_t *graph, int *curtile);
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

/* parser.c */
automata_t *ReadMapFile(FILE *fpin, int *ngraph);
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
void PrintList(list_t *list);
void EmptyList(list_t *list);
void FreeList(list_t *list);

/* partition.c */
void SetPartSizeTarget(float *tpwgts, int npart, int minsize);
int CalcBoundaryOverhead(int *nin, int *nout, int npart, char has_g4);
void WritePartitionToFile(const char* fname, int *part, int n);
char PartitionGraph(graph_t *ungraph, graph_t *graph, int remain, list_t *choice, int has_g4, int no_opt);
void RePartitionGraph(graph_t *ungraph, graph_t *graph, list_t *choice, char has_g4);

/* tile.c */
void ResetTile(tile_t *tile);
void InitTile(tile_t *tile, char has_g4);
void ResolveConstraint(tile_t* tile, graph_t *graph, char has_g4);
void MapTile(tile_t *tile, graph_t *graph, int *remain);
void CopyGraphToTile(chip_t *chip, graph_t *graph, int curtile);
void CopySmallGraphToTile(tile_t *tile, graph_t *graph);
void EmitTile(tile_t *tile, FILE *fp);
void FreeTile(tile_t *tile);

/* util.c */
void errexit(char *f_str,...);

#endif
