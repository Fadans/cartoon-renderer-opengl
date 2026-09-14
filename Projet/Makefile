CC        = g++
CXX_FLAGS = -Wall -Wextra -O2 -g -std=c++11 -m64 -Isrc -Ilib
LDFLAGS   = -lGL -lGLEW -lglut -lpthread

# Objets partagés par les trois scènes — AUCUNE dépendance Assimp.
SHARED = src/matrix4.cpp src/program.cpp src/mesh.cpp src/camera.cpp src/texture.cpp
SHARED_OBJS = $(SHARED:.cpp=.o)

# Tous les en-têtes : modifier un .hh (ex. style.hh) force la recompilation des .o.
HEADERS = $(wildcard src/*.hh)

# `pont` charge un glTF via Assimp → on ne le construit que s'il est installé,
# de sorte que `desert` et `meshes` se construisent TOUJOURS, même sans Assimp.
HAS_ASSIMP := $(shell pkg-config --exists assimp 2>/dev/null && echo yes)

all: desert meshes pont

# Scène désert (buttes, herbe, rochers, sol + ombres) — primitives générées.
desert: src/main_desert.o $(SHARED_OBJS)
	$(CC) -o $@ $^ $(CXX_FLAGS) $(LDFLAGS)

# Scène mesh simple (sphère/cylindre/tore + sol + ombres).
meshes: src/main_meshes.o $(SHARED_OBJS)
	$(CC) -o $@ $^ $(CXX_FLAGS) $(LDFLAGS)

ifeq ($(HAS_ASSIMP),yes)
# Scène pont japonais (assets/maquette/scene.gltf) — chargée via Assimp.
pont: src/main_pont.o src/mesh_loader.o $(SHARED_OBJS)
	$(CC) -o $@ $^ $(CXX_FLAGS) $(LDFLAGS) -lassimp
else
pont:
	@echo ">> Assimp introuvable : binaire 'pont' non construit (sudo pacman -S assimp). 'desert'/'meshes' OK."
endif

%.o: %.cpp $(HEADERS)
	$(CC) -c -o $@ $< $(CXX_FLAGS)

clean:
	rm -f src/*.o desert meshes pont

.PHONY: all clean