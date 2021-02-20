#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <limits>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <math.h>


// simple replacement for the to_string function since we are building for -std=c++03 
template <class T>
std::string numeric_to_string(T num){
	std::stringstream strs;
	strs << std::setprecision(std::numeric_limits<T>::digits10) << num;
	return strs.str();
};


// convert numeric to string with ndecdig decimal digits
// will be used mostly to print cpu times and such
template <class T>
std::string numeric_to_fixed(T num, int ndecdig){
	std::stringstream strs;
	strs << std::fixed << std::setprecision(ndecdig) << num;
	return strs.str();
};


// code from
// https://www.cl.cam.ac.uk/~mgk25/ucs/utf8_check.c
/*
 * The utf8_check() function scans the '\0'-terminated string starting
 * at s. It returns a pointer to the first byte of the first malformed
 * or overlong UTF-8 sequence found, or NULL if the string contains
 * only correct UTF-8. It also spots UTF-8 sequences that could cause
 * trouble if converted to UTF-16, namely surrogate characters
 * (U+D800..U+DFFF) and non-Unicode positions (U+FFFE..U+FFFF). This
 * routine is very likely to find a malformed sequence if the input
 * uses any other encoding than UTF-8. It therefore can be used as a
 * very effective heuristic for distinguishing between UTF-8 and other
 * encodings.
 *
 * I wrote this code mainly as a specification of functionality; there
 * are no doubt performance optimizations possible for certain CPUs.
 *
 * Markus Kuhn <http://www.cl.cam.ac.uk/~mgk25/> -- 2005-03-30
 * License: http://www.cl.cam.ac.uk/~mgk25/short-license.html
 */

//#include <stdlib.h>

unsigned char *utf8_check(unsigned char *s);
