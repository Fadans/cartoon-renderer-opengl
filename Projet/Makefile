CC        = g++
CXX_FLAGS = -Wall -Wextra -O2 -g -std=c++11 -m64 -Isrc -Ilib
LDFLAGS   = -lGL -lGLEW -lglut -lpthread -lassimp

# Code partagé par les deux applications (désert + Zelda)
SHARED = src/matrix4.cpp src/program.cpp src/mesh.cpp src/camera.cpp src/texture.cpp
SHARED_OBJS = $(SHARED:.cpp=.o)

# Deux binaires à la racine : cartoon (désert) et zelda (visualiseur de modèles)
all: cartoon zelda

cartoon: src/main.o $(SHARED_OBJS)
	$(CC) -o $@ $^ $(CXX_FLAGS) $(LDFLAGS)

zelda: src/main_zelda.o $(SHARED_OBJS)
	$(CC) -o $@ $^ $(CXX_FLAGS) $(LDFLAGS)

%.o: %.cpp
	$(CC) -c -o $@ $< $(CXX_FLAGS)

clean:
	rm -f src/*.o cartoon zelda

.PHONY: all clean
