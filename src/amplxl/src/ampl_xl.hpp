


/* amplxl is a table handler
**
** It uses following libraries to achieve its purpose:
** - zlib : to extract and compress data
**   https://zlib.net/
**
** - pugixml : library to manipulate XML
**   https://pugixml.org/
**   


// links to ooxml document information
//https://docs.microsoft.com/en-us/dotnet/api/documentformat.openxml.spreadsheet.cell?view=openxml-2.8.1
//https://docs.microsoft.com/en-us/dotnet/api/documentformat.openxml.spreadsheet.row?view=openxml-2.8.1
//https://docs.microsoft.com/en-us/dotnet/api/documentformat.openxml.spreadsheet.cell?view=openxml-2.8.1

*/

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
#include <ctime>
#include <limits>
#include <iomanip>

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
#include "copyzip.hpp"
#include "oxmlutils.hpp"
#include "logger.hpp"
#include "utils.hpp"


// headers from AMPL
#include "funcadd.h"
#include "arith.h"	/* for Arith_Kind_ASL and Long */





// auxiliary macro to allocate memory in AMPLs internal structures
#define TM(len) (*ae->Tempmem)(TI->TMI,len)


// some constants
const int EXCEL_MAX_ROWS = 1048576;
const std::string EXCEL_MAX_COLS = "XFD";
const std::string version = "0.1.5";


const char* row_attr = "r";
const int CPUTIMES_NDIGITS = 3; // number of decimal digits to present in printed cpu times


/* Usually you use c_str() to convert an std::string to a const char*
 *   const char* somechar = somestring.c_str();
 * however some functions explicitly require char* type as input so we use
 *   char* somechar = &somestring[0u];
 * to make access the information.
 * */


// Base functions to read and write data to AMPL
static int Read_ampl_xl(AmplExports *ae, TableInfo *TI);
static int Write_ampl_xl(AmplExports *ae, TableInfo *TI);


/*
** Auxiliary class to iterate columns of a spreadsheet.
*/
class ExcelColumnManager
{
	std::string upcase_chars; // alphabet major strings
	int nchars; // the number of strings in alphabet
	std::map<char, int> char_pos; // map of string position in alphabet for faster access

	public:

	ExcelColumnManager();

	/*
	** main method, gets as input a string definig a spreadsheet column
	** that is modefied in place to the next column, e.g. AA is turned into AB.
	*/
	void next(std::string &astring);
};


/*
** Enum to classify how the table is defined in the spreadsheet
** TABLE_RANGE - table is defined by a named range
** TABLE_HEADER - only the  header of the table is defined by a named range
** TABLE_SHEET - table is defined by a sheet name
*/
enum TABLE_TYPE{
	TABLE_RANGE = 0,
	TABLE_HEADER = 1,
	TABLE_SHEET = 2
};


/*
** Base class, provides the methods and structures to read information from a OOXML file.
*/
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
	bool has_range; // if the table is defined by a named range or not
	std::string excel_range; // the named range (in case it is defined)
	std::map<std::string, std::string> sheet_rel_map; // map of the sheets to relations

	// excel range info
	std::vector<char*> split; // auxiliary vector to split a named range
	std::string range_sheet; // the sheet were the named range is
	std::string range_first_col;
	std::string range_last_col;
	int range_first_row;
	int range_last_row;

	// excel relations info
	std::string sheet_rel; // the relations of the sheet
	std::string data_sheet; // the final name of the sheet

	// structure to hold the shared strings found in the spreadsheet
	std::vector<std::string> shared_strings; 

	// map of ampl column index to excel
	std::vector<std::string> ampl_to_excel_cols;

	// to generate the sucesive columns in an excel sheet
	ExcelColumnManager ecm;

	// for errors and message printing to users 
	Logger logger;

	// weather to break on first blank line or not
	bool break_mode;

	// display information during execution
	int verbose;

	// weather to delete or drop a table
	std::string write;

	// weather to backup the initial data or not
	bool backup;

	// inout keyword of the table, can be IN, OUT or INOUT
	std::string inout;

	// if we are dealing with a 2 dimensional table
	bool is2D;

	// check if we are only reading a table
	bool isReader;

	// how the table is represented in the spreadsheet, see enum TABLE_TYPE
	int tableType;

	// map of a shared string to its position in the shared_strings array
	std::map<std::string, int> sstrings_map;

	// if the named range that defines the table needs to be updated
	bool updateRange;


	// methods


	/*
	** Adds Ampl Exports and Table Info as attributes of te class
	*/
	int add_info(AmplExports *ae, TableInfo *TI);


	/*
	** Method that extracts the spreadsheet workbook relevant information.
	** The process is done in the following steps:
	** - extract the workbook file from the spreadsheet to the temp folder;
	** - open the resulting xml file;
	** - parse the workbook data;
	** - check if the table was defined as named range or sheet name;
	** - get the relation of the sheet with the data.
	*/
	int manage_workbook();


	/*
	** Method that extracts the spreadsheet relations file.
	** The process is done in the following steps:
	** - extract the relations file from the spreadsheet to the temp folder
	** - open the resulting xml file
	** - parse the relations to obtain the name of the data sheet
	*/
	int manage_relations();


	/*
	** Method that extracts the shared strings from the corresponding file in the spreadsheet.
	** The process is done in the following steps:
	** - extract the shared strings file from the spreadsheet to the temp folder;
	** - open the resulting xml file;
	** - extract the shared strings to the shared strings vector: shared_strings;
	*/
	int manage_shared_strings();


	/*
	** Tries to find a defined range and loads the relations of sheets.
	*/
	int parse_workbook();


	/*
	** Parses the excel range into five elements: sheet, first colum, first row, last column and last row.
	*/
	int parse_excel_range();


	/*
	** Parses the spreadsheet data. Data is loaded into AMPL line by line.
	** Parameters:
	**     node : an xml node that holds the spreadsheet rows as children
	**     remaining parameters define the bounds of the data we want to read
	** Returns:
	*/
	int parse_data(
		const pugi::xml_node &node,
		const int first_row,
		const int last_row,
		const std::string first_col,
		const std::string last_col
	);

	// check if all columns of the ampl table are present in the spreadsheet table
	// the order of the columns in the spreadsheet does not need to be equal to the order in ampl 
	// a map of the index is created in the attribute ampl_to_excel_cols
	// the position of column i in ampl is ampl_to_excel_cols[i] in the spreadsheet
	// node is an xml node that holds the spreadsheet rows as children
	// remaining parameters define the bounds of the header in the table
	// returns -1 if all columns are found or the index of the missing column
	int check_columns(
		const pugi::xml_node &node,
		const int first_row,
		const std::string &first_col,
		std::string &last_col
	);

	// gets the name of the excel sheet with the information to extract
	int get_excel_sheet(std::string &path);

	// extract excel shared strings to shared_strings vector
	int get_shared_strings();

	int parse_data2D(pugi::xml_node node, int first_row, int last_row, std::string & first_col, std::string & last_col);

	void get_cell_val(pugi::xml_node node, std::string & val);
	void set_dbcol_val(std::string & val, DbCol * db, int is_string);

	int get_table_top_left_coords(pugi::xml_node node, int & first_row, std::string & first_col);

	int
	create_temp_folder();

	int
	clean_temp_folder();

	int get_last_column_in_table(
		const pugi::xml_node node,
		const int first_row,
		const std::string & first_col,
		std::string & last_col
	);

	int get_last_row_in_table(
		const pugi::xml_node node,
		const int first_row,
		const std::string & first_col
	);


	void
	parse_header(
		const std::string & first_col,
		std::string & last_col,
		int first_row,
		pugi::xml_node node,
		std::map<std::string, std::string> & xl_col_map
	);


	int
	parse_header_2D_reader(
		const std::string & first_col,
		const std::string & last_col,
		int first_row,
		pugi::xml_node node,
		std::map<std::string, std::string> & xl_col_map,
		std::vector<std::string> & header,
		std::vector<int> & is_header_string
	);




	void
	set_default_2D_col_map(
		const std::string & first_col,
		const std::string & last_col,
		std::map<std::string, std::string> & col_map
	);

	void
	set_default_col_map(
		const std::string & first_col,
		const std::string & last_col,
		std::map<std::string, std::string> & col_map
	);

	void set_logger(Logger & logger);

	ExcelManager();

	// parses the arguments given in AMPLs TI (table info) structure
	int prepare();

	int
	add_missing_column(
		pugi::xml_node node,
		const std::string & col_name,
		int row,
		std::string & col
	);

	void
	set_cell_string_value(
		pugi::xml_node cell,
		const std::string wstr,
		int row,
		const std::string col

	);


	/*
	** Checks if a given string already exists in the shared strings map. If the string does not
	*  exist, it is added to the map sstrings_map and to the shared_strings vector.  
	** Parameters:
	** - s the string we want to check;
	** Returns:
	** - the index of string s in the shared_strings vector;
	*/
	int check_shared_strings(const std::string & s);


	/*
	** Maps shared strings to their index in the shared strings vector.
	*/
	void get_sstrings_map();



	std::string numeric_to_scientific(real num);


	// common log messages
	void log_table_coords(
		const std::string & first_col,
		const std::string & last_col,
		int first_row,
		int last_row
	);

	void log_missing_column(int col);

	void
	log_last_row_change(int initial_row, int updated_row);

	int
	get_sheet_from_zip();

};


/*
** Class to read a table from an OOXML file
*/
class ExcelReadManager:
public ExcelManager{

	public:

	/*
	** Main method of the class.
	** Invokes the sucessive steps needed to read the defined table.
	*/
	int run();


	/*
	** Method that defines how the data is read from the corresponding sheet.
	** The process is done in the following steps:
	** - extract the data sheet from the spreadsheet file to a file in the temp folder;
	** - open the resulting xml file;
	** - get the node that has the spreadsheet rows as children;
	** - derive the bounds of the spreadsheet table;
	** - read the data.
	*/
	int manage_data();

	int manage_data2D();
};


/*
** Class to write a table provided by AMPL into a OOXML file.
*/
class ExcelWriteManager:
public ExcelManager{

	public:

	// vectors to store the arity strings (in case INOUT)
	std::vector<std::string> excel_keys;
	std::vector<std::string> ampl_keys;

	ExcelWriteManager();

	void check_range_update(int last_row);
	int update_workbook(std::string & xl_copy_path);
	int get_new_range(std::string & new_range);

	/*
	** Main method of the class.
	** Invokes the sucessive steps needed to read the defined table.
	*/
	int run();


	/*
	** Method that defines how the data is written in the corresponding sheet.
	** The process is done in the following steps:
	** - extract the data sheet from the spreadsheet file to the temp folder
	** - open the resulting xml file
	** - get the node that has the spreadsheet rows as children
	** - derive the bounds of the spreadsheet table
	** - map shared strings
	** - insert the data nodes in the XML considering flag and write options
	** - update the data sheet in the temporary folder
	** - check if new shared strings were added and update the temporary file acordingly
	** - copy the inner files of the original spreadsheet that were not changed to a temporary zip file
	** - add the modified files to the temporary zip file
	** replace the original document with the modified one
	*/
	int manage_data();


	int manage_data2D();


	/*
	** Writes the information of the provided AMPL table to the given spreadsheet.
	** For each value in an AMPL table row it searches for the corresponding column in
	** the spreadsheet table and writes the value. It is not required that the order of the
	** columns in the spreadsheet table is the same as the order in the AMPL table.
	** Parameters:
	** - node is an XML node that holds the spreadsheet rows as children;
	** - remaining parameters define the bounds od the table;
	** Returns:
	** - 0 success;
	** Currently not checking errors;
	*/
	int write_data_out(
		pugi::xml_node node,
		int first_row,
		int last_row,
		std::string &first_col,
		std::string &last_col
	);

	/*
	** Writes the information of the provided AMPL table to the given spreadsheet.
	** Unlike write_data_out we assume that the spreadsheet table was droped (so data and headers
	** were deleted) so we write the data in the column sequence presented in the AMPL table.
	** Parameters:
	** - node is an XML node that holds the spreadsheet rows as children;
	** - remaining parameters define the bounds od the table;
	** Returns:
	** - 0 success;
	** Currently not checking errors;
	*/
	int write_all_data_out(
		pugi::xml_node node,
		int first_row,
		int last_row,
		std::string &first_col,
		std::string &last_col
	);

	/*
	** Writes the information of the provided AMPL table to the given spreadsheet but respects the
	** row order of the spreadsheet table. The function process is described in following pseudocode:
	**
	**   forall rows in spreadsheet:
	**       get the row information of the arity (key) columns
	**       get the AMPL row with the same keys in the AMPL table
	**       copy the ramaining information from the AMPL row to the spreadsheet row
	**
	** Parameters:
	** - node is an XML node that holds the spreadsheet rows as children;
	** - remaining parameters define the bounds of the table;
	** Returns:
	** - 0 success;
	** - 1 in case of error;
	*/
	int write_data_inout(
		pugi::xml_node node,
		int first_row,
		int last_row,
		std::string &first_col,
		std::string &last_col
	);


	/*
	** Writes the value of AMPL DbCol[trow] at the XML node excel_cell.
	** excel_cell is created if it does not exist.
	** Parameters:
	** - db : pointer to a DbCol of the AMPL table;
	** - db_row: considered row in AMPL DbCol;
	** - xl_cell : cell in the spreadsheet row that will hold the data;
	*/
	void set_cell_value(
		DbCol *db,
		int db_row,
		pugi::xml_node xl_cell
	);

	/*
	** Updates the shared strings XML in the current temp folder.
	** Parameters:
	**   init_size: the initial number of shared strings, after reading the XML;
	** Returns:
	**   0 success;
	**   1 error opening XML;
	*/
	int update_shared_strings(int init_size);


	/*
	** Gets the values of the arity columns of a given spreadsheet row.
	** The values are added to the excel_keys vector.
	** Parameters:
	** - excel_row: an XML node of the row from which we want to get the keys;
	** - row: the index of the provided excel row, used to search within the row's cells
	** Returns:
	** - 0 success;
	** - 1 error, could not find a given key;
	*/
	int get_excel_keys(pugi::xml_node excel_row, int row);


	/*
	** Gets the values of the arity columns of a given AMPL row.
	** The values are added to the ampl_keys vector.
	** Parameters:
	** - row: the index of the ampl row;
	** Returns:
	** - 0 success;
	** Not checking for errors.
	*/
	int get_ampl_keys(int row);


	/*
	** Used in write_data_inout.
	** This method assumes excel_keys was already calculated.
	** Searches for the row in AMPL's table that has the same keys as excel_keys.
	** Right now it is doing a simple linear seach that could prove ineficient for large tables.
	** This could be improved with a mapping of the keys of an AMPL row to the rows index.
	** Returns:
	**     -1 if the row was not found;
	**     row index otherwise.
	*/
	int get_ampl_row();


	/*
	** Used in write_data_inout.
	** Copies the information of an AMPL row to a spreadsheet row.
	** Only the non arity columns are copied, since we assume the row with the keys is already
	** in the spreadsheet.
	** Parameters:
	**     excel_row : XML node of the node were the data will be written;
	**     row : index of the spreadsheet row;
	**     ampl_row : the index of the AMPL row with the data.
	** Returns:
	**      0 success;
	** Not checking for errors.
	*/
	int copy_info(pugi::xml_node excel_row, int row, int ampl_row);


	/*
	** Deletes the data of the table in the given spreadsheet.
	** It checks the drop flag, to decide if the header (column names) should be deleted or not
	** and calls a specific delete method, according to the table placeholder.
	** Returns:
	**      0 success;
	** Not checking for errors.
	*/
	int delete_data(pugi::xml_node parent);


	/*
	** Deletes the data of the table defined by the spreadsheet range.
	** Only cell elements in each row are deleted, since the row may have elements of other tables.
	** Parameters:
	**    parent : XML node that holds row elements as children;
	**    include_header : weather to delete the header or not.
	** Returns:
	**      0 success;
	** Not checking for errors.
	*/
	int delete_range(pugi::xml_node parent, int include_header);


	/*
	** Deletes the data of the table defined by the spreadsheet header range.
	** Only cell elements in each row are deleted, since the row may have elements of other tables.
	** Row deletion is done until an empty row in the table columns is found.
	** Parameters:
	**    parent : XML node that holds row elements as children;
	**    include_header : weather to delete the header or not.
	** Returns:
	**      0 success;
	** Not checking for errors.
	*/
	int delete_header_range(pugi::xml_node parent, int include_header);


	/*
	** Deletes the data of the table defined by sheet name.
	** In this particular case we check weather to remove or not the header row and completely
	** remove the xml nodes of rows, since we assume there are no other tables in this sheet.
	** Parameters:
	**     parent : XML node that holds row elements as children;
	**     include_header : weather to delete the header or not.
	** Returns:
	**     0 success;
	** Not checking for errors.
	*/
	int delete_sheet(pugi::xml_node parent, int include_header);


	/*
	** Writes the header (column names) of a given AMPL table in the defined spreadsheet.
	** The method confirms that the string exists in the shared_strings map, if not it is added.
	** Parameters:
	**     parent : XML node that holds spreadsheet rows as children;
	**     first_row : number of the row in the spreadsheet to write the information;
	**     first_col : column of the spreadsheet row to start writting the header.
	** Returns:
	**     0 success;
	** Not checking for errors.
	*/
	int write_header(pugi::xml_node parent, int first_row, std::string & first_col, std::map<std::string, pugi::xml_node> & cell_map);

	int
	write_header_2D(
		pugi::xml_node parent,
		int first_row,
		std::string & first_col,
		std::map<std::string, pugi::xml_node> & cell_map,
		std::map<std::string, std::string> & xl_col_map
	);

	void write_arity_cells(pugi::xml_node row_node, int xl_row, int db_row);


	int write_data_out_2D(
		pugi::xml_node node,
		int first_row,
		int last_row,
		std::string &first_col,
		std::string &last_col
	);

	int write_data_inout_2D(
		pugi::xml_node node,
		int first_row,
		int last_row,
		std::string &first_col,
		std::string &last_col
	);

	int get_excel_keys_2D(
		pugi::xml_node excel_row,
		int row,
		int h_set_pos
	);

	int get_ampl_keys_2D(int line, int h_set);

	int count_2D_rows(std::map<std::vector<std::string>, int> & key_set, int pos);

	void delete_range_values(
		pugi::xml_node parent,
		int first_row,
		int last_row,
		const std::string & first_col,
		const std::string & last_col
	);


};


/*
** Search for a string terminated in .xlsx or .xlsm in the TI (AMPLs table info structure)
** that corresponds to the path to the file we want to open.
*/
std::string get_excel_path(TableInfo *TI);


/*
** Gets the file extension of a file name, i.e. the substring after the last dot.
*/
std::string get_file_extension(const std::string& filepath);


/*
** Joins the folder name temp_folder and the file name excel_file into a single string path.
** Separators added acording to _WIN32 or other arch.
*/
void join_path(
	std::string &temp_folder,
	std::string &excel_file,
	std::string &path
);


/*
** Prints the information present in AMPLs table info structure.
*/
void inspect_ti(AmplExports *ae, TableInfo *TI);


/*
** Prints the information of the table present in AMPLs table info structure.
*/
void inspect_values(AmplExports *ae, TableInfo *TI);


/*
** Confirms that the XML row nodes required to write the table are already present
** in the structure, otherwise they are created and added.
** Parameters:
**     node : XML node that holds spreadsheet rows as children;
**     first_row : number of the first row in the spreadsheet to write the information;
**     last_row : number of the last row in the spreadsheet to write the information;
** Returns:
**     1 if all rows already present;
**     0 if extra rows were added;
*/
int check_rows(pugi::xml_node node, int first_row, int last_row);


/*
** Gets the XML node that defines a given row in the spreadsheet.
** Parameters:
**     parent : XML node that holds spreadsheet rows as children;
**     row : the number of the row we want to retrieve
** Returns:
**     pugi::xml_node of the requested row (might be NULL).
*/
pugi::xml_node get_excel_row(pugi::xml_node parent, int row);


/*
** Gets the XML node that defines a given cell in the spreadsheet.
** Parameters:
**     parent : XML node of the cell's row;
**     row : the number of the row were the cell is;
**     col : string of the column of the requested cell;
** Returns:
**     pugi::xml_node of the requested cell (might be NULL).
*/
pugi::xml_node get_xl_cell(pugi::xml_node parent, int row, std::string &col);


/*
** Removes quotes from a given string. The operation is done inplace.
** Parameters:
**     str : string to remove quotes;
*/
void unquote_string(std::string &str);


void
get_maps(
	pugi::xml_node parent,
	std::map<std::string, pugi::xml_node> & row_map,
	std::map<std::string, pugi::xml_node> & cell_map,
	Logger & logger
);


int
check_table_cells(
	pugi::xml_node parent,
	std::map<std::string, pugi::xml_node> & row_map,
	std::map<std::string, pugi::xml_node> & cell_map,
	int first_row,
	int last_row,
	std::string & first_col,
	std::string & last_col,
	Logger & logger
);

void
add_missing_cells(
	pugi::xml_node row,
	int row_num,
	std::vector<std::string> & col_range,
	std::map<std::string, pugi::xml_node> & cell_map,
	Logger & logger
);


int
cell_reference_to_number(std::string & s);

std::string
number_to_cell_reference(int n);



double
clock_to_seconds(std::clock_t start_time, std::clock_t end_time);

std::clock_t get_time();

void
fill_range(std::vector<std::string> & col_range, std::string & first_col, std::string & last_col);


void
add_range_cells(pugi::xml_node row, int row_num, std::vector<std::string> & col_range, std::map<std::string, pugi::xml_node> & cell_map);



pugi::xml_node
row_insert_cell(pugi::xml_node row_node, int row_num, std::string & cell_col);


void
cell_add_basic_attrs(pugi::xml_node node, std::string & cell_ref);


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












