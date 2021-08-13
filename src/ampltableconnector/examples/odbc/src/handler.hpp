#pragma once

#ifdef _WIN32
	#include <windows.h>
#endif

#include <sql.h>
#include <sqlext.h>
#include <cstring>

#include "ampltableconnector.hpp"

using namespace amplt;

//https://docs.microsoft.com/en-us/sql/odbc/reference/appendixes/c-data-types?view=sql-server-ver15
std::unordered_set<SQLSMALLINT> sql_num_types ({
	SQL_NUMERIC,
	SQL_DECIMAL,
	SQL_INTEGER,
	SQL_SMALLINT,
	SQL_FLOAT,
	SQL_REAL,
	SQL_DOUBLE
});


static std::string name = "eodbc";
static std::string version = "0.0.0";

static std::string doc = name + "\n" + name + "-" + version + "\n" +
"ODBC experimental driver.\n"
;

class Handler:
public Connector{

	private:

	std::string driver;
	std::string sql;
	std::string dsn;
	std::string write;
	bool autocommit;
	std::vector<int> amplcoltypes; // 0 numeric, 1 string, 2 mixed
	std::map<int, SQLSMALLINT> odbccoltypes;

	SQLHENV henv; // Environment
	SQLHDBC hdbc; // Connection handle
	SQLHSTMT hstmt;// Statement handle
	SQLRETURN retcode; // Return status

	// vectors that require allocation (cleanup in the dtor) 
	std::vector<SQLCHAR *> ColumnName;
	std::vector<SQLCHAR *> ColumnData;

	// override functions
	void register_handler_names(){
		log_msg = "<register_handler_names>";
		logger.log(log_msg, LOG_DEBUG);
		handler_names ={name, name + ".dll"};
	};
	void register_handler_extensions();
	void register_handler_args();
	void register_handler_kargs();
	void generate_table();
	void validate_arguments();

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

	void alloc_and_connect();
	bool table_exists();
	void table_create();
	void table_delete();
	void table_drop();

	void get_ampl_col_types();
	void get_odbc_col_types();
	//~ void clean_handle_stmt();

	void
	check_error(
		SQLRETURN e,
		char *s,
		SQLHANDLE h,
		SQLSMALLINT t
	);


	void
	extract_error(
    char *fn,
    SQLHANDLE handle,
    SQLSMALLINT type);

	std::unordered_set<SQLSMALLINT>
	get_db_supported_types();





	public:

	Handler(AmplExports *ae, TableInfo *TI) : Connector(ae, TI){
		handler_version = name + " - " + version;
		henv = NULL;
		hdbc = NULL;
		hstmt = NULL;
		retcode = 0;
		write = "DELETE";
		autocommit = false;
	};

	~Handler();
};

template <class T>
void clean_vector(std::vector<T> v) {
	if (v.size() > 0){
		for (size_t i=0; i<v.size(); i++){
			if (v[i] != NULL){
				delete v[i];
			}
		}
	}
	v.clear();
};


