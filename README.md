# mi-UGens

Some *mutable instruments* eurorack modules ported to [SuperCollider](https://supercollider.github.io/)

Thanks to Émilie Gillet for making the original source code available!
https://github.com/pichenettes/eurorack

**Please note, this is NOT a project by [mutable instruments](https://mutable-instruments.net/) !**

Volker Böhm, 2020
https://vboehm.net

## Installation

### Build libraries

Clone the repository and its submodules
```bash
git clone --recurse-submodules https://github.com/v7b1/mi-UGens.git
```

#### Mac OS X
- cd into the directory of the project you want to build
- `mkdir build`
- `cd build`

and then:

- `cmake -DSC_PATH="path/to/sc-src" -DCMAKE_BUILD_TYPE=RELEASE ..`
- `make`

or use the included `mac-build.sh` script to build all projects at once. It takes the path to SuperCollider source code as its first argument

```bash
cd mi-UGens
./mac-build.sh PATH/TO/SC/SOURCE/CODE
```

Then step inside the newly created build folder and copy the Mi-UGens folder to your SuperCollider extensions.

For compiled mac versions see https://vboehm.net/downloads

#### Linux

Use the included `linux-build.sh` script to build for linux. It takes the SuperCollider source code as its first argument.

```bash
cd mi-UGens
./linux-build.sh PATH/TO/SC/SOURCE/CODE
```


Collect the UGen files from the build folders of the projects and put them into your SC extensions folder.

### Install Quark

From Supercollider execute:
```
Quarks.install("https://github.com/v7b1/mi-UGens")
```