#!/bin/bash

# Variables
DJGPP_IMAGE="djfdyuruiry/djgpp"
CSDPMI_URL="http://na.mirror.garr.it/mirrors/djgpp/current/v2misc/csdpmi7b.zip"
USER_ID=$(id -u)
GROUP_ID=$(id -g)
DOS_TARGET="midiplayer.exe"

# Build MSDOS version using Docker
echo "Building MSDOS version of midiplayer..."

# Pull the DJGPP Docker image
echo "Pulling DJGPP Docker image..."
docker pull ${DJGPP_IMAGE}

# Download CSDPMI if needed
if [ ! -d "csdpmi" ]; then
    echo "Downloading CSDPMI..."
    wget ${CSDPMI_URL}
    mkdir -p csdpmi
    unzip -o csdpmi7b.zip -d csdpmi
fi

# Create a script to run inside Docker for building the application
cat > build_msdos.sh << 'EOF'
#!/bin/bash
set -e

# Build the midiplayer application
cd /src
echo "Building midiplayer for MSDOS..."
# Add preprocessor define for MSDOS to handle any platform-specific code
g++ midiplayer.cpp -o midiplayer.exe -DMSDOS

echo "Build complete!"
EOF

# Make the script executable
chmod +x build_msdos.sh

# Create a temporary directory to avoid symlink issues
TEMP_BUILD_DIR=$(mktemp -d)
echo "Created temporary build directory: ${TEMP_BUILD_DIR}"

# Copy midiplayer.cpp directly
cp -L midiplayer.cpp "${TEMP_BUILD_DIR}/"

# Copy build script
cp build_msdos.sh "${TEMP_BUILD_DIR}/"

# List files in the temporary directory for verification
echo "Files in temporary build directory:"
ls -la "${TEMP_BUILD_DIR}"

# Run the Docker container with the temporary directory
echo "Starting Docker build process..."
docker run --rm -v "${TEMP_BUILD_DIR}:/src:z" -u ${USER_ID}:${GROUP_ID} ${DJGPP_IMAGE} /src/build_msdos.sh

# Copy back the built files
echo "Copying built files from temporary directory..."
cp "${TEMP_BUILD_DIR}/${DOS_TARGET}" ./ 2>/dev/null || echo "Failed to copy executable"

# Clean up
echo "Cleaning up temporary directory..."
rm -rf "${TEMP_BUILD_DIR}"

# Ensure we have the CSDPMI executable in the current directory
if [ -f "csdpmi/bin/CWSDPMI.EXE" ]; then
    cp csdpmi/bin/CWSDPMI.EXE .
fi

# Check if build was successful
if [ -f "${DOS_TARGET}" ]; then
    echo "MSDOS build successful! Files created:"
    echo "- ${DOS_TARGET}"
    echo "- CWSDPMI.EXE"
    echo "To run in DOSBox, execute: dosbox ${DOS_TARGET}"
else
    echo "MSDOS build failed."
fi
