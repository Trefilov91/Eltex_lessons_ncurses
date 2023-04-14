

CC = gcc

EXE += ncurses_mc

SRC = \
ncurses_mc.c

LIB = \
-lncurses

all: $(EXE)

$(EXE): $(SRC)
	$(CC) $^ $(LIB) -o $@

clean:
	rm -f $(EXE)

