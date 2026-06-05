CC        = g++
CXX_FLAGS = -Wall -Wextra -O2 -g -std=c++11 -m64 -Isrc -Ilib
LDFLAGS   = -lGL -lGLEW -lglut -lpthread

# Objets partagés par les deux apps — AUCUNE dépendance Assimp.
SHARED = src/matrix4.cpp src/program.cpp src/mesh.cpp src/camera.cpp src/texture.cpp
SHARED_OBJS = $(SHARED:.cpp=.o)

# `zelda` charge des modèles via Assimp → on ne le construit que s'il est installé,
# de sorte que `cartoon` (le désert) se construit TOUJOURS, même sans Assimp.
HAS_ASSIMP := $(shell pkg-config --exists assimp 2>/dev/null && echo yes)

all: cartoon zelda

cartoon: src/main.o $(SHARED_OBJS)
	$(CC) -o $@ $^ $(CXX_FLAGS) $(LDFLAGS)

ifeq ($(HAS_ASSIMP),yes)
zelda: src/main_zelda.o src/mesh_loader.o $(SHARED_OBJS)
	$(CC) -o $@ $^ $(CXX_FLAGS) $(LDFLAGS) -lassimp
else
zelda:
	@echo ">> Assimp introuvable : binaire 'zelda' non construit (sudo pacman -S assimp). 'cartoon' OK."
endif

%.o: %.cpp
	$(CC) -c -o $@ $< $(CXX_FLAGS)

clean:
	rm -f src/*.o cartoon zelda

.PHONY: all clean
