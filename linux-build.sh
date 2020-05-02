SC_SRC=$HOME/Downloads/supercollider
FOLDERS=(MiBraids MiClouds MiElements MiMu MiPlaits MiRings MiVerb)

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
