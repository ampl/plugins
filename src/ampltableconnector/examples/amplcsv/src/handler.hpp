#pragma once

#include <ctime>

#include "ampltableconnector.hpp"

using namespace amplt;

static std::string name = "amplcsv";
static std::string version = "beta 0.0.3";

class Handler:
public Connector{

	public:

	Handler(AmplExports *ae, TableInfo *TI) : Connector(ae, TI){

	handler_version = name + " - " + version;

	sep = ",";
	quotechar = "";
	quotestrings = false;
	has_header = true;
	use_header = true;
};

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
	void register_handler_args();
	void register_handler_kargs();
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
	void write_data_ampl(FileHandler & f);
	void write_data_perm(FileHandler & f, std::vector<int>& perm);
	void write_header(FileHandler & f, std::vector<std::string>& header);
	std::map<std::vector<std::string>, int> get_used_keys_map(std::vector<int>& perm);
	void write_remaining_rows(
		std::ifstream & infile,
		std::map<std::vector<std::string>, int> & used_keys_map,
		std::vector<int> & perm,
		FileHandler & f,
		int init_nfields
	);
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



std::string doc = name + "\n" + name + "-" + version + "\n"
"\n"
"A table handler for comma-separated values (.csv) files.\n"
"General information on table handlers and data correspondence between AMPL and an external table is \n"
"available at:\n"
"\n"
"    https://ampl.com/BOOK/CHAPTERS/13-tables.pdf\n"
"\n"
"The available options for amplcsv are:\n"
"\n"
"external-table-spec:\n"
"    specifies the path to the .csv file to be read or written with the read table and write table \n"
"    commands. If no file is specified amplcsv will search for a file with the table name and the \n"
"    .csv file extension in the current directory. If the table is to be written and the file does \n"
"    not exist it will be created. \n"
"\n"
"    Example:\n"
"        table foo OUT \"amplcsv\" \"bar.csv\": [keycol], valcol;\n"
"\n"
"verbose:\n"
"    display warnings during the execution of the read table and write table commands.\n"
"\n"
"    Example:\n"
"        table foo OUT \"amplcsv\" \"verbose\": [keycol], valcol;\n"
"\n"
"verbose=option:\n"
"    display information acording to the speficied option. Available options: \n"
"        0 (default) - display information only on error, \n"
"        1 - display warnings, \n"
"        2 - display general information\n"
"        3 - display debug information.\n"
"\n"
"    Example:\n"
"        table foo OUT \"amplcsv\" \"verbose=2\": [keycol], valcol;\n"
"\n"
"sep=option:\n"
"    specifies the separator character in the .csv file. Available options: \n"
"        comma (default) - \",\" character separator, \n"
"        semicolon - \";\" character separator, \n"
"        colon - \":\" character separator, \n"
"        tab - tab character separator,\n"
"        space - single space separator.\n"
"\n"
"    Example:\n"
"        table foo IN \"amplcsv\" \"sep=tab\": keycol <- [keycol], valcol;\n"
"\n"
"quote=option:\n"
"    weather or not to quote strings when writting data to a file or unquote strings when reading \n"
"    data from a file. Available options: \"none\" (default), \"single\" for single quotes and \n"
"    \"double\" for double quotes.\n"
"\n"
"    Example:\n"
"        table foo IN \"amplcsv\" \"quote=double\": keycol <- [keycol], valcol;\n"
"\n"
"header=option:\n"
"    Weather or not to read/write the header from the .csv file (defaults to true). If the option \n"
"    \"header=false\"  is specified, amplcsv will assume that the external table does not have a \n"
"    header and that the order of the columns in AMPL and in the .csv file is the same.\n"
"\n"
"    Example:\n"
"        table foo OUT \"amplcsv\" \"header=false\": [keycol], valcol;\n"
"\n"
"overwrite:\n"
"    (OUT only) If the option is used the external table will be completely overwriten. Note that, by\n"
"     default, in OUT mode amplcsv will maintain the initial header.\n"
"\n"
"    Example:\n"
"        table foo OUT \"amplcsv\" \"overwrite\": [keycol], valcol;\n"
"\n"
"alias:\n"
"    instead of writting the data to a specific .csv file it is possible to define an alias. In \n"
"    the following example the table handler will search for the file bar.csv to write the data. If the \n"
"    file does not exist it will be created.\n"
"\n"
"    Example:\n"
"        table foo OUT \"amplcsv\" \"bar\": [A], B;\n"
"\n"
;







