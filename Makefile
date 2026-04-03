CC = gcc
CFLAGS = -O2 -Wall -Wextra -std=c99
LFLAGS = -lm

SRC_DIR  = src
TEST_DIR = test
OBJ_DIR  = obj
BIN_DIR  = bin
 
CORE_SRC = $(SRC_DIR)/fzstego.c $(SRC_DIR)/fuzzy.c $(SRC_DIR)/fuzzy_steg.c
CORE_OBJ = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(CORE_SRC))
 
HEADERS = $(SRC_DIR)/fzstego.h $(SRC_DIR)/fuzzy.h $(SRC_DIR)/fuzzy_steg.h



.PHONY: all clean demo unit-test perf-test
all: demo unit-test perf-test
test: unit-test perf-test

demo: $(BIN_DIR)/demo
unit-test: $(BIN_DIR)/unit-test
perf-test: $(BIN_DIR)/perf-test

$(BIN_DIR)/demo: $(OBJ_DIR)/demo.o $(CORE_OBJ) | $(BIN_DIR)
	$(CC) $(CFLAGS) -I$(SRC_DIR) $^ -o $@ $(LFLAGS)

$(BIN_DIR)/unit-test: $(OBJ_DIR)/test.o $(CORE_OBJ) | $(BIN_DIR)
	$(CC) $(CFLAGS) -I$(SRC_DIR) -I$(TEST_DIR) $^ -o $@ $(LFLAGS)

$(BIN_DIR)/perf-test: $(OBJ_DIR)/performance.o $(CORE_OBJ) | $(BIN_DIR)
	$(CC) $(CFLAGS) -I$(SRC_DIR) -I$(TEST_DIR) $^ -o $@ $(LFLAGS)

 

$(OBJ_DIR)/demo.o: demo.c $(HEADERS) | $(OBJ_DIR)
	$(CC) $(CFLAGS) -I$(SRC_DIR) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(HEADERS) | $(OBJ_DIR)
	$(CC) $(CFLAGS) -I$(SRC_DIR) -c $< -o $@

$(OBJ_DIR)/%.o: $(TEST_DIR)/%.c $(HEADERS) | $(OBJ_DIR)
	$(CC) $(CFLAGS) -I$(SRC_DIR) -I$(TEST_DIR) -c $< -o $@


$(OBJ_DIR) $(BIN_DIR):
	mkdir $@


clean:
	@if exist $(BIN_DIR) rmdir /s /q $(BIN_DIR)
	@if exist $(OBJ_DIR) rmdir /s /q $(OBJ_DIR)
