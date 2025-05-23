# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -pg -Iinclude -Iinclude/ppu
LDFLAGS = -lSDL2

# Directories
SRC_DIR = src
BUILD_DIR = build
BIN_DIR = bin
BIN = $(BIN_DIR)/emulator

# Source files (including PPU sources)
SRCS := $(shell find $(SRC_DIR) -name '*.c')


# Map .c files to .o files in the build/ folder
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# Default target
all: $(BIN)

# Linking
$(BIN): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compiling .c to .o
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean
clean:
	rm -rf $(BUILD_DIR)/* $(BIN)/*

.PHONY: all clean
