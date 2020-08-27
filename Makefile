IDIR=include
SDIR=src
CC=gcc
CFLAGS=-I$(IDIR) -g

ODIR=obj
LIBS=-lm -lmetis

_DEPS = apmapbin.h proto.h struct.h def.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = apmap.o chip.o global.o graph.o parser.o list.o partition.o tile.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

all:apmap

$(ODIR)/%.o: $(SDIR)/%.c $(DEPS)
	$(info $(shell mkdir -p $(ODIR)))
	$(CC) -c -o $@ $< $(CFLAGS)

apmap: $(OBJ) 
	gcc -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ 
