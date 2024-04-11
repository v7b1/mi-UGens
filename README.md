# mi-UGens

Some *mutable instruments* eurorack modules ported to [SuperCollider](https://supercollider.github.io/)

Thanks to Émilie Gillet for making the original source code available!
https://github.com/pichenettes/eurorack

**Please note, this is NOT a project by [mutable instruments](https://mutable-instruments.net/) !**



Volker Böhm, 2020
https://vboehm.net



## Installing

Go to the [release page](https://github.com/v7b1/mi-UGens/releases) , download the appropriate binaries for your OS and unpack the files into your SC extensions folder. (Re-)start SuperCollider or recompile the class library.

 

## Building

Clone the repository and its submodules:

`git clone --recurse-submodules https://github.com/v7b1/mi-UGens.git`



To build all projects/UGens in one go:

```bash
cd mi-UGens
mkdir build && cd build
cmake .. -DSC_PATH="path/to/SC/sources" -DCMAKE_BUILD_TYPE="Release"
make install
```

where `"path/to/SC/sources"` is your local path to the SuperCollider sources.

You should find a newly created folder `mi-UGens` in the build folder. (If you prefer a different location, you can specify an install directory with `-DCMAKE_INSTALL_PREFIX="path/to/my/folder"`). Copy this to your SC extensions folder and recompile the class library.

On Windows, use the [Git Bash terminal](https://git-scm.com/download/win) to run the above lines.



Single projects can be built by:

```bash
cd vb_UGens/projects/<whateverproject>
mkdir build && cd build
cmake .. -DSC_PATH="path/to/SC/sources" -DCMAKE_BUILD_TYPE="Release"
make
```

