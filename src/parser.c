/*
* Copyright (c) 2019, Delft University of Technology
*
* parser.c
*
* Functions related to parsing input files
*/

#include "apmapbin.h"
#include <errno.h>

/*
* Read a map file, which defines a full automaton
*/
automata_t *ReadMapFile(FILE *fpin, int *ngraph)
{
  size_t lnlen = 1024;
  char *line = (char*)malloc(1024);
  automata_t *automata;
  int rlen;
  char nfields;
  int i;

  /* Skip comment lines until you get to the first valid line */
  do {
    if (getline(&line, &lnlen, fpin) == -1) 
      errexit("Premature end of the graph input file\n");
  } while (line[0] == '%');

  nfields = sscanf(line, "%d", ngraph);
  if (nfields!=1 || *ngraph <= 0) 
    errexit("Wrong map file.\n");

  automata = (automata_t*)malloc(*ngraph * sizeof(automata_t));

  for (i=0; i<*ngraph; i++) {
    do {
      rlen = getline(&line, &lnlen, fpin);
      if (rlen == -1) 
        errexit("Premature end of input file while reading CC %d.\n", i);
    } while (line[0] == '%');

    nfields = sscanf(line, "%d %d", &automata[i].nstate, &automata[i].nedge);
    if (nfields!=2 || *ngraph <= 0) 
      errexit("Wrong size while reading CC %d.\n");

    /* Decide the position of the graph file name */
    nfields = ceil(log10(automata[i].nstate + 0.5)) + ceil(log10(automata[i].nedge + 0.5));
    while (line[nfields + 2] == ' ' || line[nfields + 2] == '\t') {
      nfields++;
    }
    rlen -= nfields + 3;

    /* Copy the graph file name */
    automata[i].fname = (char*)malloc(rlen + 1);
    sscanf(line + nfields + 2, "%s", automata[i].fname);
  }

  return automata;
}

/*
* Read a graph file, which is a connected component of an automaton
* Modified based on the ReadGraph function of Metis.
*/
void ReadGraphFile(graph_t *graph, const char *file, int nvtxs, int nedges)
{
  FILE* fpin = fopen(file, "r");
  size_t lnlen = 1024;
  char *line = (char*)malloc(1024);
  int *xadj = graph->xadj;
  int *adjncy = graph->adjncy;
  int *ste = graph->ste;
  char *start = graph->start;
  char *report = graph->report;
  char **name = graph->name;
  int edge;
  char *curstr, *newstr;
  char nfields;
  int i, j, k;

  if (!fpin) {
    errexit("Cannot open file \"%s\"!\n", file);
  }
  if (!line) {
    errexit("Cannot allocate line!\n");
  }
  graph->nvtxs = nvtxs;

  /*----------------------------------------------------------------------
   * Read the sparse graph file
   *---------------------------------------------------------------------*/
  for (xadj[0]=0, k=0, i=0; i<nvtxs; i++) {
    do {
      if (getline(&line, &lnlen, fpin) == -1) {
        errexit("Premature end of input file while reading vertex %d.\n", i+1);
      }
    } while (line[0] == '%');

    curstr = line;
    newstr = NULL;

    /* Read NAME field */
    for (j=0; curstr[j]!=' '; j++) {
      if (curstr[j] == '\n') {
        errexit("STE pattern is not complete at the %d line of file %s.\n", i, file);
      }
    }
    name[i] = (char*)malloc(j + 1);
    strncpy(name[i], curstr, j);
    name[i][j] = '\0';
    curstr += j + 1;

    /* Read START field */
    if (curstr[0] == '0') {
      start[i] = 0;
    }
    else {
      start[i] = 1;
    }
    curstr += 2;

    /* Read REPORT field */
    if (curstr[0] == '0') {
      report[i] = 0;
    }
    else {
      report[i] = 1;
    }
    curstr += 2;

    /* Read PATTERN field */
    for (j=0; j<8; j++) {
      ste[i * 8 + j] = strtoul(curstr, &newstr, 16);
      if (newstr == curstr) {
        errexit("STE pattern is not complete at the %d line of file %s.\n", i, file);
      }
      curstr = newstr;
    }

    /* Read outgoing edges */
    while (1) {
      edge = strtol(curstr, &newstr, 10);
      if (newstr == curstr)
        break; /* End of line */
      curstr = newstr;

      if (edge < 1 || edge > graph->nvtxs)
        errexit("Edge %d for vertex %d is out of bounds\n", edge, i+1);

      if (k == nedges)
        errexit("There are more edges in the file than the %d specified.\n", 
            nedges);

      adjncy[k] = edge-1;
      k++;
    } 
    xadj[i+1] = k;
  }

  if (k != nedges) {
    printf("------------------------------------------------------------------------------\n");
    printf("***  I detected an error in your input file  ***\n\n");
    printf("In the .map file, you specified that the graph contained\n"
           "%d edges. However, I only found %d edges in the file.\n", 
           nedges, k);
    printf("Please specify the correct number of edges in the .map file.\n");
    printf("------------------------------------------------------------------------------\n");
    exit(0);
  }

  fclose(fpin);
  free(line);
}

