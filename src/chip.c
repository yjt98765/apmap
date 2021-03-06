/*
* Copyright (c) 2019, Delft University of Technology
*
* chip.c
*
* Functions related to the chip struct
*/

#include "apmapbin.h"

/*
* Initiate a chip
*/
void ChipInit(chip_t *chip, char has_g4)
{
  int i, j;

  chip->curtile = 0;
  chip->remain = TILE_SIZE;

  // Init global switches
  for (i=0; i<GLOBAL_NUM; i++) {
    InitGlobal(&chip->global[i]);
  }
  if (has_g4) {
    chip->g4 = (g4_t*)malloc(sizeof(g4_t));
    for (j=0; j<TILE_NUM; j++) {
      for (i=0; i<8; i++) {
        chip->g4->src[j][i] = -1;
      }
    }
  }
  else {
    chip->g4 = NULL;
  }

  // Init tiles
  for (i=0; i<TILE_NUM; i++) {
    InitTile(&chip->tile[i], has_g4);
  }
}

/*
* Map a graph to a chip.
* return 1 if succeed
* return 0 if the chip does not have enough tiles
* return -1 if global switches do not have enough resources 
*/
char MapLargeGraph(chip_t *chip, graph_t *graph, char use)
{
  int curtile = chip->curtile;
  int npart = graph->npart;
  global_t gback[GLOBAL_NUM];
  g4_t g4back;
  tile_t tback;
  int remain = chip->remain;
  int oldnpart, oldtile;
  int i, j;

  if (remain!=TILE_SIZE && !use) {
    chip->curtile++;
  }
  curtile = chip->curtile;

  if (curtile + graph->cost > TILE_NUM) {
    return 0;
  }

  /* Backup in case of mapping failure */
  oldnpart = graph->npart;
  oldtile = curtile;
  CopyGlobal(gback, chip->global);
  if (chip->g4 != NULL) {
    CopyG4(&g4back, chip->g4);
  }

  ResolveConstraint(&chip->tile[curtile], graph, chip->g4 != NULL);
  if (MapGlobal(chip, graph, &curtile) == 1) {
    CopyGraphToTile(chip, graph, oldtile);
  }
  else { /* Roll back */
    graph->npart = oldnpart;
    CopyGlobal(chip->global, gback);
    if (chip->g4 != NULL) {
      CopyG4(chip->g4, &g4back);
    }
    if (remain == TILE_SIZE) {
      ResetTile(&chip->tile[curtile]);
    }
    for (i=curtile+1; i<curtile + graph->cost; i++) {
      ResetTile(&chip->tile[i]);
    }
    return -1;
  }
  return 1;
}

char MapGraphToChip(chip_t *chip, graph_t *graph, graph_t *ungraph, int no_opt)
{
  list_t parchoice;
  char succeed;
  char use;

  if (chip->curtile >= TILE_NUM) {
    return 0;
  }

  if (graph->nvtxs <= TILE_SIZE) {
    if (graph->nvtxs > chip->remain) {
      if (chip->curtile == TILE_NUM -1) {
        return 0;
      }
      else {
        chip->curtile++;
        chip->remain = TILE_SIZE;
      }
    }
    graph->npart = 1;
    CopySmallGraphToTile(&chip->tile[chip->curtile], graph);
    chip->remain -= graph->nvtxs;
    return 1;
  }

  if (graph->nvtxs > TILE_SIZE * (TILE_NUM - chip->curtile - 1) + chip->remain) {
    return 0;
  }
  InitList(&parchoice, 4);

  /* Generate a bidirection graph */
  GetUndiGraph(graph, ungraph);

  /* Partition the graph */
  use = PartitionGraph(ungraph, graph, chip->remain, &parchoice, chip->g4 != NULL, no_opt);

  succeed = MapLargeGraph(chip, graph, use);
  /* If failed, give up the remaining part on the current tile and start with a fresh tile. */
  if (succeed!=1 && chip->remain!=TILE_SIZE) {
    chip->curtile++;
    chip->remain = TILE_SIZE;
    use = PartitionGraph(ungraph, graph, chip->remain, &parchoice, chip->g4 != NULL, no_opt);
    succeed = MapLargeGraph(chip, graph, use);
  }
  while (parchoice.size>0 && succeed!=1) {
    RePartitionGraph(ungraph, graph, &parchoice, chip->g4 != NULL);
    succeed = MapLargeGraph(chip, graph, 0);
    if (succeed == 1) {
      break;
    }
  }

  free(parchoice.value);
  return succeed;
}

/*
* Emit the mapping result to files
*/
void EmitChip(chip_t *chip, FILE* fp)
{
  int curtile = chip->curtile;
  int remain = chip->remain;
  int i;

  if (curtile > 0) {
    EmitGlobal(chip->global, chip->tile, fp);
    if (chip->g4 != NULL) {
      EmitG4(chip->g4, chip->tile, fp);
    }
  }

  for (i=0; i<curtile; i++) {
    fprintf(fp, "\n--- Tile %d ---\n", i);
    EmitTile(&chip->tile[i], fp);
  }
  if (chip->remain < TILE_SIZE) {
    fprintf(fp, "\n--- Tile %d ---\n", curtile);
    EmitTile(&chip->tile[curtile], fp);
  }
  fprintf(fp, "\n");
}

/*
* Free the resource in chip struct
*/
void FreeChip(chip_t *chip)
{
  int i;

  for (i=0; i<TILE_NUM; i++) {
    FreeTile(&chip->tile[i]);
  }
  free(chip->g4);
}
