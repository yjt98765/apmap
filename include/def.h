/*
* Copyright (c) 2019, Delft University of Technology
*
* def.h
*
* Defination of constant values
*/

#ifndef _DEF_BIN_H_
#define _DEF_BIN_H_

/* The number of tiles in a chip */
#define TILE_NUM 128

/* The number of global switches in a chip */
#define GLOBAL_NUM 4

/* The number of chips in a system */
#define CHIP_NUM 2

/* The number of STEs in a tile */
#define TILE_SIZE 256

/*
* Parameter used for unbalanced mapping. If the number of remaining STEs in a tile is
* less than the THRESHOLD, then these STEs will not be used. The mapping of other large
* graphs will start from the next tile.
*/
#define THRESHOLD 25

/* The number of outgoing channels in a tile */
#define MAX_OUT (GLOBAL_NUM * 2 + 8)

/* The number of incoming channels in a tile */
#define MAX_IN (GLOBAL_NUM * 2 + 8)

#endif
