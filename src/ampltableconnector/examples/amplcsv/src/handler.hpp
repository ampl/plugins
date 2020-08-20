#pragma once

#include "ampltableconnector.hpp"


class Handler:
public Connector{

	public:

	// map string to variant map<string, someclass>
	//get double
	//get string
	//get bool
	//get int

	Handler();

	// separator between elements in csv
	char* sep; 

	// character used to quote strings
	char* quotechar;

	// weather or not to read/write the header
	bool has_header;

	// Weather or not to quote/unquote strings when writting/reading, defaults to false
	bool quotestrings;

	// override functions
	void read_in();
	void write_out();
	void write_inout();
	void register_handler_names();
	void register_handler_extensions();
	void generate_table();

	// Splits the string str into elements of vector row according to the defined separator.
	// If the first character of a string is a quote all elements are included until an individual
	// quote is found. This allows the inclusion of separators inside strings. We do not enforce
	// that a string starting with quote must end with a quote so something like
	//     " a strin,g""inside quote"" more text"should not be here
	// between separators will be parsed as a single string.
	void parse_row(std::string & str, std::vector<std::string> & row);

	// Validates if all column names in AMPLs table are found in the header and assigns the position
	// of each column in perm, so perm[i] gives as the position of column i in the external table on
	// AMPLs table.
	void validate_header(const std::vector<std::string> & header, std::vector<int> & perm);

	void get_keys(
		std::vector<std::string> & row,
		std::vector<int> & perm,
		std::vector<std::string> & keyvec
	);

	void send_val_to_ampl(std::string & val, int col);
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
