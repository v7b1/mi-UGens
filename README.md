# mi-UGens

Some *mutable instruments* eurorack modules ported to [SuperCollider](https://supercollider.github.io/)

Thanks to Émilie Gillet for making the original source code available!
https://github.com/pichenettes/eurorack

**Please note, this is NOT a project by [mutable instruments](https://mutable-instruments.net/) !**



Volker Böhm, 2020
https://vboehm.net



## Building

- cd into the directory of the project you want to build
- mkdir build
- cd build

and then:

- cmake -DSC_PATH="path/to/sc-src" -DCMAKE_BUILD_TYPE=RELEASE ..
- make

or, for creating an Xcode project:

- cmake -DSC_PATH="path/to/sc-src" -G Xcode ..
- and build with Xcode gui

For compiled mac versions see https://vboehm.net/downloads

## Building for linux

Use the included `linux-build.sh` script to build for linux. It takes the SuperCollider source code as it's first argument.

```bash
cd mi-UGens
./linux-build.sh PATH/TO/SC/SOURCE/CODE
```

