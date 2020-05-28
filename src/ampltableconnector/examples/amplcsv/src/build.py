import subprocess
import os
import shutil


# compile
params = [
	"g++",
	"-I/home/nsantos/Documents/ampl/mylibs/solvers2/sys.x86_64.Linux/",
	"-std=c++03",
	"-O2",
	# ~ "-Wall",
	# ~ "-g",
	# ~ "-Wshadow",
	"-c",
	"-fPIC",
	"ampltableconnector.cpp",
	"logger.cpp",
	"utils.cpp"
]

subprocess.call(params)

# link
params = [
	"g++",
	"-shared",
	"-o",
	"amplcsv.dll",
	"ampltableconnector.o",
	"logger.o",
	"utils.o"
]

subprocess.call(params)

# clean
for f in os.listdir("."):
	if f.endswith(".o"):
		os.remove(f)

# move dll to ampl folder
src = "./amplcsv.dll"
dst = "/home/nsantos/Documents/ampl/ampl_linux-intel64/amplcsv.dll"
shutil.copyfile(src, dst)
