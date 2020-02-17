#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <limits>
#include <iomanip>
#include <sstream>
#include <cstring>


template <class T>
std::string my_to_string(T num){
	std::stringstream strs;
	strs << std::scientific << std::setprecision(std::numeric_limits<T>::digits10+1) << num;
	return strs.str();
};

/* convert numeric to string with ndecdig decimal digits*/
template <class T>
std::string my_to_string2(T num, int ndecdig){
	std::stringstream strs;
	strs << std::fixed << std::setprecision(ndecdig) << num;
	return strs.str();
};

