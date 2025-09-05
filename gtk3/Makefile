CXX = g++
CC = gcc
CXXFLAGS = -Wall -Wextra -g $(shell sdl2-config --cflags) $(shell pkg-config --cflags gtk+-3.0 vorbisfile) -fpermissive
CFLAGS = -Wall -Wextra -g $(shell sdl2-config --cflags) $(shell pkg-config --cflags gtk+-3.0 vorbisfile)
LDFLAGS = $(shell sdl2-config --libs) $(shell pkg-config --libs gtk+-3.0 vorbisfile) -lSDL2_mixer -lm -pthread -lstdc++ -lFLAC

SOURCES_CPP = dbopl.cpp dbopl_wrapper.cpp main.cpp midiplayer.cpp instruments.cpp virtual_mixer.cpp wav_converter.cpp audioconverter.cpp convertmidi.cpp convertoggtowav.cpp vfs.cpp cache.cpp convertflactowav.cpp
SOURCES_C = 
OBJECTS_CPP = $(SOURCES_CPP:.cpp=.o)
OBJECTS_C = $(SOURCES_C:.c=.o)
OBJECTS = $(OBJECTS_CPP) $(OBJECTS_C)
EXECUTABLE = gtk-musicplayer

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(EXECUTABLE)

install: $(EXECUTABLE)
	install -D $(EXECUTABLE) $(DESTDIR)/usr/local/bin/$(EXECUTABLE)

.PHONY: all clean install
