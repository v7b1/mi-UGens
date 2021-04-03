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

Use the included  `build.sh` script. It takes the SuperCollider source code as its first argument.

```bash
cd mi-UGens
./build.sh PATH/TO/SC/SOURCE/CODE
```

On Windows, use the [Git Bash terminal](https://git-scm.com/download/win) to run the above lines.

Find the newly created mi-UGens folder in the build folder and copy it to your SC extensions folder.
