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

static std::string doc = name + "\n" + name + "-" + version + "\n" +
"amplsqlite3: experimental sqlite3 driver for AMPL.\n"
;

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

	public:

	Handler(AmplExports *ae, TableInfo *TI) : TableConnector(ae, TI){
		handler_version = name + " - " + version;
		write = "DELETE";
		create_primary_keys = true;
	};

	~Handler();
};
