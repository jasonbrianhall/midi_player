#!/bin/bash

# Variables
DJGPP_IMAGE="djfdyuruiry/djgpp"
CSDPMI_URL="http://na.mirror.garr.it/mirrors/djgpp/current/v2misc/csdpmi7b.zip"
USER_ID=$(id -u)
GROUP_ID=$(id -g)
SOURCE_FILE="midiplayer.cpp"
DOS_TARGET="midipl.exe"

# Build MSDOS version using Docker
echo "Building MSDOS version of midiplayer..."

# Pull the DJGPP Docker image
echo "Pulling DJGPP Docker image..."
if ! docker pull ${DJGPP_IMAGE}; then
    echo "Failed to pull Docker image. Exiting."
    exit 1
fi

# Download CSDPMI if needed
if [ ! -d "csdpmi" ]; then
    echo "Downloading CSDPMI..."
    wget ${CSDPMI_URL} || { echo "Failed to download CSDPMI"; exit 1; }
    mkdir -p csdpmi
    unzip -o csdpmi7b.zip -d csdpmi || { echo "Failed to extract CSDPMI"; exit 1; }
fi

# Create a script to run inside Docker for building the application
cat > build_msdos.sh << EOF
#!/bin/bash
set -e

# Build the midiplayer application
cd /src
echo "Building midiplayer for MSDOS..."
# Add preprocessor define for MSDOS to handle any platform-specific code
g++ ${SOURCE_FILE} -o ${DOS_TARGET} -DMSDOS

echo "Build complete!"
EOF

# Make the script executable
chmod +x build_msdos.sh

# Create a temporary directory to avoid symlink issues
TEMP_BUILD_DIR=$(mktemp -d)
echo "Created temporary build directory: ${TEMP_BUILD_DIR}"

# Check if source file exists
if [ ! -f "${SOURCE_FILE}" ]; then
    echo "Error: Source file ${SOURCE_FILE} not found!"
    exit 1
fi

# Copy source file to the temporary directory
echo "Copying source file to temporary directory..."
cp -L "${SOURCE_FILE}" "${TEMP_BUILD_DIR}/" || { echo "Failed to copy source file"; exit 1; }

# Copy build script
cp build_msdos.sh "${TEMP_BUILD_DIR}/" || { echo "Failed to copy build script"; exit 1; }

# List files in the temporary directory for verification
echo "Files in temporary build directory:"
ls -la "${TEMP_BUILD_DIR}"

# Run the Docker container with the temporary directory
echo "Starting Docker build process..."
docker run --rm \
    -v "${TEMP_BUILD_DIR}:/src:z" \
    -u ${USER_ID}:${GROUP_ID} \
    -e SOURCE_FILE=${SOURCE_FILE} \
    -e DOS_TARGET=${DOS_TARGET} \
    ${DJGPP_IMAGE} /bin/bash -c "chmod +x /src/build_msdos.sh && /src/build_msdos.sh"

# Copy back the built files
echo "Copying built files from temporary directory..."
if [ -f "${TEMP_BUILD_DIR}/${DOS_TARGET}" ]; then
    cp "${TEMP_BUILD_DIR}/${DOS_TARGET}" ./ || echo "Failed to copy executable"
else
    echo "Build failed: ${DOS_TARGET} not found in temporary directory"
    exit 1
fi

# Clean up
echo "Cleaning up temporary directory..."
rm -rf "${TEMP_BUILD_DIR}"
rm -f build_msdos.sh

# Ensure we have the CSDPMI executable in the current directory
if [ -f "csdpmi/bin/CWSDPMI.EXE" ]; then
    cp csdpmi/bin/CWSDPMI.EXE . || echo "Failed to copy CWSDPMI.EXE"
fi

# Check if build was successful
if [ -f "${DOS_TARGET}" ]; then
    echo "MSDOS build successful! Files created:"
    echo "- ${DOS_TARGET}"
    echo "- CWSDPMI.EXE"
    echo "To run in DOSBox, execute: dosbox ${DOS_TARGET}"
else
    echo "MSDOS build failed."
    exit 1
fi
