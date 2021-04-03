#
# Build all projects
# Usage: build.sh <SUPERCOLLIDER SOURCE>
#
SC_SRC=$1
FOLDERS=(MiBraids MiClouds MiElements MiMu MiOmi MiPlaits MiRings MiRipples MiTides MiVerb MiWarps)
MI_UGENS=build/mi-UGens

mkdir -p $MI_UGENS

# MiBraids depends on libsamplerate, let's build that first
cd MiBraids/libsamplerate
echo "Building libsamplerate"
mkdir -p build
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
	cmake -DSC_PATH=$SC_SRC -DCMAKE_BUILD_TYPE=Release ..
	cmake --build . --config Release

	# # copy binaries 
	if [[ "$OSTYPE" == "darwin"* ]] ; then
		cp $FOLDER.scx ../../$MI_UGENS
    elif [[ "$OSTYPE" == "linux-gnu"* ]] ; then
        cp $FOLDER.so ../../$MI_UGENS
	else 
		cp Release/${FOLDER}.scx ../../$MI_UGENS
	fi

	# # Return
	cd ../..
done

cp -r sc/* $MI_UGENS
