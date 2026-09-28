BUILD_DIR ?= build
DEBUG_BUILD_DIR ?= build-debug
CMAKE ?= cmake
JOBS ?= $(shell nproc 2>/dev/null || echo 4)

.PHONY: all configure build run debug clean reconfigure help

all: build

configure:
	$(CMAKE) -S . -B $(BUILD_DIR) -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release

build:
	@if [ ! -f "$(BUILD_DIR)/Makefile" ]; then $(MAKE) configure; fi
	$(CMAKE) --build $(BUILD_DIR) -j$(JOBS)

run: build
	./$(BUILD_DIR)/StudyBoard

debug:
	$(CMAKE) -S . -B $(DEBUG_BUILD_DIR) -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug
	$(CMAKE) --build $(DEBUG_BUILD_DIR) -j$(JOBS)
	./$(DEBUG_BUILD_DIR)/StudyBoard

reconfigure:
	rm -rf $(BUILD_DIR)
	$(MAKE) configure

clean:
	rm -rf $(BUILD_DIR) $(DEBUG_BUILD_DIR)

help:
	@printf '%s\n' \
	  'StudyBoard - comandos disponibles:' \
	  '  make              Compila en modo Release' \
	  '  make run          Compila lo necesario y abre StudyBoard' \
	  '  make debug        Compila y abre una build Debug' \
	  '  make reconfigure  Regenera la carpeta build desde cero' \
	  '  make clean        Elimina las carpetas de compilacion' \
	  '  make help         Muestra esta ayuda'
