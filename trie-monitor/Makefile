# Compiler
CC = gcc

# Directories
SRC_DIR = src
BUILD_DIR = build

# Source files
SRC = $(wildcard $(SRC_DIR)/*.c)
OBJ = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRC))

# Output executable
TARGET = $(BUILD_DIR)/monitor

# Allowed optimization levels
VALID_OPTS := O0 O1 O2 O3 Os Ofast

# Optimization level (default: O2, the level of the paper's timing run)
OPT ?= O2

# Validate OPT
ifeq (,$(filter $(OPT),$(VALID_OPTS)))
$(error Invalid optimization level '$(OPT)'. Valid options are: $(VALID_OPTS))
endif

# Compiler flags
CFLAGS = -Wall -Wextra -$(OPT)

# Linker flags
LDFLAGS =

# Default target
all: $(TARGET)

# Link executable
$(TARGET): $(OBJ)
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(OBJ) -o $@ $(LDFLAGS)

# Compile each source file
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean
clean:
	rm -rf $(BUILD_DIR)

