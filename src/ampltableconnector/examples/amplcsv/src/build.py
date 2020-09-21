import subprocess
import os
import shutil


dll = "amplcsv.dll"

# compile
params = [
	"g++",
	# ~ "clang",
	# ~ "-std=c++03",
	# ~ "-std=c++0x",
	"-std=c++11",
	# ~ "-std=c++14",
	"-I../../../include/",
	# ~ "-O2",
	"-Wall",
	"-Wextra",
	"-g",
	"-c",
	"-fPIC",
	"handler.cpp",
]

subprocess.call(params)

# link
params = [
	"g++",
	"-shared",
	"-o",
	dll,
	"handler.o",
]

subprocess.call(params)

# clean
for f in os.listdir("."):
	if f.endswith(".o"):
		os.remove(f)

# move dll to ampl folder
src = "./" + dll
dst = "/home/nsantos/Documents/ampl/ampl_linux-intel64/" + dll
shutil.copyfile(src, dst)
