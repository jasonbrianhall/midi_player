# Makefile for MIDI Player MS-DOS build with DPMI support
VERSION=1.0

# Source files (C and C++ files)
C_SRCS = main.c midiplayer.c instruments.c sb_audio.c
CPP_SRCS = dbopl_wrapper.cpp dbopl.cpp
SRCS = $(C_SRCS) $(CPP_SRCS)

# Output executable
DOS_TARGET = sbmidi.exe
DOS_COFF = sbmidi

# Docker image for DJGPP
DJGPP_IMAGE = djfdyuruiry/djgpp

# Correct CSDPMI URL
CSDPMI_URL = http://na.mirror.garr.it/mirrors/djgpp/current/v2misc/csdpmi7b.zip

# Get current user and group IDs for Docker
USER_ID = $(shell id -u)
GROUP_ID = $(shell id -g)

# Default target
all: msdos

# Target to pull the DJGPP Docker image
pull-djgpp:
	@echo "Pulling DJGPP Docker image..."
	docker pull $(DJGPP_IMAGE)

# Target to download CSDPMI
get-csdpmi:
	@echo "Downloading CSDPMI..."
	wget $(CSDPMI_URL)
	mkdir -p csdpmi
	unzip -o csdpmi7b.zip -d csdpmi

# Target to build for MS-DOS using DJGPP in Docker
msdos: pull-djgpp get-csdpmi
	@echo "Building MSDOS version of SoundBlaster MIDI Player..."
	docker run --rm -v $(PWD):/src:z -u $(USER_ID):$(GROUP_ID) $(DJGPP_IMAGE) /bin/sh -c "cd /src && \
	g++ -s $(SRCS) -o $(DOS_TARGET) -DMSDOS -O6 -lm && \
	exe2coff $(DOS_TARGET) && \
	cat csdpmi/bin/CWSDSTUB.EXE $(DOS_COFF) > $(DOS_TARGET)"
	@echo "Build complete!"
	@echo "- $(DOS_TARGET)"
	@echo "- CWSDPMI.EXE"
	cp csdpmi/bin/CWSDPMI.EXE ./
	@echo "To run in DOSBox, execute: dosbox $(DOS_TARGET)"
	
# Target to run the MS-DOS executable using DOSBox
run: msdos
	dosbox $(DOS_TARGET)

# Unix build target - builds for Unix/Linux systems
unix:
	@echo "Building Unix version of SoundBlaster MIDI Player..."
	gcc $(C_SRCS) -c $(CFLAGS)
	g++ $(CPP_SRCS) -c $(CXXFLAGS)
	g++ *.o -o midiplayer -lm -lpthread
	@echo "Unix build complete!"

# Clean target to remove generated files
clean:
	rm -f $(DOS_TARGET) csdpmi7b.zip || true
	rm -rf csdpmi || true
	rm -f *.exe *.EXE *.o *.zip *.zip.* $(DOS_COFF) midiplayer || true

.PHONY: all pull-djgpp get-csdpmi msdos run unix clean
