# xfemm based CoilGun coil simulator
This is my simulator for simulating multiple coils using multiple threads.
I'm designing super-optimized coilgun, and I needed to compare hundreds of coils, so I made this.

![image](https://github.com/Erdroy/CoilGun-coil-simulator/assets/7634316/84388012-2b7f-4bd5-ad6e-1ed5a961d53d)
Simulation can be stopped, program will resume where it ended.
Source code is here: [coilgunsim](https://github.com/Erdroy/CoilGun-coil-simulator/tree/master/cfemm/coilgunsim)

Note that there is a bug with multithreading, when simulating less coils than amount of threads.
It says that the simulation is finished, but it still simulates the coils. Workaround is to reduce amount of threads if you feel annoyed with it.

# How to use (Windows, Release version):
- Download latest version from Release tab,
- Unzip it somewhere,
- Adjust config.json to match your needs,
- Open `coilgunsim.exe` and it will start the simulation. This will output data in Data folder.
- Open Data folder and take a look at .csv and .json files. It's the data, do whatever you want with it (simulate, graph forces etc.)

# Data format
- Naming: 0.9_C15x100T-P8.0x30, this means 0.9mm wire, C15 is the length of the coil (15mm), 100T is the amount of turns, P8.0 is the diameter of the coil, x30 is the length of the coil.
- csv file contains data like this:
```
Distance, Inductance, Force@10A, Force@100A, Force@1000A 
0.000000, 390.797750, 0.008886, 0.130171, 0.596539
1.000000, 390.824651, 0.130918, 2.538926, 23.916522
2.000000, 386.938488, 0.238714, 4.885965, 48.689247
3.000000, 382.931224, 0.363702, 7.692373, 79.129892
...
```
Each line is the distance from center of the coil to center of the projectile. There are different forces (Newtons) at given currents and the inductance (uH/Microhenry) at a point.
- JSON file should be self-explanatory.

# Build (Windows)
- Install CMake and Visual Studio 2022,
- Clone repo to a new directory,
- Open the directory in a command line: `cmake .\xfemm\cfemm\ -G "Visual Studio 17 2022" -A x64`
- Take sample_config.json and put it into xfemm\coilgunsim directory (working dir is screwed up and I didn't have time to fix it) and rename the JSON file to config.json

# Config description:
Every value uses milimeters as the unit.
- NumThreads - How many threads to use (use number of logical CPU cores - 1)
- CoilLengthStep - step size for coil length (how much to increase the length by each iteration)
- CoilLengthRange - range of coil lengths to simulate
- CoilTurnStep - step size for coil turns
- CoilTurnRange - range of coil turns to simulate
- CoilWireSizes - list of wire sizes to simulate
- ProjectileLengthStep - step size for projectile length
- ProjectileLengthRange - range of projectile lengths to simulate
- ProjectileDiameters - list of projectile diameters to simulate
- BoundaryLayers - number of boundary layers to use for simulation (higher is better, less noise, but slower)
- BoundaryHeight - height of boundary layer (this should be at least 150, should be as small as possible, but FEMM might not be able to simulate the projectile if it is too long and this value too small)
- BoreWallThickness - thickness of the bore's wall
- WireCompactFactor - compact factor for wire (1.1 is pretty ok)

# License
MIT

# Original README:
# Welcome to xfemm

Welcome to the xfemm project. xfemm is a software project intended to
create a direct interface to a high quality magnetics finite element code
based on FEMM. The objective of xfemm is to create a cross-platform
command line magnetics finite element solver written in standard C++, a
set of magnetics problem definition and post-processing functions in
native Matlab/Octave code, and a mex interface to the solvers

## NOTE TO USERS

If you use xfemm, particularly for industrial work, but also academic, 
it will be greatly appreciated if you could write an email stating this 
and how it has supported your work. This is a low-cost way to ensure 
further development and maintenance will continue! Contact the authors 
on the discussion forum, or you will find an email address in the source 
files.

If you wish to cite xfemm in your work, please use the following:

Crozier, R, Mueller, M., "A New MATLAB and Octave Interface to a 
Popular Magnetics Finite Element Code", Proceedings of the 22nd 
International Conference on Electric Machines (ICEM 2016), September 
2016.

We would also suggest you cite the original FEMM program.

## Installation and Setup

There are two ways to make use of the xfemm project. One is to use it as
a collections of standalone programs run from the command line. The other
is to use it through the Matlab/Octave programming language.

The standalone programs are fmesher and fsolver. There is also a library
of post-processing functions called fpproc, but no standalone program
interface is provided to this at this time, you will have to create your
own (note that the Matlab/Octave interface does, however, provide full
access to fpproc). 

## Compiling Standalone Binary Programs

Released versions of xfemm come with pre-built binaries. But if you want
to compile xfemm on your platform, you can do so quite easily with cmake
and your compiler of choice. Run cmake on the CMakeLists.txt in the cfemm
directory to create the build system, and then build the project. On Linux
this would be done as

    cd <install dir>/xfemm/cfemm
    cmake .
    make

the binary files are found in the xfemm/cfemm/bin directory


### Side-note: Compiling with an external triangle

Xfemm ships with triangle 1.6 by Jonathan Shewchuk of the Carnegie Mellon
University, which was released in 2005.  There is a newer (unofficial) version
available, that also incorporates the aCute mesher developed at the University
of Florida, Gainesville.  This version has the main goal "to turn Triangle into
a re-usable library and the introduction of a simplified C API."

You can get the new triangle code here:
https://github.com/wo80/Triangle

If you install the external triangle to a standard location (e.g. /usr/local),
xfemm will automatically pick it up. You can notice this by the line "Found
triangle <version>" when you run cmake.  If you install the external triangle
in a different location, you'll need to tell cmake where to find it:

    cd <install dir>/xfemm/cfemm
    cmake . -DCMAKE_PREFIX_PATH=<triangle install dir>
    make

## Compiling Matlab Interface

Detailed instructions for compiling the Matlab inteface can be found in
the README file provided in the mfemm directory. The process is fairly well 
automated.
