# Makefile for MIDI to WAV Converter MS-DOS build with DPMI support
VERSION=1.0

# Source files
CPP_SRCS = 
C_SRCS = waveplayer.c

# Output executable
DOS_TARGET = waveplay.exe
DOS_COFF = waveplay

# Docker image for DJGPP
DJGPP_IMAGE = djfdyuruiry/djgpp

# Correct CSDPMI URL
CSDPMI_URL = http://na.mirror.garr.it/mirrors/djgpp/current/v2misc/csdpmi7b.zip

# Get current user and group IDs for Docker
USER_ID = $(shell id -u)
GROUP_ID = $(shell id -g)

# DJGPP cross-compiler prefix
PREFIX = i586-pc-msdosdjgpp

# Compiler flags
CFLAGS = -Wall -O2
CXXFLAGS = -Wall -O2 -fpermissive
INCLUDES = -I.

# Default target
all: msdos

# Target to pull the DJGPP Docker image
pull-djgpp:
	@echo "Pulling DJGPP Docker image..."
	docker pull $(DJGPP_IMAGE)

# Target to download CSDPMI
get-csdpmi:
	@echo "Downloading CSDPMI..."
	wget -N $(CSDPMI_URL)
	mkdir -p csdpmi
	unzip -o csdpmi7b.zip -d csdpmi

# Target to build for MS-DOS using DJGPP in Docker
msdos: pull-djgpp get-csdpmi
	@echo "Building MSDOS version of MIDI to WAV converter..."
	docker run --rm -v $(PWD):/src:z -u $(USER_ID):$(GROUP_ID) $(DJGPP_IMAGE) /bin/sh -c "cd /src && \
	$(PREFIX)-g++ $(CXXFLAGS) $(INCLUDES) $(CPP_SRCS) $(C_SRCS) -o $(DOS_TARGET) -DMSDOS && \
	exe2coff $(DOS_TARGET) && \
	cat csdpmi/bin/CWSDSTUB.EXE $(DOS_COFF) > $(DOS_TARGET)"
	rm $(DOS_COFF)
	@echo "Build complete!"
	@echo "- $(DOS_TARGET)"
	@echo "- CWSDPMI.EXE"
	cp csdpmi/bin/CWSDPMI.EXE ./

# Target to build without Docker (if you have DJGPP installed locally)
local-build:
	@echo "Building with local DJGPP..."
	$(PREFIX)-g++ $(CXXFLAGS) $(INCLUDES) $(CPP_SRCS) $(C_SRCS) -o $(DOS_TARGET) -DMSDOS
	exe2coff $(DOS_TARGET)
	cat csdpmi/bin/CWSDSTUB.EXE $(DOS_COFF) > $(DOS_TARGET)

# Target to create a distribution ZIP
dist: msdos
	@echo "Creating distribution ZIP..."
	mkdir -p dist
	cp $(DOS_TARGET) dist/
	cp CWSDPMI.EXE dist/
	cp CONVERT.BAT dist/
	cp README.DJGPP dist/README.TXT
	cd dist && zip -9 ../midiconv.zip *
	@echo "Created midiconv.zip"

# Target to run the MS-DOS executable using DOSBox
run: msdos
	dosbox $(DOS_TARGET)

# Clean target to remove generated files
clean:
	rm -f $(DOS_TARGET) csdpmi7b.zip || true
	rm -rf csdpmi || true
	rm -f *.exe *.EXE *.o *.zip *.zip.* $(DOS_COFF) || true

.PHONY: all pull-djgpp get-csdpmi msdos local-build dist run clean
