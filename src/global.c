/*
* Copyright (c) 2019, Delft University of Technology
*
* global.c
*
* Functions related to global switches
*/
#include "apmapbin.h"

/*
* Initiate a global switch
*/
void InitGlobal(global_t *global)
{
  int j;
  for (j=0; j<TILE_NUM; j++) {
    global->src[j][0] = -1;
    global->src[j][1] = -1;
  }
}

/*
* Map the output of a state to a 1-way global switch
* return 0 if fail; return 1 if succeed.
*/
char MapStateToGlobal(global_t *global, list_t *ext, int src, int curtile)
{
  int dest;
  int i;

  assert(ext);
  /* detect */
  for (i=0; i<ext->size; i++) {
    dest = ext->value[i] + curtile;
    if (global->src[dest][1] != -1) {
      return 0;
    }
  }

  /* map */
  for (i=0; i<ext->size; i++) {
    dest = ext->value[i] + curtile;
    if (global->src[dest][0] == -1) {
      global->src[dest][0] = src;
    }
    else {
      global->src[dest][1] = src;
    }
  }
  return 1;
}

/*
* Map the output of a state to a 4-way global switch
* return 0 if fail; return 1 if succeed.
*/
char MapStateToG4(g4_t *g4, list_t *ext, int src, int curtile)
{
  int dest;
  int i, j;

  /* detect */
  for (i=0; i<ext->size; i++) {
    dest = ext->value[i] + curtile;
    if (g4->src[dest][3] != -1) {
      return 0;
    }
  }

  /* map */
  for (i=0; i<ext->size; i++) {
    dest = ext->value[i] + curtile;
    for (j=0; j<8; j++) {
      if (g4->src[dest][j] == -1) {
        g4->src[dest][j] = src;
        break;
      }
    }
  }
  return 1;
}

/* 
* Config global switches according to the tiles 
*/
char MapGlobal(global_t global[GLOBAL_NUM], g4_t *g4, graph_t *graph, tile_t tile[TILE_NUM], int *curtile)
{
  int npart = graph->npart;
  list_t *ext;
  int state;
  char mapped;
  int i, j, k;

  if (tile[*curtile].nstate > 0) {
    for (j=0; j<GLOBAL_NUM; j++) {
      for (k=0; k<2; k++) {
        if (global[j].src[*curtile][k] != -1) {
          global[j].src[*curtile][k] = -2;
        }
      }
    }
    for (k=0; k<8; k++) {
      if (g4->src[*curtile][k] != -1) {
        g4->src[*curtile][k] = -2;
      }
    }
  }

  for (i=*curtile; i<*curtile+npart; i++) {
    for (j=0; j<tile[i].out.size; j++) {
      state = tile[i].out.value[j];
      ext = graph->ext[state];
      mapped = 0;
      for (k=0; k<GLOBAL_NUM; k++) {
        if (tile[i].global[k][0] == -1) {
          mapped = MapStateToGlobal(&global[k], ext, 2*i, *curtile);
          if (mapped) {
            tile[i].global[k][0] = state;
            break;
          }
        }
        else if (tile[i].global[k][1] == -1) {
          mapped = MapStateToGlobal(&global[k], ext, 2*i+1, *curtile);
          if (mapped) {
            tile[i].global[k][1] = state;
            break;
          }
        }
      }
      if (!mapped) {
        for (k=0; k<8; k++) {
          if (tile[i].g4[k] == -1) {
            mapped = MapStateToG4(g4, ext, 8*i+k, *curtile);
            if (mapped) {
              tile[i].g4[k] = state;
              break;
            }
          }
        }
      }
      if (!mapped) {
        return 0;
      }
    }
  }

  *curtile += npart - 1;
  return 1;
}

/*
* Copy the configuration of a global switch
*/
void CopyGlobal(global_t dest[GLOBAL_NUM], global_t src[GLOBAL_NUM])
{
  int i, j;
  for (i=0; i<GLOBAL_NUM; i++) {
    for (j=0; j<TILE_NUM; j++) {
      dest[i].src[j][0] = src[i].src[j][0];
      dest[i].src[j][1] = src[i].src[j][1];
    }
  }
}

/*
* Copy the configuration of a 4-way global switch
*/
void CopyG4(g4_t *dest, g4_t *src)
{
  int i, j;
  for (j=0; j<TILE_NUM; j++) {
    for (i=0; i<8; i++) {
      dest->src[j][i] = src->src[j][i];
    }
  }
}

/*
* Write the configuration of a global switch to a file
*/
void EmitGlobal(global_t global[GLOBAL_NUM], tile_t tile[TILE_NUM], FILE *fp)
{
  char *bitmap = (char*)malloc(4 * TILE_NUM * TILE_NUM);
  char ghost = 0;
  int tsrc, row;
  int i, j, k, l;

  for (i=0; i<TILE_NUM; i++) {
    if (tile[i].ghost) {
      if (tile[i].ghost->size > 0) {
        ghost = 1;
      }
    }
  }

  for (i=0; i<GLOBAL_NUM; i++) {

    /* Store the configuration to bitmap */
    memset(bitmap, 0, 4 * TILE_NUM * TILE_NUM);
    for (j=0; j<TILE_NUM; j++) {
      for (k=0; k<2; k++) {
        if (global[i].src[j][k] != -1) {
          bitmap[global[i].src[j][k] * TILE_NUM * 2 + j * 2 + k] = 1;
          if (ghost) {
            tsrc = global[i].src[j][k] / 2;
            if (tile[tsrc].ghost) {
              for (l=0; l<tile[tsrc].ghost->size; l++) {
                row = tile[tsrc].ghost->value[l] + global[i].src[j][k] % 2;
                bitmap[row * TILE_NUM * 2 + j * 2 + k] = 1;
              }
            }
          }
        }
      }
    }

    /* Print the configuration according to the bitmap */
    fprintf(fp, "\n--- Global Switch %d ---\n", i);
    for (j=0; j<2*TILE_NUM; j++) {
      fprintf(fp, "%d[%d]:", j / 2, j % 2);
      for (k=0; k<2*TILE_NUM; k++) {
        if (bitmap[j * TILE_NUM * 2 + k]) {
          fprintf(fp, " %d[%d]", k / 2, k % 2);
        }
      }
      fprintf(fp, "\n");
    }
  }
  free(bitmap);
}

/*
* Write the configuration of a 4-way global switch to a file
*/
void EmitG4(g4_t *g4, tile_t tile[TILE_NUM], FILE *fp)
{
  char *bitmap = (char*)malloc(64 * TILE_NUM * TILE_NUM);
  char ghost = 0;
  int tsrc, row;
  int i, j, k, l;

  for (i=0; i<TILE_NUM; i++) {
    if (tile[i].ghost) {
      if (tile[i].ghost->size > 0) {
        ghost = 1;
      }
    }
  }

  /* Store the configuration to bitmap */
  memset(bitmap, 0, 16 * TILE_NUM * TILE_NUM);
  for (j=0; j<TILE_NUM; j++) {
    for (k=0; k<8; k++) {
      if (g4->src[j][k] != -1) {
        bitmap[g4->src[j][k] * TILE_NUM * 8 + j * 8 + k] = 1;
        if (ghost) {
          tsrc = g4->src[j][k] / 8;
          if (tile[tsrc].ghost) {
            for (l=0; l<tile[tsrc].ghost->size; l++) {
              row = tile[tsrc].ghost->value[l] + g4->src[j][k] % 8;
              bitmap[row * TILE_NUM * 8 + j * 8 + k] = 1;
            }
          }
        }
      }
    }
  }

  /* Print the configuration according to the bitmap */
  fprintf(fp, "\n--- Global-4 Switch ---\n");
  for (j=0; j<8*TILE_NUM; j++) {
    fprintf(fp, "%d[%d]:", j / 8, j % 8);
    for (k=0; k<8*TILE_NUM; k++) {
      if (bitmap[j * TILE_NUM * 8 + k]) {
        fprintf(fp, " %d[%d]", k / 8, k % 8);
      }
    }
    fprintf(fp, "\n");
  }
  free(bitmap);
}

