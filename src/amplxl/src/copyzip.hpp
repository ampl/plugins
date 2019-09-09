#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <cstring>


#include "zip.h"
#include "myzip.hpp"
#include "unzip.h"
#include "crypt.h"

#ifndef USEWIN32IOAPI
const int MAX_PATH = 256;
#endif

int
copy_uchanged_files(
	std::string& zip_string,
	std::string& zip_copy_string,
	std::vector<std::string>& removed_files
);
