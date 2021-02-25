/*
* Copyright (c) 2019, Delft University of Technology
*
* tile.c
*
* Functions related to the tile struct
*/
#include "apmapbin.h"

/*
* Clean up the current configuration of a tile
*/
void ResetTile(tile_t *tile)
{
  int i;

  for (i=0; i<TILE_SIZE; i++) {
    tile->state[i] = -1;
    tile->sname[i] = NULL;
    tile->xadj[i] = 0;
  }
  tile->nstate = 0;
  for (i=TILE_SIZE; i<=TILE_SIZE + MAX_IN; i++) {
    tile->xadj[i] = 0;
  }
  free(tile->adjncy);
  tile->adjncy = NULL;
  EmptyList(&tile->out);

  memset(tile->start, 0, TILE_SIZE);
  memset(tile->report, 0, TILE_SIZE);
  for (i=0; i<GLOBAL_NUM; i++) {
    tile->global[i][0] = -1;
    tile->global[i][1] = -1;
  }
  if (tile->g4 != NULL) {
    for (i=0; i<8; i++) {
      tile->g4[i] = -1;
    }
  }
  if (tile->ghost) {
    EmptyList(tile->ghost);
  }
  tile->duplicated = -1;
}

/*
* Tile initialization
*/
void InitTile(tile_t *tile, char has_g4)
{
  InitList(&tile->out, MAX_OUT);
  tile->ghost = NULL;
  tile->adjncy = NULL;
  tile->g4 = has_g4? (int*)malloc(sizeof(int) * 8): NULL;
  ResetTile(tile);
}

void CopyTile(tile_t *dest, tile_t *src)
{
  int i;
  ListCopy(&dest->out, &src->out);
}

void MoveStateFields(tile_t *tile, int from, int to)
{
  int i;
  tile->sname[to] = tile->sname[from];
  tile->start[to] = tile->start[from];
  tile->report[to] = tile->report[from];
  for (i=0; i<8; i++) {
    tile->ste[to][i] = tile->ste[from][i];
  }
}

/*
* Resolve constraint conflicts
*/
void ResolveConstraint(tile_t *tile, graph_t *graph, char has_g4)
{
  list_t **ext = graph->ext;
  int index, nadd, quotient, remainder;
  list_t *nin = (list_t*)malloc(TILE_NUM * sizeof(list_t));
  int realj, start;
  int i, j, k;
  int max_inout = has_g4? GLOBAL_NUM * 2 + 8: GLOBAL_NUM * 2;

  for (i=0; i<TILE_NUM; i++) {
    InitList(&nin[i], max_inout);
  }

  if (tile[0].nstate != 0) {
    EmptyList(&tile[0].out);
    for (i=0; i<GLOBAL_NUM; i++) {
      if (tile[0].global[i][0] != -1) {
        tile[0].global[i][0] = -2;
        if (tile[0].global[i][1] != -1) {
          tile[0].global[i][1] = -2;
        }
      }
    }
    if (tile[0].g4 != NULL) {
      for (i=0; i<8; i++) {
        if (tile[0].g4[i] != -1) {
          tile[0].g4[i] = -2;
        }
      }
    }
  }  

  for (i=0; i<graph->nvtxs; i++) {
    index = graph->where[i];

    if (ext[i] && ext[i]->size > 0) {
      ListAdd(&tile[index].out, i);
      for (j=0; j<ext[i]->size; j++) {
        ListAdd(&nin[ext[i]->value[j]], i);
      }
    }
  }

  // deal with outgoing constraint conflict
  start = (tile[0].nstate != 0)? 1: 0;
  for (i=start; i<graph->npart; i++) {
    nadd = (tile[i].out.size - 1) / max_inout;
    if (nadd <= 0) {
      continue;
    }
    printf("tile[%d] has %d output states. It is copied %d times\n", i, tile[i].out.size, nadd);
    
    InsertDuplicate(graph, i, nadd);
    for (j=graph->npart-1; j>i+nadd; j--) {
      ListCopy(&tile[j].out, &tile[j-nadd].out);
      ListCopy(&nin[j], &nin[j-nadd]);
    }
    for (j=i+nadd; j>i; j--) {
      ListCopy(&nin[j], &nin[i]);
      for (k=0; k<nin[i].size; k++) {
        if (!ListAddNew(ext[nin[i].value[k]], j)) {
          PrintList(ext[nin[i].value[k]]);
          errexit("Tile %d is already in the destination of state %d!\n", j, nin[i].value[k]);
        }
      }
    }

    quotient = tile[i].out.size / (nadd + 1);
    remainder = tile[i].out.size % (nadd + 1);
    for (j=1; j<remainder; j++) {
      realj = i + j;
      tile[realj].duplicated = i;

      tile[realj].out.size = quotient + 1;
      for (k=0; k<tile[realj].out.size; k++) {
        tile[realj].out.value[k] = tile[i].out.value[tile[i].out.size-1-k];
      }
      tile[i].out.size -= quotient + 1;
    }
    for (j=(remainder>0)? remainder: 1; j<=nadd; j++) {
      realj = i + j;
      tile[realj].duplicated = i;

      tile[realj].out.size = quotient;
      for (k=0; k<tile[realj].out.size; k++) {
        tile[realj].out.value[k] = tile[i].out.value[tile[i].out.size-1-k];
      }
      tile[i].out.size -= quotient;
    }
    i += nadd;
  }

  // deal with incoming constraint conflict
  for (i=start; i<graph->npart; i++) {
    nadd = (nin[i].size - 1) / max_inout;
    if (nadd <= 0) {
      continue;
    }

    tile[i].ghost = CreateList(nadd);
    InsertDuplicate(graph, i, nadd);
    for (j=graph->npart-1; j>i+nadd; j--) {
      if (tile[j - nadd].duplicated > i) {
        tile[j].duplicated = tile[j-nadd].duplicated + nadd;
      }
      else {
        tile[j].duplicated = tile[j-nadd].duplicated;
      }
      ListCopy(&tile[j].out, &tile[j-nadd].out);
      ListCopy(&nin[j], &nin[j-nadd]);
    }
    for (j=i+nadd; j>i; j--) {
      ListCopy(&tile[j].out, &tile[i].out);
    }

    printf("Create %d ghost tiles for tile %d\n", nadd, i);
    quotient = nin[i].size / (nadd + 1);
    remainder = nin[i].size % (nadd + 1);
    for (j=1; j<remainder; j++) {
      realj = i + j;
      tile[realj].duplicated = i;

      ListAdd(tile[i].ghost, realj);
      for (k=nin[i].size-1; k>nin[i].size-quotient-2; k--) {
        if (!ListChange(ext[nin[i].value[k]], i, realj)) {
          PrintList(ext[nin[i].value[k]]);
          errexit("Tile %d is not the destination of state %d!\n", i, nin[i].value[k]);
        }
      }
      nin[i].size -= quotient + 1;
    }

    for (j=(remainder>0)? remainder: 1; j<=nadd; j++) {
      realj = i + j;
      tile[realj].duplicated = i;

      ListAdd(tile[i].ghost, realj);
      for (k=nin[i].size-1; k>nin[i].size-quotient-1; k--) {
        if (!ListChange(ext[nin[i].value[k]], i, realj)) {
          PrintList(ext[nin[i].value[k]]);
          errexit("Tile %d is not the destination of State %d!\n", i, nin[i].value[k]);
        }
      }
      nin[i].size -= quotient;
    }
    i += nadd;
  }

  for (i=0; i<TILE_NUM; i++) {
    free(nin[i].value);
  }
  free(nin);
}

/*
* Copy graph information, such as state names and local transistion, to tiles
*/
void CopyGraphToTile(chip_t *chip, graph_t *graph, int fromtile)
{
  tile_t *tile = chip->tile;
  global_t *global = chip->global;
  g4_t *g4 = chip->g4;
  int *gxadj = graph->xadj;
  int *gadjncy = graph->adjncy;
  int *pos = graph->pos;
  int *where = graph->where;
  int *txadj, *tadjncy, *state, nedge, to, curtile, index, gsrc, ste, src;
  int i, j, k, l;
  char bitmap[TILE_SIZE + MAX_IN][TILE_SIZE];
  tile_t *tfirst = &tile[fromtile];
  char remain = 0;
  int start = fromtile;
  int end = start;

  if (tfirst->nstate != 0) {
    remain = 1;
    memset(bitmap, 0, (TILE_NUM + MAX_IN) * TILE_NUM);
    txadj = tfirst->xadj;
    tadjncy = tfirst->adjncy;
    for (i=0; i<TILE_SIZE+MAX_IN; i++) {
      for (j=txadj[i]; j<txadj[i+1]; j++) {
        bitmap[i][j] = 1;
      }
    }
    for (i=0; i<TILE_SIZE; i++) {
      if (tfirst->state[i] != -1) {
        tfirst->state[i] = -2;
      }
      else {
        tfirst->nstate = i;
        break;
      }
    }
  }

  /* Copy state numbers to tiles */
  for (i=0; i<graph->nvtxs; i++) {
    index = where[i] + fromtile;
    if (index > end) {
      end = index;
    }
    where[i] = index;
    tile[index].state[tile[index].nstate++] = i;
  }

  if (remain) {
    start = fromtile + 1;
    state = tfirst->state;
    /* Adjust postions of outgoing states */
    for (j=0; j<GLOBAL_NUM; j++) {
      for (k=0; k<2; k++) {
        if (tfirst->global[j][k] > -1) {
          to = SwapByPosValue(state, TILE_SIZE, tile[fromtile].global[j][k], 2 * j + k);
          MoveStateFields(tfirst, 2 * j + k, to);
          memcpy(&bitmap[to], &bitmap[2*j+k], TILE_SIZE);
          for (i=0; i<TILE_SIZE+MAX_IN; i++) {
            bitmap[i][to] = bitmap[i][2*j+k];
            bitmap[i][2*j+k] = 0;
          }
        }
      }
    }
    if (tfirst->g4 != NULL) {
      for (j=0; j<8; j++) {
        if (tfirst->g4[j] > -1) {
          to = SwapByPosValue(state, TILE_SIZE, tile[fromtile].g4[j], 2 * GLOBAL_NUM + j);
          MoveStateFields(tfirst, 2 * GLOBAL_NUM + j, to);
          memcpy(&bitmap[to], &bitmap[2*GLOBAL_NUM+j], TILE_SIZE);
          for (i=0; i<TILE_SIZE+MAX_IN; i++) {
            bitmap[i][to] = bitmap[i][2*GLOBAL_NUM+j];
            bitmap[i][2*GLOBAL_NUM+j] = 0;
          }
        }
      }
    }

    /* Copy the fields */
    for (j=0; j<tfirst->nstate; j++) {
      if (state[j] > -1) {
        tfirst->sname[j] = graph->name[state[j]];
        tfirst->start[j] = graph->start[state[j]];
        tfirst->report[j] = graph->report[state[j]];
        for (k=0; k<8; k++) {
          tfirst->ste[j][k] = graph->ste[8 * state[j] + k];
        }

        for (k=gxadj[state[j]]; k<gxadj[state[j]+1]; k++) {
          to = graph->adjncy[k];
          if (where[to] == fromtile) {
            bitmap[j][pos[to]] = 1;
          }
        }
      }
    }

    /* Add connections from Global Switches */
    for (j=0; j<GLOBAL_NUM; j++) {
      for (k=0; k<2; k++) {
        gsrc = global[j].src[fromtile][k];
        if (gsrc > -1) {
          ste = tile[gsrc / 2].global[j][gsrc % 2];
          for (l=gxadj[ste]; l<gxadj[ste+1]; l++) {
            to = gadjncy[l];
            if (where[to] == fromtile) {
              bitmap[TILE_SIZE+j*2+k][pos[to]] = 1;
            }
          }
        }
      }
    }
    if (g4 != NULL) {
      for (k=0; k<8; k++) {
        gsrc = g4->src[fromtile][k];
        if (gsrc > -1) {
          ste = tile[gsrc / 8].g4[gsrc % 8];
          for (l=gxadj[ste]; l<gxadj[ste+1]; l++) {
            to = gadjncy[l];
            if (where[to] == fromtile) {
              bitmap[TILE_SIZE + GLOBAL_NUM * 2 + k][pos[to]] = 1;
            }
          }
        }
      }
    }

    // Set local xadj
    txadj = tile[fromtile].xadj;
    txadj[0] = 0;
    for (i=0; i<TILE_SIZE+MAX_IN; i++) {
      txadj[i+1] = txadj[i];
      for (j=0; j<TILE_SIZE; j++) {
        if (bitmap[i][j]) {
          txadj[i+1]++;
        }
      }
    }

    // Set local adjncy
    free(tile[fromtile].adjncy);
    tile[fromtile].adjncy = (int*)malloc(txadj[TILE_SIZE+MAX_IN] * sizeof(int));
    index = 0;
    for (i=0; i<TILE_SIZE+MAX_IN; i++) {
      for (j=0; j<TILE_SIZE; j++) {
        if (bitmap[i][j]) {
          tile[fromtile].adjncy[index++] = j;
        }
      }
    }
  }

  for (i=start; i<=end; i++) {
    state = tile[i].state;

    /* duplicate a state as the result of constraint confilict resolving */
    src = tile[i].duplicated;
    if (src != -1) {
      printf("tile[%d] duplicates tile[%d]\n", i, src);
      tile[i].nstate = tile[src].nstate;
      for (j=0; j<tile[src].nstate; j++) {
        tile[i].state[j] = tile[src].state[j];
      }
    }
    if (tile[i].nstate<=0 || tile[i].nstate>TILE_SIZE) {
      remain = graph->nvtxs;
      for (i=start; i<=end; i++) {
        remain -= tile[i].nstate;
        printf("Tile[%d].nstate=%d src=%d ", i, tile[i].nstate, tile[i].duplicated);
      }
      printf("\n%d states are missing\n", remain);
      printf("tile.out:");
      for (i=start; i<=end; i++) {
        printf("%d ", tile[i].out.size);
      }
      exit(-1);
    }
    assert(tile[i].nstate);
    assert(tile[i].nstate <= TILE_SIZE);

    /* Adjust postions of outgoing states */
    for (j=0; j<GLOBAL_NUM; j++) {
      if (tile[i].global[j][0] != -1) {
        SwapByPosValue(state, TILE_SIZE, tile[i].global[j][0], 2 * j);
        if (tile[i].global[j][1] != -1) {
          SwapByPosValue(state, TILE_SIZE, tile[i].global[j][1], 2 * j + 1);
        }
      }
    }
    if (tile[i].g4 != NULL) {
      for (j=0; j<8; j++) {
        if (tile[i].g4[j] == -1) {
          break;
        }
        else {
          SwapByPosValue(state, TILE_SIZE, tile[i].g4[j], 2 * GLOBAL_NUM + j);
        }
      }
    }

    /* Count the upper limit of the edge number */
    nedge = 0;
    for (j=0; j<TILE_SIZE; j++) {
      if (state[j] != -1) {
        pos[state[j]] = j;
        nedge += gxadj[state[j] + 1] - gxadj[state[j]];
      }
    }
    for (j=0; j<GLOBAL_NUM; j++) {
      for (k=0; k<2; k++) {
        index = TILE_SIZE + j * 2 + k;
        gsrc = global[j].src[i][k];
        if (gsrc != -1) {
          ste = tile[gsrc / 2].global[j][gsrc % 2];
          nedge += gxadj[ste + 1] - gxadj[ste];
        }
      }
    }
    if (g4 != NULL) {
      for (k=0; k<8; k++) {
        index = TILE_SIZE + GLOBAL_NUM * 2 + k;
        gsrc = g4->src[i][k];
        if (gsrc != -1) {
          ste = tile[gsrc / 8].g4[gsrc % 8];
          nedge += gxadj[ste + 1] - gxadj[ste];
        }
      }
    }
    if (nedge == 0) {
      printf("tile: %d\n", i);
      exit(0);
    }
    tadjncy = (int*)malloc(nedge * sizeof(int));
    assert(tadjncy);
    tile[i].adjncy = tadjncy;

    /* Copy the fields */
    txadj = tile[i].xadj;
    txadj[0] = 0;
    for (j=0; j<TILE_SIZE; j++) {
      txadj[j+1] = txadj[j];
      if (state[j] != -1) {
        tile[i].sname[j] = graph->name[state[j]];
        tile[i].start[j] = graph->start[state[j]];
        tile[i].report[j] = graph->report[state[j]];
        for (k=0; k<8; k++) {
          tile[i].ste[j][k] = graph->ste[8 * state[j] + k];
        }

        for (k=gxadj[state[j]]; k<gxadj[state[j]+1]; k++) {
          to = graph->adjncy[k];
          curtile = (tile[i].duplicated==-1)? i: tile[i].duplicated;
          if (where[to] == curtile) {
            tadjncy[txadj[j+1]++] = pos[to];
          }
        }
      }
    }

    /* Add connections from Global Switches */
    for (j=0; j<GLOBAL_NUM; j++) {
      for (k=0; k<2; k++) {
        index = TILE_SIZE + j * 2 + k;
        txadj[index + 1] = txadj[index];
        gsrc = global[j].src[i][k];
        if (gsrc != -1) {
          ste = tile[gsrc / 2].global[j][gsrc % 2];
          for (l=gxadj[ste]; l<gxadj[ste+1]; l++) {
            to = gadjncy[l];
            curtile = (tile[i].duplicated==-1)? i: tile[i].duplicated;
            if (where[to] == curtile) {
              tadjncy[txadj[index+1]++] = pos[to];
            }
          }
        }
      }
    }
    if (g4 != NULL) {
      for (k=0; k<8; k++) {
        index = TILE_SIZE + GLOBAL_NUM * 2 + k;
        txadj[index + 1] = txadj[index];
        gsrc = g4->src[i][k];
        if (gsrc != -1) {
          ste = tile[gsrc / 8].g4[gsrc % 8];
          for (l=gxadj[ste]; l<gxadj[ste+1]; l++) {
            to = gadjncy[l];
            curtile = (tile[i].duplicated==-1)? i: tile[i].duplicated;
            if (where[to] == curtile) {
              tadjncy[txadj[index+1]++] = pos[to];
            }
          }
        }
      }
    }
  }
  chip->curtile = i - 1;
  chip->remain = TILE_SIZE - tile[i-1].nstate;
}

/*
* Copy graph information, such as state names and transistion, to tiles.
* Simpler than CopyGraphToTile since global switches are not involved.
*/
void CopySmallGraphToTile(tile_t *tile, graph_t *graph)
{
  int *gxadj = graph->xadj;
  int *pos = graph->pos;
  int *state = tile->state;
  int *txadj = tile->xadj;
  int nvtxs = graph->nvtxs;
  int nedge = gxadj[nvtxs] + txadj[TILE_SIZE];
  int index = 0;
  int oldxadj = 0;
  int *tadjncy;
  int i, j, k;

  assert(txadj[TILE_SIZE] >= 0);

  /* Copy STE information */
  for (i=TILE_SIZE-1; i>=0; i--) {
    if (state[i]==-1 && index<nvtxs) {
      state[i] = index;
      pos[index] = i;
      tile->sname[i] = graph->name[index];
      tile->start[i] = graph->start[index];
      tile->report[i] = graph->report[index];
      for (k=0; k<8; k++) {
        tile->ste[i][k] = graph->ste[8 * index + k];
      }
      index++;
    }
    else if (state[i] != -1) {
      state[i] = TILE_SIZE; /* mark old states.
           graph size < TILE_SIZE; therefore, the index of the graph
           is also smaller than TILE_SIZE */
    }
  }
  tile->nstate += nvtxs;

  /* Update the Local Switch */
  tadjncy = (int*)malloc(nedge * sizeof(int));
  index = 0;
  for (i=0; i<TILE_SIZE; i++) {
    /* Copy old edges */
    for (j=oldxadj; j<txadj[i+1]; j++) {
      tadjncy[index++] = tile->adjncy[j];
    }

    /* Copy new edges */
    if (state[i]!=-1 && state[i]!=TILE_SIZE) {
      for (j=gxadj[state[i]]; j<gxadj[state[i]+1]; j++) {
        tadjncy[index++] = pos[graph->adjncy[j]];
      }
    }

    /* Update xadj */
    oldxadj = txadj[i+1];
    txadj[i+1] = index;
  }
  if (index != nedge) {
    printf("index=%d nedge=%d\n", index, nedge);
  }
  assert(index == nedge);

  free(tile->adjncy);
  tile->adjncy = tadjncy;
}

/*
* Write tile configuration to a file
*/
void EmitTile(tile_t *tile, FILE *fp)
{
  int *xadj = tile->xadj;
  int *adjncy = tile->adjncy;
  int gsrc, state, index;
  int i, j, k;

  /* Print the first GLOBAL_NUM*2 lines that come from global switches */
  for (i=0; i<GLOBAL_NUM; i++) {
    for (j=0; j<2; j++) {
      index = TILE_SIZE + i * 2 + j;
      fprintf(fp, "%d[%d]: ", i, j);
      for (k=xadj[index]; k<xadj[index+1]; k++) {
        fprintf(fp, " %d", adjncy[k]);
      }
      fprintf(fp, "\n");
    }
  }
  if (tile->g4 != NULL) {
    for (i=0; i<8; i++) {
      index = TILE_SIZE + 2 * GLOBAL_NUM + i;
      fprintf(fp, "G4[%d]: ", i);
      for (k=xadj[index]; k<xadj[index+1]; k++) {
        fprintf(fp, " %d", adjncy[k]);
      }
      fprintf(fp, "\n");
    }
  }

  /* Print the STEs and their corroponding part of the local switch */
  for (i=0; i<TILE_SIZE; i++) {
    fprintf(fp, "%d: ", i);
    if (tile->state[i] != -1) {
      fprintf(fp, "%s %d %d ", tile->sname[i], tile->start[i], tile->report[i]);
      for (j=0; j<8; j++) {
        fprintf(fp, "%08X ", tile->ste[i][j]);
      }
      fprintf(fp, "->");
      for (j=xadj[i]; j<xadj[i+1]; j++) {
        fprintf(fp, " %d", adjncy[j]);
      }
    }
    fprintf(fp, "\n");
  }
}

/*
* Release tile resources
*/
void FreeTile(tile_t *tile)
{
  int i;

  free(tile->out.value);
  FreeList(tile->ghost);
  free(tile->adjncy);
  free(tile->g4);
  if (tile->duplicated != -1) {
    return;
  }
  for (i=0; i<TILE_SIZE; i++) {
    free(tile->sname[i]);
  }
}
