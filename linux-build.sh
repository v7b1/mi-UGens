#
# Build all projects
# Usage: linux-build.sh <SUPERCOLLIDER SOURCE>
#
SC_SRC=$1
FOLDERS=(MiBraids MiClouds MiElements MiMu MiOmi MiPlaits MiRings MiRipples MiTides MiVerb MiWarps)

# MiBraids depends on libsamplerate, let's build that first
cd MiBraids/libsamplerate
echo "Building libsamplerate"
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DLIBSAMPLERATE_EXAMPLES=OFF -DBUILD_TESTING=OFF ..
make
cd ../../..

for FOLDER in "${FOLDERS[@]}"
do
	cd $FOLDER

	echo "Building $FOLDER"

	# # Build folder
	mkdir build 
	cd build

	# # Build
	cmake -DSC_PATH=$SC_SRC -DCMAKE_BUILD_TYPE=RELEASE ..
	make

	# # Return
	cd ../..
done
