import subprocess
import os
import shutil


dll = "basichandler.dll"

# compile
params = [
	"g++",
	"-std=c++11",
	"-I../../../include/",
	"-O2",
	# ~ "-Wall",
	# ~ "-g",
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
