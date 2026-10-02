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
PLATFORMIO ?= $(shell command -v platformio 2>/dev/null || printf '%s' "$(HOME)/.platformio/penv/bin/platformio")
PIO_ENV ?= esp32-s3-cam

# URL потока по умолчанию (можно переопределить: make run URL=http://...)
URL       ?= http://192.168.4.1/stream

.PHONY: all build debug run camera clean install rebuild

all: build

build:
	@echo "[1/3] CMake configure..."
	cmake -S . -B $(BUILD_DIR) $(CMAKE_ARGS)
	@echo "[2/3] Building..."
	cmake --build $(BUILD_DIR) --parallel $$(nproc)
	@echo "[3/3] Done: $(TARGET)"

debug:
	cmake -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug -G "Unix Makefiles" .
	cmake --build $(BUILD_DIR) -j$$(nproc)

run: build
	env -u GTK_PATH ./$(TARGET) $(URL)

camera:
	$(PLATFORMIO) run --environment $(PIO_ENV) --target upload
	$(PLATFORMIO) device monitor --environment $(PIO_ENV)

clean:
	rm -rf $(BUILD_DIR)

rebuild: clean build

install: build
	cmake --install $(BUILD_DIR)
