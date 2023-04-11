

CC = gcc
BUILD_DIR = build

EXE += ncurses_mc

SRC = \
ncurses_mc.c

LIB = \
-lncurses

all: $(BUILD_DIR) $(EXE)

$(BUILD_DIR):
	mkdir $(BUILD_DIR)

$(EXE): $(SRC)
	$(CC) $^ $(LIB) -o $(BUILD_DIR)/$@

clean:
	rm -rf $(BUILD_DIR)

run:
	./$(BUILD_DIR)/$(EXE)
