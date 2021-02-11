#
# Build all projects
# Usage: win-build.sh <SUPERCOLLIDER SOURCE>
#
set -euo pipefail

SC_SRC=$1
FOLDERS=(MiClouds MiElements MiMu MiOmi MiPlaits MiRings MiRipples MiTides MiVerb MiWarps)
# FIXME MiBraids currently unsupported because building libsamplerate is hard on Windows

mkdir build_artifacts

for FOLDER in "${FOLDERS[@]}"
do
	cd $FOLDER

	echo "Building $FOLDER"

	# # Build folder
	mkdir build 
	cd build

	# # Build
	cmake -DSC_PATH=$SC_SRC ..
	cmake --build . --config Release
    cp Release/${FOLDER}.scx ../../build_artifacts

	# # Return
	cd ../..
done
