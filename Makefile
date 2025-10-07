# Minimal Makefile for Modbus-only build
# Requires: libmodbus (pkg-config: libmodbus)

CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra
PKG_CFLAGS := $(shell pkg-config --cflags libmodbus 2>/dev/null)
PKG_LIBS   := $(shell pkg-config --libs libmodbus 2>/dev/null)
INCLUDES := -Iinclude

BUILD_DIR := build
BIN_DIR := $(BUILD_DIR)/bin
LIB_DIR := $(BUILD_DIR)/lib

LIB_NAME := dsemodbus
LIB_STATIC := $(LIB_DIR)/lib$(LIB_NAME).a

SRC := src/dsemodbus/modbus_client.cpp
HDR := include/dsemodbus/modbus_client.h

all: $(LIB_STATIC) $(BIN_DIR)/weight_client

$(LIB_STATIC): $(SRC) $(HDR)
	@mkdir -p $(LIB_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(PKG_CFLAGS) -c $(SRC) -o $(BUILD_DIR)/modbus_client.o
	ar rcs $(LIB_STATIC) $(BUILD_DIR)/modbus_client.o

$(BIN_DIR)/weight_client: src/weight_client.cpp $(LIB_STATIC) $(HDR)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(PKG_CFLAGS) -o $@ $< $(SRC) $(PKG_LIBS)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean
