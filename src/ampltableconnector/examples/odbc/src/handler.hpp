#pragma once

#include <sql.h>
#include <sqlext.h>
#include <cstring>

#include "ampltableconnector.hpp"

using namespace amplt;

static std::string name = "eodbc";
static std::string version = "-1";

static std::string doc = name + "\n" + name + "-" + version + "\n" +
"ODBC experimental driver.\n"
;

class Handler:
public Connector{

	private:

	std::string driver;
	std::string database;
	std::string user;
	std::string password;
	std::string sql;
	bool autocommit;
	bool append;


	SQLHENV henv; // Environment
	SQLHDBC hdbc; // Connection handle
	SQLHSTMT hstmt;// Statement handle
	SQLRETURN retcode; // Return status


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
	void write_out2();
	void write_inout();

	std::string get_stmt_insert();
	std::string get_stmt_select();
	std::string get_stmt_update();
	std::string get_stmt_delete();

	void alloc_and_connect();


	void
	CHECK_ERROR2(
		SQLRETURN e,
		char *s,
		SQLHANDLE h,
		SQLSMALLINT t
	);


	void
	extract_error2(
    char *fn,
    SQLHANDLE handle,
    SQLSMALLINT type);







	public:

	Handler(AmplExports *ae, TableInfo *TI) : Connector(ae, TI){
		handler_version = name + " - " + version;
		henv = NULL;
		hdbc = NULL;
		hstmt = NULL;
		retcode = 0;
		autocommit = false;
		append = false;
	};

	~Handler();
};
