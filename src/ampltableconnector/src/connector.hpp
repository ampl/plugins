#pragma once


// current version of the handler
const std::string version = "examplehandler - 0.0.0";


// We need a named enum with DB return values to use in try/catch
enum DBE{	/* return values from (*DbRead)(...) and (*DbWrite)(...) */
	DBE_Done = 0,	/* Table read or written. */
	DBE_Refuse = 1,	/* Refuse to handle this table. */
	DBE_Error = 2	/* Error reading or writing table. */
};


// base class with attributes and methods usefull both to the reader and writer
class Connector{

	public:

	// connections to AMPL
	TableInfo *TI;
	AmplExports *ae;

	// name of the table
	std::string table_name;

	// path for file (or something else) to read/write
	std::string filepath;

	// inout keyword of the table, can be IN, OUT or INOUT
	std::string inout;

	// names that will identify this table handler in the table declaration
	std::vector<std::string> handler_names;

	// file extensions accepted by the handler
	std::vector<std::string> handler_extensions;

	// structures to parse the provided arguments:
	// args - vector of strings of individual arguments passed by AMPL
	std::vector<std::string> args;
	// kargs_map - map of the arguments separated by an "=". An argument of the form "key=val" will
	// be automatically processed and accessed with kargs_map[key] = val.
	std::map<std::string, std::string> kargs_map;
	// kargs - vector of keys in kargs_map, used to preserve the order in which the keys were read
	std::vector<std::string> kargs;

	// in order to have a single parser for the reader/writer we need to be able to identify both
	bool is_writer;

	// for errors and messages to users 
	std::string log_msg;
	Logger logger;

	Connector();
	virtual ~Connector();

	// add pointers to comunicate with AMPL 
	void add_ampl_connections(AmplExports *ae, TableInfo *TI);

	// check if the string arg ends with any of the previously defined extensions
	bool is_handler_extensions(const std::string & arg);

	// parses all the arguments passed by ampl to the structures args, kargs_map and kargs. Filters
	// the verbose flag and updates the level in the logger. Launches warnings for redundant/unused
	// arguments.
	void parse_arguments();

	// Validates the structures derived from the parsing.
	void validate_arguments();

	// Check if a path to an external file was defined, otherwise constructs a path based on the
	// table name and the provided extensions.
	void validate_filepath();

	// Table info functions

	// returns the numer of arity columns in AMPL's table
	int nkeycols();

	// returns the number of non arity (data) columns in AMPL's table
	int ndatacols();

	// returns the total number of columns in AMPL's table
	int ncols();

	//returns the number of rows in AMPL's table
	int nrows();

	void set_col_val(double val, int col);
	void set_col_val(std::string & val, int col);
	void set_col_val(char* val, int col);

	double get_numeric_val(int row, int col);
	char* get_char_val(int row, int col);

	bool is_numeric_val(int row, int col);
	bool is_char_val(int row, int col);

	char* get_col_name(int col);

	void add_row();

	// AMPL Export functions

	int ampl_fprintf(FILE* stream, const char* format, ...);

	int ampl_printf(const char* format, ...);

	int ampl_sprintf(char* str, const char* format, ...);

	int ampl_vfprintf(FILE* stream, const char* format, va_list arg);

	int ampl_vsprintf(char* buffer, const char* format, va_list arg);

	double ampl_strtod(const char* str, char** endptr);

	// parse and validate arguments, ensure the external table is found
	void prepare();

	// read data from external table
	// additional methods could be added here, if needed
	void run();

	// read data from external table
	virtual void read_in() = 0;

	// overwrite external table
	virtual void write_out() = 0;

	// update external table
	virtual void write_inout() = 0;

	// if the external table does not exist we need a function to create it
	virtual void generate_table() = 0;

	// let AMPL know how to invoke this handler
	virtual void register_handler_names() = 0;

	// define what type of file extensions will be accepted
	virtual void register_handler_extensions() = 0;
};
