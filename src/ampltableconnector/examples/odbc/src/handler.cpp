#include "handler.hpp"


static int
Read_odbcx(AmplExports *ae, TableInfo *TI){

	int res = DBE_Done;
	Handler cn(ae, TI);

	try{
		cn.run();
	}
	catch (DBE e){
		// something went wrong
		// the error must be in the logger
		res = e;
	}
	return res;
};


static int
Write_odbcx(AmplExports *ae, TableInfo *TI){

	int res = DBE_Done;
	Handler cn(ae, TI);
	cn.is_writer = true;

	try{
		cn.run();
	}
	catch (DBE e){
		// something went wrong
		// the error must be in the logger
		res = e;
	}
	// everything OK
	return res;
};


void
funcadd(AmplExports *ae){

	// Inform AMPL about the handlers
	add_table_handler(ae, Read_odbcx, Write_odbcx, const_cast<char *>(doc.c_str()), 0, 0);
};

// Adapt the functions bellow to meet your requirements

Handler::~Handler(){

	// Free handles
	// Statement
	if (hstmt != SQL_NULL_HSTMT){
		SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
	}

	// Connection
	if (hdbc != SQL_NULL_HDBC) {
		SQLDisconnect(hdbc);
		SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
	}

	// Environment
	if (henv != SQL_NULL_HENV){
		SQLFreeHandle(SQL_HANDLE_ENV, henv);
	}
};




void
Handler::read_in(){

	log_msg = "<read_in>";
	logger.log(log_msg, LOG_DEBUG);

	//https://www.easysoft.com/developer/languages/c/examples/DescribeAndBindColumns.html

	std::string selectstr;
	if (sql.empty()){
		selectstr = get_stmt_select();
	}
	else{
		selectstr = sql;
	}
	std::cout << "selectstr: " << selectstr << std::endl;

	// Prepare Statement
	retcode = SQLPrepare (hstmt, selectstr.c_str(), strlen(selectstr.c_str()));
	CHECK_ERROR2(retcode, "SQLPrepare(SQL_HANDLE_ENV)",
				hstmt, SQL_HANDLE_STMT);

	SQLSMALLINT numCols;

	// Retrieve number of columns
	retcode = SQLNumResultCols (hstmt, &numCols);
	CHECK_ERROR2(retcode, "SQLNumResultCols()", hstmt,
				SQL_HANDLE_STMT);

	printf ("Number of Result Columns %i\n", numCols);

	int MAX_COL_NAME_LEN = 256;

	// vectors to describe and bind columns
	std::vector<SQLCHAR *>      ColumnName(numCols);
	std::vector<SQLSMALLINT>    ColumnNameLen(numCols);
	std::vector<SQLSMALLINT>    ColumnDataType(numCols);
	std::vector<SQLULEN>        ColumnDataSize(numCols);
	std::vector<SQLSMALLINT>    ColumnDataDigits(numCols);
	std::vector<SQLSMALLINT>    ColumnDataNullable(numCols);
	std::vector<SQLCHAR *>      ColumnData(numCols);
	std::vector<SQLLEN>         ColumnDataLen(numCols);

	std::vector<int> perm(numCols, -1);
	std::vector<bool> foundcol(ncols(), false);

	for (int i=0;i<numCols;i++) {
		ColumnName[i] = (SQLCHAR *) malloc (MAX_COL_NAME_LEN);
		retcode = SQLDescribeCol (
					hstmt,                    // Select Statement (Prepared)
					i+1,                      // Columnn Number
					ColumnName[i],            // Column Name (returned)
					MAX_COL_NAME_LEN,         // size of Column Name buffer
					&ColumnNameLen[i],        // Actual size of column name
					&ColumnDataType[i],       // SQL Data type of column
					&ColumnDataSize[i],       // Data size of column in table
					&ColumnDataDigits[i],     // Number of decimal digits
					&ColumnDataNullable[i]);  // Whether column nullable

		CHECK_ERROR2(retcode, "SQLDescribeCol()", hstmt, SQL_HANDLE_STMT);

		// Display column data
		printf("\nColumn : %i\n", i+1);
		printf("Column Name : %s\n  Column Name Len : %i\n  SQL Data Type : %i\n  Data Size : %i\n  DecimalDigits : %i\n  Nullable %i\n",
				 ColumnName[i], (int)ColumnNameLen[i], (int)ColumnDataType[i],
				 (int)ColumnDataSize[i], (int)ColumnDataDigits[i],
				 (int)ColumnDataNullable[i]);

		// Bind column, changing SQL data type to C data type
		// (assumes INT and VARCHAR for now)
		ColumnData[i] = (SQLCHAR *) malloc (ColumnDataSize[i]+1);
		switch (ColumnDataType[i]) {
			case SQL_DOUBLE:
				ColumnDataType[i]=SQL_C_DOUBLE;

			break;
			case SQL_VARCHAR:
				ColumnDataType[i]=SQL_C_CHAR;

			break;
		}
		retcode = SQLBindCol (hstmt,                  // Statement handle
							  i+1,                    // Column number
							  ColumnDataType[i],      // C Data Type
							  ColumnData[i],          // Data buffer
							  ColumnDataSize[i],      // Size of Data Buffer
							  &ColumnDataLen[i]); // Size of data returned

		CHECK_ERROR2(retcode, "SQLBindCol()", hstmt, SQL_HANDLE_STMT);

		for (int j = 0; j < ncols(); j++){
			std::string ampl_col = get_col_name(j);
			std::string odbc_col = (char*)ColumnName[i];
			if (ampl_col == odbc_col){
				perm[i] = j;
				foundcol[j] = true;
			}
		}
	}

	std::cout << "perm: ";
	print_vector(perm);
	std::cout << "foundcol: ";
	print_vector(foundcol);

	for (int j = 0; j < ncols(); j++){
		if (!foundcol[j]){
			std::cout << "Cannot find column: ";
			std::cout << get_col_name(j) << std::endl;
			throw DBE_Error;
		}
	}

	//~ print_vector(ColumnName);
	//~ print_vector(ColumnNameLen);
	//~ print_vector(ColumnDataType);
	//~ print_vector(ColumnDataSize);
	//~ print_vector(ColumnDataDigits);
	//~ print_vector(ColumnDataNullable);
	//~ print_vector(ColumnData);
	//~ print_vector(ColumnDataLen);

	// Fetch records
	printf ("\nRecords ...\n\n");
	retcode = SQLExecute (hstmt);
	CHECK_ERROR2(retcode, "SQLExecute()", hstmt, SQL_HANDLE_STMT);

	printf ("\n  Data Records\n  ------------\n");
	for (int i=0; ; i++) {
		retcode = SQLFetch(hstmt);

		//No more data?
		if (retcode == SQL_NO_DATA) {
			break;
		}

		CHECK_ERROR2(retcode, "SQLFetch()", hstmt, SQL_HANDLE_STMT);

		for (int j = 0; j < numCols; j++) {
			if (ColumnDataType[j] == SQL_C_DOUBLE) {
				set_col_val(*(double*)ColumnData[j], perm[j]);
			} else {
				set_col_val((char*)ColumnData[j], perm[j]);
			}
		}
		add_row();
	}
};


std::string
Handler::get_stmt_select(){
	std::string stmt = "SELECT ";
	for (size_t i = 0; i < ncols(); i++){
		stmt += get_col_name(i);
		if (i + 1 < ncols()){
			stmt += ", ";
		}
	}
	stmt += " FROM ";
	stmt += table_name;
	stmt += ";";

	return stmt;
};


std::string
Handler::get_stmt_insert(){

	std::string stmt = "INSERT INTO ";
	stmt += table_name;
	stmt += " (";

	for (size_t i = 0; i < ncols(); i++){
		stmt += get_col_name(i);
		if (i + 1 < ncols()){
			stmt += ", ";
		}
	}

	stmt += ") VALUES (";
	for (size_t i = 0; i < ncols(); i++){
		stmt += "?";
		if (i + 1 < ncols()){
			stmt += ", ";
		}
	}
	stmt += ");";

	return stmt;
};

std::string
Handler::get_stmt_update(){

	std::string stmt = "UPDATE ";
	stmt += table_name;
	stmt += " SET ";
	for (size_t i = 0; i < ndatacols(); i++){
		stmt += get_col_name(i + nkeycols());
		stmt += " = ";
		stmt += "?";
		if (i + 1 < ndatacols()){
			stmt += ", ";
		}
	}
	stmt += " WHERE ";
	for (size_t i = 0; i < nkeycols(); i++){
		stmt += get_col_name(i);
		stmt += " = ";
		stmt += "?";
		if (i + 1 < nkeycols()){
			stmt += " AND ";
		}
	}
	stmt += ";";

	return stmt;
};

std::string
Handler::get_stmt_exists(){
	std::string stmt = "select case when exists((select * from information_schema.tables where table_name = '"; 
	stmt += table_name;
	stmt += "')) then 1 else 0 end;";

	return stmt;
};

std::string
Handler::get_stmt_drop(){
	std::string stmt = "DROP TABLE "; 
	stmt += table_name;
	stmt += ";";

	return stmt;
};

std::string
Handler::get_stmt_delete(){

	std::string stmt = "DELETE FROM ";
	stmt += table_name;
	stmt += ";";
	return stmt;
};

std::string
Handler::get_stmt_create(){

	std::string stmt = "CREATE TABLE ";
	stmt += table_name;
	stmt += " (";

	for (size_t i = 0; i < ncols(); i++){
		stmt += get_col_name(i);

		if (amplcoltypes[i] == 1){
			stmt += " VARCHAR(255)";
		}
		else{
			stmt += " DOUBLE";
		}

		if (i + 1 < ncols()){
			stmt += ", ";
		}
	}

	stmt += ");";

	return stmt;
};

void
Handler::write_out(){

	log_msg = "<write_out>";
	logger.log(log_msg, LOG_DEBUG);

	if (nrows() == 0){
		std::cout << "No rows to write" << std::endl;
		return;
	}

	// Get the insert statement
	std::string selectstr = get_stmt_insert();
	std::cout << "selectstr: " << selectstr << std::endl;

	// Prepare Statement
	retcode = SQLPrepare (hstmt, selectstr.c_str(), strlen(selectstr.c_str()));
	CHECK_ERROR2(retcode, "SQLPrepare(SQL_HANDLE_ENV)",
				hstmt, SQL_HANDLE_STMT);

	std::cout << "SQLPrepare done" << std::endl;

	SQLSMALLINT NumParams;
	SQLSMALLINT numCols;

	// Retrieve number of columns
	retcode = SQLNumParams(hstmt, &NumParams);
	CHECK_ERROR2(retcode, "SQLNumParams()", hstmt,
				SQL_HANDLE_STMT);

	printf ("Number of Result Parameters %i\n", NumParams);

	// Retrieve number of columns
	retcode = SQLNumResultCols (hstmt, &numCols);
	CHECK_ERROR2(retcode, "SQLNumResultCols()", hstmt,
				SQL_HANDLE_STMT);

	printf ("Number of Result Columns %i\n", numCols);

	// vectors to describe and bind parameters
	std::vector<SQLCHAR *>      ColumnName(ncols());
	std::vector<SQLSMALLINT>    ColumnNameLen(ncols());
	std::vector<SQLSMALLINT>    ColumnDataType(ncols());
	std::vector<SQLULEN>        ColumnDataSize(ncols());
	std::vector<SQLSMALLINT>    ColumnDataDigits(ncols());
	std::vector<SQLSMALLINT>    ColumnDataNullable(ncols());
	std::vector<SQLCHAR *>      ColumnData(ncols());
	std::vector<double>         ColumnDataDouble(ncols());
	std::vector<SQLLEN>         ColumnDataLen(ncols());


	std::vector<SQLSMALLINT>    DataType(ncols());
	std::vector<SQLULEN>        bytesRemaining(ncols());
	std::vector<SQLSMALLINT>    DecimalDigits(ncols());
	std::vector<SQLSMALLINT>    Nullable(ncols());



	int MAX_COL_NAME_LEN = 30;

	// assert ncols() == NumParams

	// bind parameters
	for (int i=0; i<NumParams; i++){

		// Describe the parameter.
		retcode = SQLDescribeParam(hstmt,
								   i+1,
								   &DataType[i],
								   &bytesRemaining[i],
								   &DecimalDigits[i],
								   &Nullable[i]);

		printf("\nSQLDescribeParam() OK\n");
		printf("Data Type : %i, bytesRemaining : %i, DecimalDigits : %i, Nullable %i\n",
									(int)DataType[i], (int)bytesRemaining[i],
									(int)DecimalDigits[i], (int)Nullable[i]);

	}

	//~ return;

	for (int i=0; i<NumParams; i++){
		// alloc acording to type
		ColumnData[i] = (SQLCHAR *) malloc (MAX_COL_NAME_LEN);

		if (amplcoltypes[i] == 0){
			retcode = SQLBindParameter(hstmt, i+1, SQL_PARAM_INPUT, SQL_C_DOUBLE,
										SQL_DOUBLE, 0, 0, (unsigned char*)&ColumnDataDouble[i], 0, NULL);
		}
		else if (amplcoltypes[i] == 1){
			retcode = SQLBindParameter(hstmt, i+1, SQL_PARAM_INPUT, SQL_C_CHAR,
										SQL_VARCHAR, 2, 0, ColumnData[i], 2, NULL);
		}
		else{
			std::cout << "Cannot Bind mixed parameter" << std::endl;
			throw DBE_Error;
		}
		std::string tmp = "SQLBindParameter(";
		tmp += std::to_string(i);
		tmp += ")";
		CHECK_ERROR2(retcode, tmp.c_str(), hstmt, SQL_HANDLE_STMT);
	}

	for (int i=0; i<nrows(); i++){
		for (int j=0; j<ncols(); j++){
			if (amplcoltypes[j] == 0){

				double d = get_numeric_val(i, j);
				//~ ColumnData[j] = &d;
				//~ ColumnDataDouble[j] = (unsigned char*)&d;
				ColumnDataDouble[j] = d;
			}
			else if (amplcoltypes[j] == 1){
				strcpy(ColumnData[j], get_char_val(i, j));
			}
		}
		retcode = SQLExecute(hstmt);
		CHECK_ERROR2(retcode, "SQLExecute()", hstmt,
					SQL_HANDLE_STMT);
	}
	if (!autocommit){
		retcode = SQLEndTran(SQL_HANDLE_ENV, henv, SQL_COMMIT);
	}
};

void
Handler::write_inout(){

	log_msg = "<write_inout>";
	logger.log(log_msg, LOG_DEBUG);

	if (nrows() == 0){
		std::cout << "No rows to update" << std::endl;
		return;
	}

	// Get the update statement
	std::string selectstr = get_stmt_update();
	std::cout << "selectstr: " << selectstr << std::endl;

	// Prepare Statement
	retcode = SQLPrepare (hstmt, selectstr.c_str(), SQL_NTS);
	CHECK_ERROR2(retcode, "SQLPrepare(SQL_HANDLE_ENV)",
				hstmt, SQL_HANDLE_STMT);

	// Permutation of columns derived from the update statement
	std::vector<int> perm(ncols());

	for (int i=0; i<ndatacols(); i++){
		perm[i] = i + nkeycols();
	}
	for (int i=0; i<nkeycols(); i++){
		perm[i + ndatacols()] = i;
	}

	std::cout << "perm: ";
	print_vector(perm);


	int MAX_COL_NAME_LEN = 30;

	char buff[100];


	std::vector<SQLCHAR *>      CData(ncols());
	std::vector<double>         DData(ncols());
	std::vector<SQLLEN> PartIDInd(ncols(), 0);

	for (int i=0; i<ncols(); i++){
		CData[i] = (SQLCHAR *) malloc (MAX_COL_NAME_LEN);
	}

	for (int i=0; i<ncols(); i++){

		int amplcol = perm[i];

		if (amplcoltypes[amplcol] == 0){
			std::cout << "\tnumeric col" << std::endl;
			retcode = SQLBindParameter(
							hstmt,
							i+1,
							SQL_PARAM_INPUT,
							SQL_C_DOUBLE,
							SQL_DOUBLE,
							0,
							0,
							(unsigned char*)&DData[i],
							0,
							NULL
						);
		}
		else if (amplcoltypes[amplcol] == 1){
			std::cout << "\tchar col" << std::endl;
			retcode = SQLBindParameter(
							hstmt, 
							i+1, 
							SQL_PARAM_INPUT, 
							SQL_C_CHAR,
							SQL_VARCHAR, 
							2, 
							0, 
							CData[i], 
							2, 
							NULL);
		}
		std::string tmp = "SQLBindParameter(";
		tmp += std::to_string(i+1);
		tmp += ")";
		CHECK_ERROR2(retcode, tmp.c_str(), hstmt, SQL_HANDLE_STMT);
	}

	std::cout << "Starting update..." << std::endl;

	for (int i=0; i<nrows(); i++){
		for (int j=0; j<ncols(); j++){

			int amplcol = perm[j];

			if (amplcoltypes[amplcol] == 0){
				DData[j] = get_numeric_val(i, amplcol);
			}
			else if (amplcoltypes[amplcol] == 1){
				strcpy(CData[j], get_char_val(i, amplcol));
			}
		}
		retcode = SQLExecute(hstmt);
		CHECK_ERROR2(retcode, "SQLExecute()", hstmt,
					SQL_HANDLE_STMT);
	}

	if (!autocommit){
		retcode = SQLEndTran(SQL_HANDLE_ENV, henv, SQL_COMMIT);
	}
};


void
Handler::generate_table(){

	log_msg = "<generate_table>";
	logger.log(log_msg, LOG_DEBUG);
};


void
Handler::register_handler_extensions(){

	log_msg = "<register_handler_extensions>";
	logger.log(log_msg, LOG_DEBUG);
};


void
Handler::register_handler_args(){

	log_msg = "<register_handler_args>";
	logger.log(log_msg, LOG_DEBUG);
};


void
Handler::register_handler_kargs(){

	log_msg = "<register_handler_kargs>";
	logger.log(log_msg, LOG_DEBUG);

	allowed_kargs = {"autocommit", "write"};
};


void
Handler::validate_arguments(){

	log_msg = "<validate_arguments>";
	logger.log(log_msg, LOG_DEBUG);

	for(const auto it: user_kargs){

		std::string key = it.first;

		if (key == "autocommit"){
			autocommit = get_bool_karg(key);
		}
		else if (key == "write"){
			write = it.second;
		}
	}

	std::string tempstr;
	for (size_t i=0; i<ampl_args.size(); i++){

		std::string arg = ampl_args[i];

		tempstr = "DRIVER=";
		if (!arg.compare(0, tempstr.size(), tempstr)){
			driver = arg;
		}

		tempstr = "SQL=";
		if (!arg.compare(0, tempstr.size(), tempstr)){
			sql = arg.substr(tempstr.size());
			if (is_writer){
				log_msg = "SQL declaration only accepted when reading data. Ignoring: " + arg;
				logger.log(log_msg, LOG_WARNING);
			}
		}
	}

	if (inout != "IN"){
		analyze_columns();
	}

	alloc_and_connect();
	bool exists = table_exists();

	if (!exists){
		if (inout == "IN"){
			std::cout << "No table to read from" << std::endl;
			throw DBE_Error;
		}
		else{
			table_create();
			inout = "OUT";
		}
	}
	else{
		if (write == "DELETE"){
			table_delete();
		}
		else if (write == "DROP"){
			table_drop();
			table_create();
		}
	}
};


void
Handler::alloc_and_connect(){

	// Allocate environment
	retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
	CHECK_ERROR2(retcode, "SQLAllocHandle(SQL_HANDLE_ENV)",
				henv, SQL_HANDLE_ENV);

	// Set ODBC Verion
	retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION,
										(SQLPOINTER*)SQL_OV_ODBC3, 0);
	CHECK_ERROR2(retcode, "SQLSetEnvAttr(SQL_ATTR_ODBC_VERSION)",
				henv, SQL_HANDLE_ENV);

	// Allocate Connection
	retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
	CHECK_ERROR2(retcode, "SQLAllocHandle(SQL_HANDLE_DBC)",
				henv, SQL_HANDLE_DBC);

	// Set Login Timeout
	retcode = SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);
	CHECK_ERROR2(retcode, "SQLSetConnectAttr(SQL_LOGIN_TIMEOUT)",
				hdbc, SQL_HANDLE_DBC);

	// Set Auto Commit
	if (!autocommit){
		retcode = SQLSetConnectAttr(hdbc, SQL_ATTR_AUTOCOMMIT, 0, 0);
		CHECK_ERROR2(retcode, "SQLSetConnectAttr(SQL_ATTR_AUTOCOMMIT)",
															hdbc, SQL_HANDLE_DBC);
	}

	std::string connstr = driver;
	std::cout << "connstr: " << connstr << std::endl;

	retcode = SQLDriverConnect(hdbc, NULL, connstr.c_str(), SQL_NTS,
					 NULL, 0, NULL, SQL_DRIVER_COMPLETE);

	std::cout << "SQLDriverConnect:" << retcode << std::endl;

	// Allocate a statement handle
	SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
	CHECK_ERROR2(retcode, "SQLAllocHandle(SQL_HANDLE_STMT)",
				hstmt, SQL_HANDLE_STMT);




	//~ CHECK_ERROR2(retcode, "SQLConnect(DSN:DATASOURCE;)",
				//~ hdbc, SQL_HANDLE_DBC);
	//~ retcode = SQLConnect(hdbc, connstr.c_str(), SQL_NTS,
							   //~ (SQLCHAR*) NULL, 0, NULL, 0);

	//~ std::cout << "SQLConnect:" << retcode << std::endl;

	//~ std::string tmp = "SQLDriverConnect(";
	//~ tmp += "SQLDriverConnect(

	//~ CHECK_ERROR2(retcode, "SQLDriverConnect(SQL_ATTR_AUTOCOMMIT)",
				//~ hdbc, SQL_HANDLE_DBC);


	//~ // Connect to DSN
	//~ retcode = SQLConnect(hdbc, (SQLCHAR*) "DATASOURCE", SQL_NTS,
							   //~ (SQLCHAR*) NULL, 0, NULL, 0);
	//~ CHECK_ERROR2(retcode, "SQLConnect(DSN:DATASOURCE;)",
				//~ hdbc, SQL_HANDLE_DBC);
};

bool
Handler::table_exists(){

	log_msg = "<table_exists>";
	logger.log(log_msg, LOG_DEBUG);

	std::string selectstr = get_stmt_exists();

	std::cout << "selectstr: " << selectstr << std::endl;

	retcode = SQLExecDirect(hstmt, selectstr.c_str(), strlen(selectstr.c_str()));
	//~ retcode = SQLExecDirect(hstmt, selectstr.c_str(), SQL_NTS);
	CHECK_ERROR2(retcode, "SQLExecDirect(SQL_HANDLE_ENV)",
			hstmt, SQL_HANDLE_STMT);

	SQLUINTEGER count = 0;
	SQLLEN sicount = 0;

	retcode = SQLFetch(hstmt);

	retcode = SQLGetData(hstmt, 1, SQL_C_ULONG, &count, 0, &sicount);

	retcode = SQLFreeStmt(hstmt, SQL_CLOSE);

	std::cout << "count: " << count << std::endl;

	if (count == 0){return false;}
	else if (count == 1){return true;}
	else{
		std::cout << "Unexpected result value" << std::endl;
		throw DBE_Error;
	};

	return true;
};

void
Handler::table_create(){

	log_msg = "<table_create>";
	logger.log(log_msg, LOG_DEBUG);

	std::string selectstr = get_stmt_create();
	std::cout << "selectstr: " << selectstr << std::endl;

	retcode = SQLExecDirect(hstmt, selectstr.c_str(), strlen(selectstr.c_str()));
	CHECK_ERROR2(retcode, "SQLExecDirect(SQL_HANDLE_ENV)",
			hstmt, SQL_HANDLE_STMT);

	std::cout << "Create done." << std::endl;

};

void
Handler::table_delete(){

	log_msg = "<table_delete>";
	logger.log(log_msg, LOG_DEBUG);

	std::string selectstr = get_stmt_delete();
	std::cout << "selectstr: " << selectstr << std::endl;

	retcode = SQLExecDirect(hstmt, selectstr.c_str(), strlen(selectstr.c_str()));
	CHECK_ERROR2(retcode, "SQLExecDirect(SQL_HANDLE_ENV)",
			hstmt, SQL_HANDLE_STMT);

	std::cout << "Delete done." << std::endl;
};

void
Handler::table_drop(){

	log_msg = "<table_drop>";
	logger.log(log_msg, LOG_DEBUG);

	std::string selectstr = get_stmt_drop();
	std::cout << "selectstr: " << selectstr << std::endl;

	retcode = SQLExecDirect(hstmt, selectstr.c_str(), strlen(selectstr.c_str()));
	CHECK_ERROR2(retcode, "SQLExecDirect(SQL_HANDLE_ENV)",
			hstmt, SQL_HANDLE_STMT);

	std::cout << "Drop done." << std::endl;
};

void
Handler::analyze_columns(){

	amplcoltypes.resize(ncols(), 0);

	for (size_t j = 0; j < ncols(); j++){

		bool has_numeric = false;
		bool has_char = false;

		for (size_t i = 0; i < nrows(); i++){

			if (is_missing(i, j)){
				int debug = 0;
			}
			else if (is_numeric_val(i, j)){
				has_numeric = true;
			}
			else{
				has_char = true;
			}
		}

		if (has_numeric && has_char){
			amplcoltypes[j] = 2;
			std::cout << "Column has numerical and string type" << std::endl;
			throw DBE_Error;
		}
		else if (has_numeric){
			amplcoltypes[j] = 0;
		}
		else if (has_char){
			amplcoltypes[j] = 1;
		}
		else{
			// column only has empty values, type is undefined
			std::cout << "Could not deduce column type" << std::endl;
			throw DBE_Error;
		}
	}

	std::cout << "amplcoltypes: ";
	print_vector(amplcoltypes);
};

void
Handler::CHECK_ERROR2(
	SQLRETURN e,
	char *s,
	SQLHANDLE h,
	SQLSMALLINT t
){
	if (e != SQL_SUCCESS && e != SQL_SUCCESS_WITH_INFO){
		extract_error2(s, h, t);
	}
};

void
Handler::extract_error2(
	char *fn,
	SQLHANDLE handle,
	SQLSMALLINT type)
{
	//~ SQLINTEGER   i = 0;
	//~ SQLINTEGER   native;
	//~ SQLCHAR      state[ 7 ];
	//~ SQLCHAR      text[256];
	//~ SQLSMALLINT  len;
	//~ SQLRETURN    ret;

	//~ fprintf(stderr,
			//~ "\n"
			//~ "The driver reported the following diagnostics whilst running "
			//~ "%s\n\n",
			//~ fn);

	//~ do
	//~ {
		//~ ret = SQLGetDiagRec(type, handle, ++i, state, &native, text,
							//~ sizeof(text), &len );
		//~ if (SQL_SUCCEEDED(ret))
			//~ printf("%s:%ld:%ld:%s\n", state, i, native, text);
	//~ }
	//~ while( ret == SQL_SUCCESS );

    SQLINTEGER i = 0;
    SQLINTEGER NativeError;
    SQLCHAR SQLState[ 7 ];
    SQLCHAR MessageText[256];
    SQLSMALLINT TextLength;
    SQLRETURN ret;

    fprintf(stderr, "\nThe driver reported the following error %s\n", fn);
    do
    {
        ret = SQLGetDiagRec(type, handle, ++i, SQLState, &NativeError,
                            MessageText, sizeof(MessageText), &TextLength);
        if (SQL_SUCCEEDED(ret)) {
            printf("%s:%ld:%ld:%s\n",
                        SQLState, (long) i, (long) NativeError, MessageText);
        }
    }
    while( ret == SQL_SUCCESS );

	throw DBE_Error;
};







