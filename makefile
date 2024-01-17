CC = gcc
CFLAGS = -std=c89 -Wall -Wextra -Werror -Wno-unused-variable -Wno-unused-parameter -Wno-declaration-after-statement -pedantic -g

SRC_DIR = src
BUILD_DIR = build
BIN_DIR = $(BUILD_DIR)/bin
OBJ_DIR = $(BUILD_DIR)/obj
ASSEMBLY_DIR = $(BUILD_DIR)/asm

EXECUTABLE = $(BIN_DIR)/web_server
SOURCES = $(shell find $(SRC_DIR) -type f -name "*.c")
OBJECTS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SOURCES))
ASSEMBLY_FILES = $(patsubst $(SRC_DIR)/%.c, $(ASSEMBLY_DIR)/%.s, $(SOURCES))

all: $(EXECUTABLE) $(ASSEMBLY_FILES)

$(EXECUTABLE): $(OBJECTS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I$(SRC_DIR) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(ASSEMBLY_DIR)/%.s: $(SRC_DIR)/%.c | $(ASSEMBLY_DIR)
	mkdir -p $(dir $@)
	$(CC) -S -I$(SRC_DIR) $< -o $@

$(ASSEMBLY_DIR):
	mkdir -p $(ASSEMBLY_DIR)

clean:
	rm -rf $(BIN_DIR) $(OBJ_DIR) $(ASSEMBLY_DIR)

.PHONY: all clean
