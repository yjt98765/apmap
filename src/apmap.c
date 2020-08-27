/*
* Copyright (c) 2019, Delft University of Technology
*
* apmap.c
*
* Main function of the Apmap program
*/
#include "apmapbin.h"

/*
* Compare the sizes of automata. Needed by qsort
*/
int CompAutomata(const void *a, const void *b)
{
  int sizea = ((automata_t*)a)->nstate;
  int sizeb = ((automata_t*)b)->nstate;
  int edgea = ((automata_t*)a)->nedge;
  int edgeb = ((automata_t*)b)->nedge;
  if (sizea == sizeb) {
    return edgeb - edgea;
  }
  else {
    return sizeb - sizea;
  }
}

int main(int argc, char *argv[])
{
  FILE *fmap;
  char **gfname;
  automata_t *automata;
  graph_t *graph, *ungraph;
  int ngraph, maxedge;
  chip_t *chip;
  int minauto, minautosize, candidate;
  char succeed;
  float ntile;
  int i, j, k;

  /* parse input file */
  if (argc != 2) {
    printf("usage: %s map_file\n", argv[0]);
    exit(0);
  }
  fmap = fopen(argv[1], "r");
  if (!fmap) {
    errexit("Cannot open file %s!\n", argv[1]);
  }

  chip = (chip_t*)malloc(CHIP_NUM * sizeof(chip_t));
  for (i=0; i<CHIP_NUM; i++) {
    ChipInit(&chip[i]);
  }

  automata = ReadMapFile(fmap, &ngraph, &gfname);
  fclose(fmap);
  qsort(automata, ngraph, sizeof(automata_t), CompAutomata);
  minauto = ngraph-1;
  minautosize = automata[minauto].nstate;

  maxedge = automata[0].nedge;
  for (i=0; i<ngraph; i++) {
    automata[i].mapped = 0;
    maxedge = (automata[i].nedge>maxedge)? automata[i].nedge: maxedge;
  }
  graph = CreateGraph(automata[0].nstate, maxedge, 1);
  ungraph = CreateGraph(automata[0].nstate, maxedge * 2, 0);

  for (i=0; i<ngraph; i++) {
    if (automata[i].mapped) {
      continue;
    }

    /* read graph */
    ReadGraphFile(graph, gfname[automata[i].fname], automata[i].nstate, automata[i].nedge);

    for (k=0; k<CHIP_NUM; k++) {
      fflush(stdout);
      succeed = MapGraphToChip(&chip[k], graph, ungraph);
      if (succeed == 1) {
        break;
      }
    }
    if (succeed != 1) {
      errexit("%s cannot be mapped!\n", gfname[automata[i].fname]);
    }

    automata[i].mapped = 1;
    if (i == minauto) {
      break;
    }
	
    /* fill the remaining part of a tile with small graphs */
    while (chip[k].remain >= minautosize) {
      for (j=minauto; (automata[j].nstate<=chip[k].remain) && (j>0); j--) {
        if (!automata[j].mapped) {
          candidate = j;
        }
      }
      ReadGraphFile(graph, gfname[automata[candidate].fname],
                    automata[candidate].nstate, automata[candidate].nedge);
      MapGraphToChip(&chip[k], graph, graph);
      automata[candidate].mapped = 1;
      fflush(stdout);

      /* Update minimum automata size */
      if (candidate == minauto) {
        for (j=minauto-1; j>0; j--) {
          if (!automata[j].mapped) {
            minauto = j;
            minautosize = automata[j].nstate;
            break;
          }
        }
        if (minauto == candidate) { /* No small automata */
          minautosize = TILE_SIZE;
        }
      }
    }
    if (chip[k].remain < THRESHOLD)
    {
      chip[k].curtile++;
      chip[k].remain = TILE_SIZE;
    }
  }

  /* Report chip utilization */
  ntile = 0;
  for (k=0; k<CHIP_NUM; k++) {
    if (chip[k].remain == TILE_SIZE) {
      ntile += chip[k].curtile;
    }
    else {
      ntile += chip[k].curtile + 1 - (float)chip[k].remain / TILE_SIZE;
    }
  }
  printf("%.1f tiles in total\n", ntile);
  fflush(stdout);

  /* Emit mapping result */
  fmap = fopen("map_result", "w");
  if (!fmap) {
    errexit("Cannot open file map_result!\n");
  }
  for (i=0; i<CHIP_NUM; i++) {
    if (chip[i].curtile>0 || chip[i].remain<TILE_SIZE) {
      fprintf(fmap, "**************\n");
      fprintf(fmap, "*** Chip %d ***\n", i);
      fprintf(fmap, "**************\n");
      EmitChip(&chip[i], fmap);
    }
  }
  fclose(fmap);

  /* Release resources */
  FreeGraph(&graph, automata[0].nstate);
  FreeGraph(&ungraph, automata[0].nstate);
  for (i=0; i<ngraph; i++) {
    free(gfname[i]);
  }
  free(gfname);
  free(automata);
  for (i=0; i<CHIP_NUM; i++) {
    FreeChip(&chip[i]);
  }
  free(chip);
  return 0;
}
