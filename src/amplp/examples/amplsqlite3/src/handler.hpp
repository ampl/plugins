#pragma once

#ifdef _WIN32
	#include <windows.h>
#endif

#include <cstring>

#include "sqlite3.h"
#include "amplp.hpp"

using namespace amplp;

static std::string name = "amplsqlite3";
static std::string version = "0.0.0";

class Handler:
public TableConnector{

	private:

	sqlite3 *db;
	std::string sql;
	std::string write;
	bool create_primary_keys;
	std::vector<int> amplcoltypes; // 0 numeric, 1 string, 2 mixed

	// override functions
	void register_handler_names(){
		log_msg = "<register_handler_names>";
		logger.log(log_msg, LOG_DEBUG);
		handler_names = {name, name + ".dll"};
	};
	void register_handler_extensions();
	void register_handler_args();
	void register_handler_kargs();
	void generate_table();
	void validate_arguments();

	void check_table();
	void read_in();
	void write_out();
	void write_inout();

	std::string get_stmt_create();
	std::string get_stmt_insert();
	std::string get_stmt_select();
	std::string get_stmt_update();
	std::string get_stmt_delete();
	std::string get_stmt_exists();
	std::string get_stmt_drop();

	bool table_exists();
	void table_create();
	void table_delete();
	void table_drop();

	void get_ampl_col_types();
	void validate_transaction(bool all_good);
	void check_prepare(sqlite3_stmt *stmt, int rc);
	void log_step(int rc);

	public:

	Handler(AmplExports *ae, TableInfo *TI) : TableConnector(ae, TI){
		handler_version = name + " - " + version;
		write = "DELETE";
		create_primary_keys = true;
	};

	~Handler();
};

static std::string doc = name + "\n" +
name + " - " + version + "\n" +
"sqlite3 - " + sqlite3_version + "\n" +
"A table handler for sqlite3 databases.\n"
"\n"
"General information on table handlers and data correspondence between AMPL and \n"
"an external table is available at chapter 10 of the AMPL book:\n"
"\n"
"    https://ampl.com/learn/ampl-book/\n"
"\n"
"Information on sqlite3 is available at\n"
"\n"
"    https://www.sqlite.org\n"
"\n"
"The available options for amplsqlite3 are:\n"
"\n"
"alias:\n"
"    Instead of writing the data to the table with the name defined in the table\n"
"    declaration it's possible define the table name with an alias. In the\n"
"    following example the table handler will search for a table named \"bar\"\n"
"    instead of a table named \"foo\" as in the table declaration.\n"
"\n"
"    Example:\n"
"        table foo OUT \"amplsqlite3\" \"bar\": [A], B;\n"
"\n"
"external-table-spec:\n"
"    Specifies the path to the .db file to be read or written with the read \n"
"    table and write table commands. If no file is specified, amplsqlite3 will\n"
"    search for a file with the table name and the .db file extension in the\n"
"    current directory. If the table is to be written and the file does not exist\n"
"    it will be created.\n"
"\n"
"    Example:\n"
"        table foo OUT \"amplsqlite3\" \"bar.db\": [keycol], valcol;\n"
"\n"
"verbose:\n"
"    Display warnings during the execution of the read table and write table\n"
"    commands.\n"
"\n"
"    Example:\n"
"        table foo OUT \"amplsqlite3\" \"verbose\": [keycol], valcol;\n"
"\n"
"verbose=option:\n"
"    Display information according to the specified option. Available options:\n"
"        0 (default) - display information only on error,\n"
"        1 - display warnings,\n"
"        2 - display general information\n"
"        3 - display debug information.\n"
"\n"
"    Example:\n"
"        table foo OUT \"amplsqlite3\" \"verbose=2\": [keycol], valcol;\n"
"\n"
"write=option\n"
"    Define how the data is written in OUT mode. Available options:\n"
"        delete (default) - deletes all the rows of the corresponding table (if\n"
"        it exists) before writing the data from AMPL.\n"
"        drop - drops the corresponding table and creates a new one.\n"
"\n"
"    Example:\n"
"        table foo OUT \"amplsqlite3\" \"write=drop\": [keycol], valcol;\n"
;
