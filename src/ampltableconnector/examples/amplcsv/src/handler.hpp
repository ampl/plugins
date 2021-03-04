#pragma once

#include <ctime>

#include "ampltableconnector.hpp"

using namespace amplt;

class Handler:
public Connector{

	public:

	Handler();

	// separator between elements in csv
	std::string sep; 

	// character used to quote strings
	std::string quotechar;

	// weather or not to read/write the header
	bool has_header;

	// Weather or not to quote/unquote strings when writting/reading, defaults to false
	bool quotestrings;

	// Weather or not to use the existing csv header (OUT only)
	bool use_header;

	// override functions
	void read_in();
	void write_out();
	void write_inout();
	void register_handler_names();
	void register_handler_extensions();
	void generate_table();
	void validate_arguments();

	// Splits the string str into elements of vector row according to the defined separator.
	// If the first character of a string is a quote all elements are included until an individual
	// quote is found. This allows the inclusion of separators inside strings. We do not enforce
	// that a string starting with quote must end with a quote so something like
	//     " a strin,g""inside quote"" more text"should not be here
	// between separators will be parsed as a single string.
	std::vector<std::string> parse_row(const std::string & str, int row_size = 0);
	//~ void parse_row(const std::string & str, std::vector<std::string> & row);

	// Validates if all column names in AMPLs table are found in the header and assigns the position
	// of each column in perm, so perm[i] gives as the position of column i in the external table on
	// AMPLs table.
	std::vector<int> validate_header(std::vector<std::string> & header);

	void get_keys(
		std::vector<std::string> & row,
		std::vector<int> & perm,
		std::vector<std::string> & keyvec
	);

	void send_val_to_ampl(std::string val, int col);

	std::vector<std::string> get_header_ampl();
	std::vector<std::string> get_header_csv();
	std::vector<int> parse_header();
	void write_data_ampl(FILE *f);
	void write_data_perm(FILE *f, std::vector<int>& perm);
	void write_header(FILE *f, std::vector<std::string>& header);
};


// Remove first andl last elements of str if they are the quotechar
std::string try_unquote_string(std::string str, const std::string & quotechar);


// std::getline only checks for the endline for your particular platform (???)
// code adapted from:
// https://stackoverflow.com/questions/6089231/getting-std-ifstream-to-handle-lf-cr-and-crlf
// Changed case to if else, since with -std=c++03 gcc complains about
//
// utils.cpp:70:37: error: ‘std::char_traits<char>::eof()’ cannot appear in a constant-expression
//   case std::streambuf::traits_type::eof():
std::istream& safeGetline(std::istream& is, std::string& t);

std::string get_csv_row(std::istream& is);
