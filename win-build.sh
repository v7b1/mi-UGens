#
# Build all projects
# Usage: win-build.sh <SUPERCOLLIDER SOURCE>
#
set -euo pipefail

SC_SRC=$1
FOLDERS=(MiBraids MiClouds MiElements MiMu MiOmi MiPlaits MiRings MiRipples MiTides MiVerb MiWarps)

mkdir -p build_artifacts

echo "Building libsamplerate"
cd MiBraids/libsamplerate
mkdir -p build
cd build
cmake ..
cmake --build . --config Release
cd ../../..

for FOLDER in "${FOLDERS[@]}"
do
	cd $FOLDER

	echo "Building $FOLDER"

	# # Build folder
	mkdir -p build
	cd build

	# # Build
	cmake -DSC_PATH=$SC_SRC ..
	cmake --build . --config Release
    cp Release/${FOLDER}.scx ../../build_artifacts

	# # Return
	cd ../..
done
