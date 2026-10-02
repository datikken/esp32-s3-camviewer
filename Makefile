# Makefile — обёртка над CMake для сборки через make + gcc
# Использование:
#   make          — сборка (Release)
#   make debug    — сборка (Debug)
#   make run      — сборка + запуск
#   make clean    — очистка
#   make install  — установка в систему

BUILD_DIR  := build
TARGET     := $(BUILD_DIR)/esp32s3-cam-viewer
CMAKE_ARGS := -DCMAKE_BUILD_TYPE=Release -G "Unix Makefiles"

# URL потока по умолчанию (можно переопределить: make run URL=http://...)
URL       ?= http://192.168.4.1/stream

.PHONY: all debug run clean install rebuild

all: $(TARGET)

$(TARGET):
	@echo "[1/3] CMake configure..."
	cmake -B $(BUILD_DIR) $(CMAKE_ARGS) .
	@echo "[2/3] Building..."
	cmake --build $(BUILD_DIR) -j$$(nproc)
	@echo "[3/3] Done: $(TARGET)"

debug:
	cmake -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug -G "Unix Makefiles" .
	cmake --build $(BUILD_DIR) -j$$(nproc)

run: all
	env -u GTK_PATH ./$(TARGET) $(URL)

clean:
	rm -rf $(BUILD_DIR)

rebuild: clean all

install: all
	cmake --install $(BUILD_DIR)
