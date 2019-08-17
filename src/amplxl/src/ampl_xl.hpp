#pragma once

#include <stdlib.h>
#include <string.h>

#include <iostream>

#include <vector>
#include <string>
#include <cstring>
#include <map>
#include <cstdlib>
#include <sstream>

// headers to manipulate folders in windows
#ifdef _WIN32
#include <windows.h>
#include <tchar.h>
#include <time.h>

void mymkstemp(std::string& tmpl, int pos);

#endif



// external libraries headers
#include "pugixml.hpp"
#include "myunz.hpp"
#include "myzip.hpp"

#include "funcadd.h"
#include "arith.h"	/* for Arith_Kind_ASL and Long */

#ifdef strtod
#undef strtod
#endif

#ifdef sprintf
#undef sprintf
#endif


// auxiliary macro to allocate memory in AMPLs internal structures
#define TM(len) (*ae->Tempmem)(TI->TMI,len)

const int EXCEL_MAX_ROWS = 1048576;
const std::string EXCEL_MAX_COLS = "ZZZZZ";



/* Usually you use c_str() to convert an std::string to a const char*
 *   const char* somechar = somestring.c_str();
 * however some functions explicitly require char* type as input so we use
 *   char* somechar = &somestring[0u];
 * to make access the information.
 * */



static int Read_ampl_xl(AmplExports *ae, TableInfo *TI);
static int Write_ampl_xl(AmplExports *ae, TableInfo *TI);



class ExcelColumnManager
{
	std::string upcase_chars;
	int nchars;
	std::map<char, int> char_pos;

	public:

	ExcelColumnManager();
	void next(std::string &astring);
};





class ExcelManager
{

	public:

	// links to ampl
	TableInfo *TI;
	AmplExports *ae;

	// strings for file manipulation
	std::string excel_path;
	std::string temp_folder;
	std::string table_name;

	std::string excel_iner_file; //the complete path to a file inside of excel
	std::string excel_file; //excel single file
	std::string final_path; //excel single file in temp folder

	// excel workbook info
	bool has_range;
	std::string excel_range;
	std::map<std::string, std::string> sheet_rel_map;

	// excel range info
	std::vector<char*> split;
	std::string range_sheet;
	std::string range_first_col;
	std::string range_last_col;
	int range_first_row;
	int range_last_row;

	// excel relations info
	std::string sheet_rel;
	std::string data_sheet;

	// shared strings info
	std::vector<std::string> shared_strings;

	// map of ampl column index to excel
	std::vector<std::string> ampl_to_excel_cols;

	// to generate the sucesive columns in an excel sheet
	ExcelColumnManager ecm;

	// weather to break on first blank line or not
	bool break_mode;

	// top level methods
	int add_info(AmplExports *ae, TableInfo *TI);
	int manage_workbook();
	int manage_relations();
	int manage_shared_strings();

	// tries to find the defined range and loads the relations of sheets
	int parse_workbook();

	// parses the excel range into five elements: sheet, first colum, first row, last column and last row
	int parse_excel_range();

	int parse_data(
		const pugi::xml_node &node,
		const int first_row,
		const int last_row,
		const std::string first_col,
		const std::string last_col
	);

	// check if all columns of ampl representation are present in table and stores the corresponding column
	int check_columns(
		const pugi::xml_node &node,
		const int first_row,
		const std::string &first_col,
		const std::string &last_col
	);

	// gets the name of the excel sheet with the information to extract
	int get_excel_sheet(std::string &path);

	// extract excel shared strings to shared_strings vector
	int get_shared_strings();

	int
	create_temp_folder();

	int
	clean_temp_folder();

	ExcelManager();
	int prepare();

	//errors
	void cannot_find_file();
	void cannot_create_temp();
	void cannot_extract_workbook();
	void cannot_open_workbook();
	void cannot_parse_range();
	void cannot_find_table();
	void cannot_extract_ss();
	void cannot_open_ss();
	void cannot_extract_sheet();
	void cannot_open_sheet();
	void cannot_find_column(int col);
	void cannot_find_keys();
	void cannot_update_ss();
	void cannot_update_sheet();
	void unsuported_flag();

};




std::string
get_excel_path(TableInfo *TI);

std::string
get_file_extension(const std::string& filepath);

void
join_path(
	std::string &temp_folder,
	std::string &excel_file,
	std::string &path
);

void inspect_ti(TableInfo *TI);
void inspect_values(TableInfo *TI);

class ExcelReadManager:
public ExcelManager{

	public:
	int run();
	int manage_data();


};




class ExcelWriteManager:
public ExcelManager{

	public:

	std::map<std::string, int> sstrings_map;
	std::vector<std::string> excel_keys;
	std::vector<std::string> ampl_keys;


	int run();
	int manage_data();
	void get_sstrings_map();

	int
	write_data_out(
		pugi::xml_node node,
		int first_row,
		int last_row,
		std::string &first_col,
		std::string &last_col
	);

	int
	write_data_inout(
		pugi::xml_node node,
		int first_row,
		int last_row,
		std::string &first_col,
		std::string &last_col
	);

	void
	set_cell_value(

		DbCol *db,
		pugi::xml_node excel_row,
		pugi::xml_node excel_cell,
		std::string &scol,
		int i,
		int trow
	);



	int check_shared_strings(std::string s);
	int update_shared_strings(int init_size);

	int get_excel_keys(pugi::xml_node excel_row, int row);
	int get_ampl_keys(int row);
	int get_ampl_row();
	int copy_info(pugi::xml_node excel_row, int row, int ampl_row);

};

int check_rows(pugi::xml_node node, int first_row, int last_row);

pugi::xml_node
get_excel_row(pugi::xml_node parent, int row);

pugi::xml_node
get_excel_cell(pugi::xml_node parent, int row, std::string &col);

void unquote_string(std::string &str);

// links to ooxml document information
//https://docs.microsoft.com/en-us/dotnet/api/documentformat.openxml.spreadsheet.cell?view=openxml-2.8.1
//https://docs.microsoft.com/en-us/dotnet/api/documentformat.openxml.spreadsheet.row?view=openxml-2.8.1
//https://docs.microsoft.com/en-us/dotnet/api/documentformat.openxml.spreadsheet.cell?view=openxml-2.8.1


//g++ -I../../mylibs/solvers/sys.x86_64.Linux -I../../mylibs/zlib-1.2.11 -std=c++03 -g -c -fPIC ampl_xl.cpp pugixml.cpp myunz.cpp unzip.c ioapi.c myzip.cpp zip.c
//g++ -shared -o ampl_xl.dll ampl_xl.o pugixml.o myunz.o unzip.o ioapi.o myzip.o zip.o -Wl,--whole-archive ../../mylibs/zlib-1.2.11/libz.a -Wl,--no-whole-archive





