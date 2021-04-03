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

Use the included platform specific `build.sh` script. It takes the SuperCollider source code as its first argument.

```bash
cd mi-UGens
./<my_platform>-build.sh PATH/TO/SC/SOURCE/CODE
```

On Windows, use this [Git Bash terminal](https://git-scm.com/download/win) to run the above lines.



#### Building for mac

- clone the repository and its submodules

   `git clone --recurse-submodules https://github.com/v7b1/mi-UGens.git`

- cd into the directory of the project you want to build

- mkdir build

- cd build

and then:

- cmake -DSC_PATH="path/to/sc-src" -DCMAKE_BUILD_TYPE=RELEASE ..
- make



or use the included `mac-build.sh` script to build all projects at once. It takes the path to SuperCollider source code as its first argument

```bash
cd mi-UGens
./mac-build.sh PATH/TO/SC/SOURCE/CODE
```



Then step inside the newly created build folder and copy the Mi-UGens folder to your SuperCollider extensions.



#### Building for linux

Clone the repository and its submodules

`git clone --recurse-submodules https://github.com/v7b1/mi-UGens.git`

Use the included `linux-build.sh` script to build for linux. It takes the SuperCollider source code as its first argument.

```bash
cd mi-UGens
./linux-build.sh PATH/TO/SC/SOURCE/CODE
```



Collect the UGen files from the build folders of the projects and put them together with the sc folder into your SC extensions folder.



#### Building for windows

Clone the repository and its submodules:

`git clone --recurse-submodules https://github.com/v7b1/mi-UGens.git`

Use the included `win-build.sh` script. It takes the SuperCollider source code as its first argument.

Run this in a [Git Bash terminal](https://git-scm.com/download/win):

```bash
cd mi-UGens
bash ./win-build.sh PATH/TO/SC/SOURCE/CODE
```

Collect the UGen files from the `./build_artifacts` folder and put them together with the sc folder into your SC extensions folder.
