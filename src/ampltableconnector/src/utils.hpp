#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <limits>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <math.h>
//#include <fstream>
#include <cstdio>


// Simple replacement for the to_string function since we are building for -std=c++03 
template <class T>
std::string numeric_to_string(T num){
	std::stringstream strs;
	strs << num;
	return strs.str();
};


// Convert numeric to string with ndecdig decimal digits
// will be used mostly to print cpu times and such
template <class T>
std::string numeric_to_fixed(T num, int ndecdig){
	std::stringstream strs;
	strs << std::fixed << std::setprecision(ndecdig) << num;
	return strs.str();
};


// Gets the substring after the last dot in the given string
std::string
get_file_extension(const std::string & filepath);


// Check if a file with a given name already exists.
bool
check_file_exists(const std::string & filename);


// Verifies if the lower case representation of 2 strings is identical
bool
compare_strings_lower (const std::string & str1, const std::string & str2);


// vector printing, might be helpfull
template <class T>
void print_vector(std::vector<T> v){

	std::cout << "[";
	for (int i = 0; i < v.size(); i++){
		std::cout << v[i];
		if (i < v.size() - 1){
			std::cout << ", ";
		}
	}
	std::cout << "]";
	std::cout << std::endl;
};




