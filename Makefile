CFLAGS  := -ggdb -Wall -pedantic -lpq

SRC_DIR := src
OBJ_DIR := src/obj
BIN_DIR := bin

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(SRC_DIR)/%.h
	$(CC) $(CFLAGS) -c $< -o $@

bbs-server: $(OBJS)
	$(CC) $(CFLAGS) $^ -o $(BIN_DIR)/$@

ifneq (clean, $(MAKECMDGOALS))
-include deps.mk
endif

deps.mk: $(SRCS)
	$(CC) -MM $^ > $(SRC_DIR)/$@

clean:
	rm -f $(OBJ_DIR)/*.o $(BIN_DIR)/bbs-server

run:
	$(BIN_DIR)/bbs-server wdir

valgrind:
	valgrind --tool=memcheck $(BIN_DIR)/bbs-server wdir