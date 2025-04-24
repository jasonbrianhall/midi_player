CXX = g++
CC = gcc
CXXFLAGS = -Wall -Wextra -g $(shell sdl2-config --cflags)
CFLAGS = -Wall -Wextra -g $(shell sdl2-config --cflags)
LDFLAGS = $(shell sdl2-config --libs) -lm -pthread -lstdc++

SOURCES_CPP = dbopl.cpp dbopl_wrapper.cpp
SOURCES_C = main.c midiplayer.c instruments.c virtual_mixer.c wav_converter.c
OBJECTS_CPP = $(SOURCES_CPP:.cpp=.o)
OBJECTS_C = $(SOURCES_C:.c=.o)
OBJECTS = $(OBJECTS_CPP) $(OBJECTS_C)
EXECUTABLE = midiconverter

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(EXECUTABLE)
