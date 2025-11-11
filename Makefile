CC = gcc
CFLAGS = -Iinclude
SRC = src/main.c src/shell.c src/execute.c
OBJ = $(SRC:.c=.o)
BIN_DIR = bin
BIN = $(BIN_DIR)/myshell

all: $(BIN)

$(BIN): $(OBJ)
	mkdir -p $(BIN_DIR)
	$(CC) $(OBJ) -o $(BIN)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf src/*.o $(BIN_DIR)
