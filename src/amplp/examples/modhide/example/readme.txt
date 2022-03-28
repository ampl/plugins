Adapted from the code available at
    http://www.ampl.com/NEW/modhide.zip

This plugin will generate an executable
    trivial_encoder
and a shared library
    trivial_decoder.dll

Place trivial_encoder in a place of your choice, trivial_decoder.dll
should be placed in the folder where you have the ampl executable.

Start with regular model and data files, say diet.mod and diet.dat.

Choose what we are going to encode:
- visible.mod will have the visible part of the model
- toencode.mod will have that part of the model that we will encode

Create a file encoded.mod with the following content

load trivial_decoder.dll;
function trivial_decoder;
call trivial_decoder(0);

Add the enconded content to the previous file with the command

./trivial_encoder toencode.mod >> encoded.mod

The script encoded.run has the instructions to load the models and data,
solve the problem and display the visible data.
