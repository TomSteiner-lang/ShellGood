CC = gcc
CFLAGS = -Wall -g -I./headers -I./headers/global

SRC_DIR = src
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj


SRC = $(wildcard $(SRC_DIR)/*.c)
OBJ = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC))


GLOBAL_HEADERS = $(wildcard headers/global/*.h)


TARGET = ShellGood

all: $(TARGET)


$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET)
	chmod +x $(TARGET)
	$(info SRC=$(SRC))

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

deps: $(SRC)
	gcc -MM $(SRC) > .deps.mk

-include .deps.mk

.PHONY: clean
clean:
	rm -f $(OBJ) $(TARGET) .deps.mk

.PHONY: headers
headers: $(GLOBAL_HEADERS)
	echo "global headers are up to date"

	