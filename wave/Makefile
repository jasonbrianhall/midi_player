# Makefile for MIDI Player MS-DOS build with DPMI support
VERSION=1.0

# Source files (automatically detect all .c files)
SRCS = $(wildcard *.c)

# Output executable
DOS_TARGET = wave.exe
DOS_COFF = wave

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
	@echo "Building MSDOS version of midiplayer..."
	docker run --rm -v $(PWD):/src:z -u $(USER_ID):$(GROUP_ID) $(DJGPP_IMAGE) /bin/sh -c "cd /src && \
	g++ -s $(SRCS) -o $(DOS_TARGET) -DMSDOS -O6 && \
	exe2coff $(DOS_TARGET) && \
	cat csdpmi/bin/CWSDSTUB.EXE $(DOS_COFF) > $(DOS_TARGET)"
	rm $(DOS_COFF)
	@echo "Build complete!"
	@echo "- $(DOS_TARGET)"
	@echo "- CWSDPMI.EXE"
	cp csdpmi/bin/CWSDPMI.EXE ./
	@echo "To run in DOSBox, execute: dosbox $(DOS_TARGET)"
	
# Target to run the MS-DOS executable using DOSBox
run: msdos
	dosbox $(DOS_TARGET)

# Clean target to remove generated files
clean:
	rm -f $(DOS_TARGET) csdpmi7b.zip || true
	rm -rf csdpmi || true
	rm -f *.exe *.EXE *.o *.zip *.zip.* $(DOS_COFF) || true

.PHONY: all pull-djgpp get-csdpmi msdos run clean
