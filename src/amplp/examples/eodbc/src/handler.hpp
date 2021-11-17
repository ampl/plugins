#pragma once

#ifdef _WIN32
	#include <windows.h>
#endif

#include <sql.h>
#include <sqlext.h>
#include <cstring>

#include "amplp.hpp"

using namespace amplp;

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

static int MAX_COL_NAME_LEN = 255;

std::unordered_map<SQLSMALLINT, std::string> odbc_types_map({
	{SQL_UNKNOWN_TYPE, "SQL_UNKNOWN_TYPE"},
	{SQL_CHAR, "SQL_CHAR"},
	{SQL_NUMERIC, "SQL_NUMERIC"},
	{SQL_DECIMAL, "SQL_DECIMAL"},
	{SQL_INTEGER, "SQL_INTEGER"},
	{SQL_SMALLINT, "SQL_SMALLINT"},
	{SQL_FLOAT, "SQL_FLOAT"},
	{SQL_REAL, "SQL_REAL"},
	{SQL_DOUBLE, "SQL_DOUBLE"},
	{SQL_DATETIME, "SQL_DATETIME"},
	{SQL_VARCHAR, "SQL_VARCHAR"},
	{SQL_TYPE_DATE, "SQL_TYPE_DATE"},
	{SQL_TYPE_TIME, "SQL_TYPE_TIME"},
	{SQL_TYPE_TIMESTAMP, "SQL_TYPE_TIMESTAMP"}
});


std::unordered_map<SQLSMALLINT, std::string> c_types_map({
	{SQL_C_LONG, "SQL_C_LONG"},
	{SQL_C_DOUBLE, "SQL_C_DOUBLE"},
	{SQL_C_CHAR, "SQL_C_CHAR"}
});

static std::string name = "eodbc";
static std::string version = "0.0.1";

static std::string doc = name + "\n" + name + "-" + version + "\n" +
"EODBC: experimental ODBC driver for AMPL.\n"
"\n"
"Main differences from previous ODBC driver: \n"
"- Autocommit is off by default, leading to faster write times.\n"
"- Table columns must contain numeric or character data. Columns with both types\n"
"  are not supported.\n"
"- No numerical conversion from/to timestamp columns. Data from the mentioned\n"
"  type will be loaded as character data. \n"
"- In OUT mode, by default, tables will be deleted rather than dropped.\n"
"- INOUT mode will use an SQL update statement.\n"
"- If a table created by AMPL has key columns they will be declared as primary\n"
"  keys.\n"
"- Files to load the data from must be declared in the DBQ option of the \n"
"  connection string.\n"
"- Explicit loading of the library with the command \"load eodbc.dll;\" is needed.\n"
"\n"
"General information on table handlers and data correspondence between AMPL and\n"
"an external table is available at:\n"
"\n"
"    https://ampl.com/BOOK/CHAPTERS/13-tables.pdf\n"
"\n"
"The available options for eodbc are (cs denotes the connectionstring for the\n"
"data provider in use):\n"
"\n"
"alias:\n"
"    Instead of using the string after the table keyword to define the table name\n"
"    to read/write/update the data from/to it is possible to define an alias.\n"
"    This is particularly useful when you need multiple declarations to\n"
"    read/write/update data from/to the same table.\n"
"    When writing data, if the table does not exist, it will be created.\n"
"\n"
"    Example:\n"
"        table tablename OUT \"eodbc\" (cs) \"tablealias\": [A], B;\n"
"\n"
"autocommit=option:\n"
"    Whether or not to interpret every database operation as a transaction.\n"
"    Options: true, false (default).\n"
"\n"
"    Example:\n"
"        table tablename OUT \"eodbc\" (cs) \"autocommit=false\": [A], B;\n"
"\n"
"connectionstring:\n"
"    An explicit ODBC connection string of the form \"DSN=...\" or \"DRIVER=...\".\n"
"    Additional fields depend on the data provider.\n"
"\n"
"    Example:\n"
"        param connectionstring symbolic := \"DRIVER=...;DATABASE=...;USER=...;\";\n"
"        table tablename IN \"eodbc\" (connectionstring): [A], B;\n"
"\n"
"SQL=statement:\n"
"    (IN only) Provide a particular SQL statement to read data into AMPL.\n"
"\n"
"    Example:\n"
"        table tablename IN \"eodbc\" (cs) \"SQL=SELECT * FROM sometable;\": [A], B;\n"
"\n"
"verbose:\n"
"    Display warnings during the execution of the read table and\n"
"    write table commands.\n"
"\n"
"    Example:\n"
"        table tablename OUT \"eodbc\" \"verbose\": [keycol], valcol;\n"
"\n"
"verbose=option:\n"
"    Display information according to the specified option. Available\n"
"    options:\n"
"        0 (default) - display information only on error,\n"
"        1 - display warnings,\n"
"        2 - display general information\n"
"        3 - display debug information.\n"
"\n"
"    Example:\n"
"        table tablename OUT \"eodbc\" (cs) \"verbose=2\": [keycol], valcol;\n"
"\n"
"write=option\n"
"    Define how the data is written in OUT mode. Available options:\n"
"        delete (default) - delete the rows in the external table before\n"
"            writing the data from AMPL.\n"
"        drop - drop the current table and create a new one before writing the\n"
"            data. The new table will only have double and varchar columns,\n"
"            depending on the original data from AMPL and the types available in\n"
"            the database.\n"
"        append - append the rows in AMPL to the external representation of the\n"
"            table.\n"
"\n"
"    Example:\n"
"        table tablename OUT \"eodbc\" (cs) \"write=append\": [keycol], valcol;\n"
;

class Handler:
public TableConnector{

	private:

	std::string driver;
	std::string sql;
	std::string dsn;
	std::string write;
	bool autocommit;
	std::vector<int> amplcoltypes; // 0 numeric, 1 string, 2 mixed
	std::map<int, SQLSMALLINT> t_num_types; // odbc table numeric type for each column (0 indexed)
	std::map<int, std::string> t_str_types; // odbc table string type for each column (0 indexed)

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
		const char *s,
		SQLHANDLE h,
		SQLSMALLINT t
	);


	void
	extract_error(
    const char *fn,
    SQLHANDLE handle,
    SQLSMALLINT type);

	std::unordered_set<SQLSMALLINT>
	get_db_supported_types();

	std::unordered_map<std::string, int>
	get_table_types();


	std::string get_timestamp_info(TIMESTAMP_STRUCT* ts);
	std::string get_date_info(tagDATE_STRUCT* ts);
	std::string get_time_info(tagTIME_STRUCT* ts);

	void set_date_info(std::string & date_str, SQL_DATE_STRUCT & DateData);
	void set_time_info(std::string & date_str, SQL_TIME_STRUCT & DateData);
	void set_timestamp_info(std::string & date_str, SQL_TIMESTAMP_STRUCT & DateData);

	void describe_cols(int ncols);

	std::vector<std::string>
	get_sql_colnames(const std::string & sql);

	void
	process_tokens(const std::vector<std::string> & tokens, std::vector<std::string> & colnames);

	void
	process_list(const std::vector<std::string> & tokens, std::vector<std::string> & colnames);

	std::string
	get_SQLBindParameter_string(int paramnum, int amplcol, int ttype, int ctype, int sqltype);

	public:

	Handler(AmplExports *ae, TableInfo *TI) : TableConnector(ae, TI){
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







