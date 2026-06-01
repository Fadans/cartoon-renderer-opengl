CC        = g++
CXX_FLAGS = -Wall -Wextra -O2 -g -std=c++11 -m64 -Isrc -Ilib
LDFLAGS   = -lGL -lGLEW -lglut -lpthread

SRCS = src/main.cpp src/matrix4.cpp src/program.cpp \
       src/mesh.cpp src/camera.cpp src/texture.cpp   \
       lib/image.cpp lib/image_io.cpp

OBJS   = $(SRCS:.cpp=.o)
TARGET = cartoon

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(CXX_FLAGS) $(LDFLAGS)

%.o: %.cpp
	$(CC) -c -o $@ $< $(CXX_FLAGS)

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
