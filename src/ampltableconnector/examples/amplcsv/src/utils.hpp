#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <limits>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <math.h>
#include <fstream>
//#include <istream>
//#include <streambuf>


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


// Remove first andl last elements of str if they are the quotechar
void try_unquote_string(std::string & str, char* quotechar);


// std::getline only checks for the endline for your particular platform (???)
// code adapted from:
// https://stackoverflow.com/questions/6089231/getting-std-ifstream-to-handle-lf-cr-and-crlf
// Changed case to if else, since with -std=c++03 gcc complains about
//
// utils.cpp:70:37: error: ‘std::char_traits<char>::eof()’ cannot appear in a constant-expression
//   case std::streambuf::traits_type::eof():
std::istream& safeGetline(std::istream& is, std::string& t);


// copies the file in source_path to dest_path
void copy_file(const std::string & source_path, const std::string & dest_path);
