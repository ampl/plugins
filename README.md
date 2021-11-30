# Table handlers for AMPL

## Build instructions

Either clone this repository recursively:
```
git clone --recursive https://github.com/ampl/plugins.git
``` 
or, after cloning, check out the submodules:
```
git submodule update --init
```

To generate build files using cmake, for Linux/Unix/OSX systems (change 64 to 32 if a 32 bits build is needed):

```
mkdir build
cd build
cmake .. -DARCH=64
```

On Windows, with Visual Studio 2017+:
```
mkdir build
cd build
cmake .. 
```

add `-A Win32` to the cmake statement to generate 32 bits libraries.
