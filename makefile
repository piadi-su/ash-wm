TARGET = ashwm

SRC_DIR = src
OBJ_DIR = bin

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=gnu99 -I$(SRC_DIR)
LDFLAGS = -lX11 -lXinerama

DEBUG_FLAGS = -g -DDEBUG -O0

PREFIX = /usr/local
BINDIR = $(PREFIX)/bin

all: prep $(TARGET)

prep:
	@mkdir -p $(OBJ_DIR)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

debug: CFLAGS += $(DEBUG_FLAGS)
debug: clean prep $(TARGET)
	@echo "[ASH-WM] successfully compiled"

clean:
	rm -rf $(OBJ_DIR) $(TARGET)
	@echo "[ASH-WM] cleaned."

install: all
	install -Dm755 $(TARGET) $(DESTDIR)$(BINDIR)/$(TARGET)
	@echo "[ASH-WM] successfully installed in $(BINDIR)/$(TARGET)"

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(TARGET)
	@echo "[ASH-WM] successfully removed"

.PHONY: all prep debug clean install uninstall
