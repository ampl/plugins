import subprocess
import os

basepath = os.getcwd()

# zlib
newpath = basepath + "/zlib-1.2.11"

os.chdir(newpath)

subprocess.call("./configure")

CFLAGS ="-O3 -D_LARGEFILE64_SOURCE=1 -DHAVE_HIDDEN"
SFLAGS ="-O3 -fPIC -D_LARGEFILE64_SOURCE=1 -DHAVE_HIDDEN"

mkpath = newpath + "/Makefile"
with open(mkpath, "r") as f:
	text = f.read()

text = text.replace(CFLAGS, SFLAGS)

with open(mkpath, "w") as f:
	f.write(text)

subprocess.call("make")

# solvers
newpath = basepath + "/solvers2"
os.chdir(newpath)

subprocess.call("./configure")
subprocess.call("make")


# compile and link
newpath = basepath + "/src"
os.chdir(newpath)

params = [
	"g++",
	"-I../solvers2/sys.x86_64.Linux",
	"-I../zlib-1.2.11",
	"-I../zlib-1.2.11/contrib/minizip",
	"-std=c++03",
	"-O2",
	"-c",
	"-fPIC",
	"ampl_xl.cpp",
	"pugixml.cpp",
	"myunz.cpp",
	"unzip.c",
	"ioapi.c",
	"myzip.cpp",
	"zip.c"
]

subprocess.call(params)

params = [
	"g++",
	"-shared",
	"-o",
	"ampl_xl.dll",
	"ampl_xl.o",
	"pugixml.o",
	"myunz.o",
	"unzip.o",
	"ioapi.o",
	"myzip.o",
	"zip.o",
	"-Wl,--whole-archive",
	"../zlib-1.2.11/libz.a",
	"-Wl,--no-whole-archive"
]

subprocess.call(params)
