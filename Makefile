CC = gcc
SRC = ./src
OBJ = ./obj
BIN = ./bin

CFLAGS = -Wall -Wextra -pedantic -std=c99

SRCS = $(SRC)/main.c 	  \
		$(SRC)/editor.c   \
		$(SRC)/fileio.c   \
		$(SRC)/terminal.c

OBJS = $(patsubst $(SRC)/%.c,$(OBJ)/%.o,$(SRCS))

all: $(BIN)/out

$(BIN):
	mkdir -p $(BIN)

$(OBJ):
	mkdir -p $(OBJ)

$(BIN)/out: $(OBJS) | $(BIN)
	$(CC) $(OBJS) -o $(BIN)/out

# Regra para compilar arquivos objeto
$(OBJ)/%.o: $(SRC)/%.c | $(OBJ)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(BIN)/out
	rm -rf $(OBJ) $(BIN)

.PHONY: all clean
