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

void PrintHelp(const char* filename)
{
  printf("usage: %s [options] map_file1 [map_file2] ...\n", filename);
  printf("Options:\n");
  printf("\t-h or --help:\tprint this usage information.\n");
  printf("\t--no-g4:\texclude the 4-way global switch from the routing matrix.\n");
  printf("\t--no-opt:\tdisable constraint conflict resolving optimizations.\n");
}

int main(int argc, char *argv[])
{
  automata_t *automata; /* The array of input automata */
  graph_t *graph, *ungraph;
  int ngraph = 0, maxedge;
  chip_t *chip;
  int minauto, minautosize, candidate;
  char succeed;
  float ntile;
  int i, j, k;

  /* Variables for file reading */
  FILE *fmap;
  automata_t **ats = (automata_t**)malloc(sizeof(automata_t*) * argc);
  int *ngs = (int*)malloc(sizeof(int) * argc);

  /* Variables for parsing the command-line */
  static int has_g4 = 1;
  static int no_opt = 0;
  static struct option long_options[] = {
    {"no-g4", no_argument,       &has_g4, 0},
    {"no-opt",   no_argument,       &no_opt, 1},
    {"help", no_argument, 0, 'h'},
    {0, 0, 0, 0}
  };
  int c;
  int option_index = 0;

  /* Parse input file */
  while (1) {
    c = getopt_long (argc, argv, "h", long_options, &option_index);
    if (c == -1)
      break;
    switch (c) {
      case 0: /* If this option set a flag, do nothing else now. */
          break;
      case 'h':
        PrintHelp(argv[0]);
        return 0;
      case '?':
        PrintHelp(argv[0]);
        return 1;
      default:
        abort();
    }
  }

  if (optind == argc) {
    errexit("Please specify at least a map file.\n");
  }

  /* After calling getopt_long, the map files are arranged to the last of argv */
  for (i=optind; i<argc; i++) {
    fmap = fopen(argv[i], "r");
    if (!fmap) {
      errexit("Cannot open file %s!\n", argv[i]);
    }

    ats[i] = ReadMapFile(fmap, &ngs[i]);
    ngraph += ngs[i];
    fclose(fmap);
  }

  /* Merge all inputs together */
  automata = (automata_t*)malloc(sizeof(automata_t) * ngraph);
  k = 0;
  for (i=optind; i<argc; i++) {
    for (j=0; j<ngs[i]; j++) {
      automata[k++] = ats[i][j];
    }
  }
  free(ats);
  free(ngs);

  /* Sort the automata */
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

  chip = (chip_t*)malloc(CHIP_NUM * sizeof(chip_t));
  for (i=0; i<CHIP_NUM; i++) {
    ChipInit(&chip[i], has_g4);
  }

  for (i=0; i<ngraph; i++) {
    if (automata[i].mapped) {
      continue;
    }

    /* Read graph */
    ReadGraphFile(graph, automata[i].fname, automata[i].nstate, automata[i].nedge);

    for (k=0; k<CHIP_NUM; k++) {
      succeed = MapGraphToChip(&chip[k], graph, ungraph, no_opt);
      if (succeed == 1) {
        break;
      }
    }
    if (succeed != 1) {
      errexit("%s cannot be mapped!\n", automata[i].fname);
    }

    automata[i].mapped = 1;
    if (i == minauto) {
      break;
    }
	
    /* Fill the remaining part of a tile with small graphs */
    while (chip[k].remain >= minautosize) {
      for (j=minauto; (automata[j].nstate<=chip[k].remain) && (j>0); j--) {
        if (!automata[j].mapped) {
          candidate = j;
        }
      }
      ReadGraphFile(graph, automata[candidate].fname,
                    automata[candidate].nstate, automata[candidate].nedge);
      MapGraphToChip(&chip[k], graph, graph, no_opt);
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
    free(automata[i].fname);
  }
  free(automata);
  for (i=0; i<CHIP_NUM; i++) {
    FreeChip(&chip[i]);
  }
  free(chip);
  return 0;
}
