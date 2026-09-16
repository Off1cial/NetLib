.PHONY: all client server run-client run-server clean rebuild

BUILD_DIR := build

all: client server

client:
	cmake -S . -B $(BUILD_DIR)
	cmake --build $(BUILD_DIR) --target client

server:
	cmake -S . -B $(BUILD_DIR)
	cmake --build $(BUILD_DIR) --target server

run-client: client
	./$(BUILD_DIR)/client

run-server: server
	./$(BUILD_DIR)/server

clean:
	rm -rf $(BUILD_DIR)

rebuild: clean all
