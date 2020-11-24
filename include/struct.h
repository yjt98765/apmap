/*
* Copyright (c) 2019, Delft University of Technology
*
* struct.h
*
* Defination of data structures
*/

#ifndef _STRUCTBIN_H_
#define _STRUCTBIN_H_

typedef struct {
  int maxsize;
  int size;
  int *value;
} list_t;

typedef struct {
  int nvtxs;	/* The # of vertices and edges in the graph */
  int npart;    /* The # of parts that the graph is divided into */
  int *xadj;		/* Pointers to the locally stored vertices */
  int *adjncy;        /* Array that stores the adjacency lists of nvtxs */
  int *where; /* Array that stores the part index of each vertex */
  int *pos; /* Position in the tile */
  list_t **ext; /* Pointers to list structs that record each vertex's the external parts */
  unsigned *ste; /* Array that stores the accepting characters of all the states */
  char *start; /* Array that stores whether a state is a start state */
  char *report; /* Array that stores whether a state is a final state */
  char **name; /* Pointers to the STE names */
  int cost;

  int *first;
  int *current;
  int *next;
  int *from;
} graph_t;

typedef struct {
  int nstate;
  int nedge;
  char *fname;
  char mapped;
} automata_t;

/*
* Represent a tile that consists of an STE array and a local switch
*/
typedef struct {
  int nstate;           /* Number of states */
  int state[TILE_SIZE]; /* The Id of the states */
  int xadj[TILE_SIZE + MAX_IN + 1]; /* The pointer to adjncy array. 
                    xadj and adjncy together store the local graph */
  int *adjncy; /* This array contains the ending points of the transistions */

  list_t out;
  char *sname[TILE_SIZE];
  unsigned ste[TILE_SIZE][8];
  char start[TILE_SIZE];
  char report[TILE_SIZE];
  int global[GLOBAL_NUM][2];
  int *g4;
  list_t *ghost;
  char duplicated;
} tile_t;

/*
* Represent a 1-way global switch
*/
typedef struct {
  int src[TILE_NUM][2]; /* The index of the input row */
} global_t;

/*
* Represent a 4-way global switch
*/
typedef struct {
  int src[TILE_NUM][8]; /* The index of the input row */
} g4_t;

/*
* Represent an Automata Processor
*/
typedef struct {
  global_t global[GLOBAL_NUM]; /* Global switches (1 way) */
  tile_t tile[TILE_NUM];       /* Tiles */
  g4_t *g4;                    /* Global switches (4 ways) */
  int curtile; /* Id of the tile that is ready for mapping the next automata */
  int remain;  /* The number of STEs remaining unused in curtile */
} chip_t;

typedef struct linkedlist {
  int value;
  struct linkedlist *next;
} linkedlist;

#endif
