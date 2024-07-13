CC = gcc
SRC = ./src
OBJ = ./obj
BIN = ./bin

CFLAGS = -Wall -Wextra -pedantic -std=c99

SRCS = $(SRC)/main.c

OBJS = $(OBJ)/main.o

all: $(BIN)/out

$(BIN):
	mkdir -p $(BIN)

$(OBJ):
	mkdir -p $(OBJ)

$(BIN)/out: $(OBJS) | $(BIN)
	$(CC) $(OBJS) -o $(BIN)/out

$(OBJ)/%.o: $(SRC)/%.c | $(OBJ)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(BIN)/out
	rm -rf $(OBJ) $(BIN)

.PHONY: all clean
