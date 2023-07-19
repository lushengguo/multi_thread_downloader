# CC := g++
# CFLAGS := -std=c++20 -Wall -g
# BUILD_DIR := build
# CPP_FILES := $(shell find . -type f -name '*.cpp')
# OBJ_FILES := $(addprefix $(BUILD_DIR)/,$(notdir $(CPP_FILES:.cpp=.o)))

# TARGET := client

# all: $(TARGET)

# $(BUILD_DIR):
# 	mkdir -p $(BUILD_DIR)

# $(BUILD_DIR)/%.o: %.cpp
# 	$(CC) $(CFLAGS) -c $< -o $@

# $(TARGET): $(OBJ_FILES)
# 	$(CC) $^ -lcurl -lcryptopp -lfmt -o $@

# clean:
# 	rm -rf $(BUILD_DIR) $(TARGET)

CXX = g++
CFLAGS = -g -std=c++2a -Wall
LIBS  = -lcurl -lcryptopp -lfmt
TARGET = client

SRCS = $(shell find . -type f -name '*.cpp')
OBJS = $(patsubst %.cpp,%.o,$(SRCS))

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $^ $(LIBS) -o $@

%.o: %.cpp
	$(CXX) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)