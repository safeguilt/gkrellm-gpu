# Makefile for GKrellM GPU Plugin

SRC_DIR := src
BUILD_DIR := build
PLUGIN := $(BUILD_DIR)/gkrellm-gpu.so
OBJS := $(BUILD_DIR)/gkrellm-gpu.o

# Compiler and flags
CC = gcc
CFLAGS = -Wall -fPIC -O2 `pkg-config --cflags gtk+-2.0`
LDFLAGS = -shared -Wl,-soname,$(notdir $(PLUGIN))
LIBS = `pkg-config --libs gtk+-2.0` -lnvidia-ml

# Installation directory
# Installation directory
INSTALL_DIR ?= $(HOME)/.gkrellm2/plugins

# Build rules
all: $(PLUGIN)

$(PLUGIN): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

install: $(PLUGIN)
	mkdir -p $(INSTALL_DIR)
	cp $(PLUGIN) $(INSTALL_DIR)/

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all install clean
