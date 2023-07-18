CC := g++
CFLAGS := -std=c++20 -Wall -g
BUILD_DIR := build
SRCS := $(wildcard *.cpp)

TARGET := client

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(addprefix $(BUILD_DIR)/,$(SRCS:%.cpp=%.o))
	$(CC) $^ -lcurl -lcryptopp -lfmt -o $@

$(BUILD_DIR)/%.o: %.cpp | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR) $(TARGET)