BIN_NAME := tcpserver_multi
CXX ?= g++
COMPILE_FLAGS = -std=c++11 -O2 -Wall -g
LINK_FLAGS = -levent -lpthread -ldl

SRC_EXT = cpp
#BIN_PATH = ./bin
SRC_PATH = ./src
BUILD_PATH = ./build
#INCLUDES = -I$(SRC_PATH)

SHELL = /bin/bash
SOURCES := $(shell find $(SRC_PATH) -name '*.$(SRC_EXT)')
OBJECTS = $(SOURCES:$(SRC_PATH)/%.$(SRC_EXT)=$(BUILD_PATH)/%.o)

DEPS = $(OBJECTS:.o=.d)

.PHONY: all dirs clean new

all: dirs $(BIN_NAME)

dirs:
	@echo "Creating directories..."
	mkdir -p $(dir $(OBJECTS))

$(BIN_NAME): $(OBJECTS)
	$(CXX) $^ $(LINK_FLAGS) -o $@
	@echo "Done"

-include $(DEPS)

$(BUILD_PATH)/%.o: $(SRC_PATH)/%.$(SRC_EXT)
	$(CXX) $(COMPILE_FLAGS) $(INCLUDES) -MP -MMD -c $< -o $@

test: 
	@echo $(SOURCES)
	@echo $(OBJECTS)

clean:
	@echo "Deleting..."
	@$(RM) $(BIN_NAME)
	@$(RM) -r $(BUILD_PATH)
	@echo "Done"

new: clean all
