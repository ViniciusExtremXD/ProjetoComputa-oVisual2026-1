# =============================================================================
# MAKEFILE - PROCESSAMENTO DE IMAGENS C++17
# =============================================================================
#
# Universidade Presbiteriana Mackenzie - Ciencia da Computacao
# Disciplina: Computacao Visual
#
# Integrantes:
#   Rodrigo Rosalles - 10409316
#   Vinicius Magno   - 10401365
#   Natalia Teixeira - 10395853
#
# Compilacao para C++17 com SDL3, SDL3_image e SDL3_ttf.
# Compativel com Linux (make) e Windows (mingw32-make / MSYS2).
#
# Uso:
#   make                      - Compilar
#   make run IMAGE=path       - Executar modo GUI
#   make test IMAGE=path      - Testar com imagem especifica
#   make test-headless IMAGE=path - Testar modo headless
#   make clean                - Limpar build
#   make clean-all            - Limpar artefatos gerados
#   make help                 - Ver todos os targets
#
# =============================================================================

# =============================================================================
# DETECCAO DO SISTEMA OPERACIONAL
# =============================================================================

ifeq ($(OS),Windows_NT)
    PLATFORM := Windows
    EXE_EXT  := .exe
else
    PLATFORM := Linux
    EXE_EXT  :=
endif

# =============================================================================
# COMPILADORES E FLAGS
# =============================================================================

CXX = g++
CC  = gcc

CXXFLAGS = -std=c++17 -Wall -Wextra -g -O2 \
           -Wshadow -Wnon-virtual-dtor \
           -Wcast-align -Wunused -Woverloaded-virtual \
           -Wmisleading-indentation -Wduplicated-cond \
           -Wduplicated-branches -Wlogical-op \
           -Wnull-dereference -Wformat=2

CFLAGS = -Wall -Wextra -g -O2

# =============================================================================
# CONFIGURACAO SDL3
# =============================================================================

ifeq ($(PLATFORM),Windows)
    # No Windows, pkg-config e tentado primeiro (disponivel no MSYS2/WinLibs).
    # Se nao estiver disponivel, ajustar SDL_ROOT conforme localizacao do SDL3.
    SDL_ROOT    ?= C:/winlibs
    SDL_CFLAGS  := $(shell pkg-config --cflags sdl3 sdl3-image sdl3-ttf 2>nul || echo -I$(SDL_ROOT)/include)
    SDL_LIBS    := $(shell pkg-config --libs   sdl3 sdl3-image sdl3-ttf 2>nul || echo -L$(SDL_ROOT)/lib -lSDL3 -lSDL3_image -lSDL3_ttf)
    LDFLAGS             :=
    SYSTEM_INCLUDE_DIRS :=
    SYSTEM_LIB_DIRS     :=
else
	PKG_CONFIG_PATH ?= /usr/local/lib/pkgconfig:/usr/lib/x86_64-linux-gnu/pkgconfig
	SDL_CFLAGS  := $(shell PKG_CONFIG_PATH="$(PKG_CONFIG_PATH)" pkg-config --cflags sdl3 sdl3-image sdl3-ttf)
	SDL_LIBS    := $(shell PKG_CONFIG_PATH="$(PKG_CONFIG_PATH)" pkg-config --libs   sdl3 sdl3-image sdl3-ttf)
    LDFLAGS             := -Wl,-rpath,/usr/local/lib -Wl,--enable-new-dtags
    SYSTEM_INCLUDE_DIRS := -I/usr/local/include -I/usr/include/freetype2 -I/usr/include/libpng16
    SYSTEM_LIB_DIRS     := -L/usr/local/lib
endif

# =============================================================================
# ESTRUTURA DE DIRETORIOS
# =============================================================================

SRC_DIR     = src
BUILD_DIR   = build
INCLUDE_DIR = include
SOURCES_CPP = $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS_CPP = $(SOURCES_CPP:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)

TARGET = $(BUILD_DIR)/main$(EXE_EXT)

# =============================================================================
# TARGETS PRINCIPAIS
# =============================================================================

.PHONY: all run test test-headless debug valgrind clean clean-all info check-deps banner help

all: banner $(TARGET)
	@echo Build concluido: $(TARGET)

$(TARGET): $(OBJECTS_CPP) | $(BUILD_DIR)
	@echo Linkando executavel...
	$(CXX) $(OBJECTS_CPP) -o $@ $(SYSTEM_LIB_DIRS) $(SDL_LIBS) $(LDFLAGS)
	@echo Executavel criado: $@

# =============================================================================
# COMPILACAO
# =============================================================================

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	@echo Compilando: $<
	$(CXX) $(CXXFLAGS) $(SYSTEM_INCLUDE_DIRS) -I$(INCLUDE_DIR) $(SDL_CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# =============================================================================
# EXECUCAO E TESTES
# =============================================================================

run: $(TARGET)
	@echo Executando em modo GUI...
	@if [ -z "$(IMAGE)" ]; then \
		echo "Uso: make run IMAGE=caminho/para/imagem.png"; \
		exit 1; \
	fi; \
	./$(TARGET) $(IMAGE)

test: $(TARGET)
	@if [ -z "$(IMAGE)" ]; then \
		echo "Uso: make test IMAGE=caminho/para/imagem.png"; \
		echo "     make test IMAGE=imagem.png NOGUI=1  (modo headless)"; \
	elif [ "$(NOGUI)" = "1" ]; then \
		./$(TARGET) --nogui $(IMAGE); \
	else \
		./$(TARGET) $(IMAGE); \
	fi

test-headless: $(TARGET)
	@echo Testando modo headless...
	@if [ -z "$(IMAGE)" ]; then \
		echo "Uso: make test-headless IMAGE=caminho/para/imagem.png"; \
		exit 1; \
	fi; \
	./$(TARGET) --nogui $(IMAGE)

debug: $(TARGET)
	@if [ -z "$(IMAGE)" ]; then \
		echo "Uso: make debug IMAGE=caminho/para/imagem.png"; \
		exit 1; \
	fi; \
	gdb --args ./$(TARGET) $(IMAGE)

valgrind: $(TARGET)
	@if [ -z "$(IMAGE)" ]; then \
		echo "Uso: make valgrind IMAGE=caminho/para/imagem.png"; \
		exit 1; \
	fi; \
	valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET) --nogui $(IMAGE)

# =============================================================================
# LIMPEZA
# =============================================================================

clean:
	@echo Limpando build...
	rm -rf $(BUILD_DIR)

clean-all: clean
	@echo Limpeza completa...
	rm -f output_image.png
	find . -maxdepth 1 -name "output_*" -type d -exec rm -rf {} + 2>/dev/null || true

# =============================================================================
# INFORMACOES E DIAGNOSTICOS
# =============================================================================

info:
	@echo Plataforma: $(PLATFORM)
	@echo Executavel: $(TARGET)
	@echo SDL CFLAGS: $(SDL_CFLAGS)
	@echo Fontes: $(SOURCES_CPP)

check-deps:
ifeq ($(PLATFORM),Linux)
	@pkg-config --exists sdl3       || (echo "SDL3 nao encontrado" && exit 1)
	@pkg-config --exists sdl3-image || (echo "SDL3_image nao encontrado" && exit 1)
	@pkg-config --exists sdl3-ttf   || (echo "SDL3_ttf nao encontrado" && exit 1)
	@echo Todas as dependencias encontradas
else
	@echo Windows: verificar SDL_ROOT=$(SDL_ROOT)
endif

banner:
	@echo ==============================================================
	@echo  Processamento de Imagens - C++17 com SDL3 - $(PLATFORM)
	@echo  Universidade Presbiteriana Mackenzie - Computacao Visual
	@echo  Rodrigo Rosalles 10409316 / Vinicius Magno 10401365 / Natalia Teixeira 10395853
	@echo ==============================================================

help:
	@echo TARGETS DISPONIVEIS:
	@echo   make                      - Compilar
	@echo   make run IMAGE=f          - Executar GUI com imagem f (obrigatorio)
	@echo   make test IMAGE=f         - Testar com imagem f
	@echo   make test IMAGE=f NOGUI=1 - Testar modo headless
	@echo   make test-headless IMAGE=f - Testar headless com imagem f (obrigatorio)
	@echo   make debug                - Executar com GDB
	@echo   make valgrind             - Analise de memoria
	@echo   make clean                - Limpar build
	@echo   make clean-all            - Limpeza completa
	@echo   make info                 - Info do sistema
	@echo   make check-deps           - Verificar dependencias

# =============================================================================
# CONFIGURACOES ESPECIAIS
# =============================================================================

.DEFAULT_GOAL := all
.SILENT:
