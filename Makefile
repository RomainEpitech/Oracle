CC = gcc
CFLAGS = -Wall -Wextra -g $(shell pkg-config --cflags json-c libcurl)
LDFLAGS = $(shell pkg-config --libs json-c libcurl)

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin
INCLUDE_DIR = includes

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

TARGET = $(BIN_DIR)/oracle

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

rebuild: clean all

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean rebuild run