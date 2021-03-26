#
# Build all projects
# Usage: mac-build.sh <SUPERCOLLIDER SOURCE>
#
SC_SRC=$1
FOLDERS=(MiBraids MiClouds MiElements MiMu MiOmi MiPlaits MiRings MiRipples MiTides MiVerb MiWarps)
MI_UGENS=build/mi-UGens

# MiBraids depends on libsamplerate, let's build that first
cd MiBraids/libsamplerate
echo "Building libsamplerate (without examples or testing)"
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DLIBSAMPLERATE_EXAMPLES=OFF -DBUILD_TESTING=OFF ..
make
cd ../../..

mkdir -p $MI_UGENS


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

	# # move binaries 
	mv $FOLDER.scx ../../$MI_UGENS

	# # Return
	cd ../..
done

cp -r sc $MI_UGENS
