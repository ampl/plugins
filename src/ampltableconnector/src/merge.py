import os
import shutil

"""Merge multiple files into single header library ampltableconnector.hpp 
"""

conn = "ampltableconnector.hpp"
merge_path = "./" + conn

hpp = [
	"includes.hpp",
	"ampl.hpp",
	"utils.hpp",
	"logger.hpp",
	"connector.hpp"
]

cpp = [
	"utils.cpp",
	"logger.cpp",
	"connector.cpp"
]

with open(merge_path, "w") as f:

	f.write("#pragma once\n")

	for h in hpp:

		with open(h, "r") as s:

			for line in s.readlines():

				if not line.startswith("#pragma"):
					f.write(line)

	for c in cpp:

		with open(c, "r") as s:

			for line in s.readlines():

				if not line.startswith("#include"):
					f.write(line)


# copy new version of ampltableconnector to relevant folders
src = "./" + conn

dests = [
	"../examples/dummy/src/",
	"../examples/template/src/",
	"../examples/amplcsv/src/"
]

for d in dests:
	dst = d + conn
	shutil.copyfile(src, dst)
