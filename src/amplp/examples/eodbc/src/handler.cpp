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

	clean_vector(ColumnName);
	clean_vector(ColumnData);

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

	char buff[1000];

	//https://www.easysoft.com/developer/languages/c/examples/DescribeAndBindColumns.html

	// Allocate a statement handle
	SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
	check_error(retcode, (char*)"SQLAllocHandle(SQL_HANDLE_STMT)",
				hstmt, SQL_HANDLE_STMT);

	std::string sqlstr;
	if (sql.empty()){
		sqlstr = get_stmt_select();
	}
	else{
		sqlstr = sql;
	}

	log_msg = "SQL: ";
	log_msg += sqlstr;
	logger.log(log_msg, LOG_INFO);

	// Prepare Statement
	retcode = SQLPrepare (hstmt, (SQLCHAR*)sqlstr.c_str(), strlen(sqlstr.c_str()));
	check_error(retcode, (char*)"SQLPrepare(SQL_HANDLE_ENV)",
				hstmt, SQL_HANDLE_STMT);

	SQLSMALLINT numCols;

	// Retrieve number of columns
	retcode = SQLNumResultCols (hstmt, &numCols);
	check_error(retcode, (char*)"SQLNumResultCols()", hstmt, SQL_HANDLE_STMT);

	sprintf (buff, "Number of Result Columns %i", numCols);
	log_msg = buff;
	logger.log(log_msg, LOG_INFO);

	// vectors to describe and bind columns
	ColumnName.resize(numCols);
	std::vector<SQLSMALLINT>    ColumnNameLen(numCols);
	std::vector<SQLSMALLINT>    ColumnDataType(numCols);
	std::vector<SQLULEN>        ColumnDataSize(numCols);
	std::vector<SQLSMALLINT>    ColumnDataDigits(numCols);
	std::vector<SQLSMALLINT>    ColumnDataNullable(numCols);
	ColumnData.resize(numCols);
	std::vector<SQLLEN>         ColumnDataLen(numCols);

	std::vector<int> perm(numCols, -1);
	std::vector<bool> foundcol(ncols(), false);

	sprintf(buff, "Column\tColName\tColNameLen\tSQLDataType\tDataSize\tDecimalDigits\tNullable");
	log_msg = buff;
	logger.log(log_msg, LOG_DEBUG);

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

		check_error(retcode, (char*)"SQLDescribeCol()", hstmt, SQL_HANDLE_STMT);

		sprintf(buff, "%i\t'%s'\t%i\t%i\t%i\t%i\t%i",
					i+1, ColumnName[i], (int)ColumnNameLen[i],
					(int)ColumnDataType[i], (int)ColumnDataSize[i],
					(int)ColumnDataDigits[i], (int)ColumnDataNullable[i]);
		log_msg = buff;
		logger.log(log_msg, LOG_DEBUG);
	}

	for (int i=0;i<numCols;i++) {
		// Bind column, changing SQL data type to C data type
		// Numeric types are read into double, other types into char

		ColumnData[i] = (SQLCHAR *) malloc (ColumnDataSize[i]);

		SQLSMALLINT cdt = ColumnDataType[i];

		if (sql_num_types.find(cdt) != sql_num_types.end()){
			ColumnDataType[i]=SQL_C_DOUBLE;

			retcode = SQLBindCol (hstmt,                  // Statement handle
								  i+1,                    // Column number
								  ColumnDataType[i],      // C Data Type
								  ColumnData[i],          // Data buffer
								  ColumnDataSize[i],      // Size of Data Buffer
								  &ColumnDataLen[i]);     // Size of data returned
		}
		else{
			ColumnDataType[i] = SQL_C_CHAR;

			retcode = SQLBindCol (hstmt,                  // Statement handle
								  i+1,                    // Column number
								  ColumnDataType[i],      // C Data Type
								  ColumnData[i],          // Data buffer
								  ColumnDataSize[i] + 1,  // Size of Data Buffer, we add one for potentialy non null terminated strings
								  &ColumnDataLen[i]);     // Size of data returned
		}


		check_error(retcode, (char*)"SQLBindCol()", hstmt, SQL_HANDLE_STMT);

		for (size_t j = 0; j < ncols(); j++){
			std::string ampl_col = get_col_name(j);
			std::string odbc_col = (char*)ColumnName[i];
			if (ampl_col == odbc_col){
				perm[i] = j;
				foundcol[j] = true;
			}
		}
	}

	//~ std::cout << "perm: ";
	//~ print_vector(perm);
	//~ std::cout << "foundcol: ";
	//~ print_vector(foundcol);

	for (size_t j = 0; j < ncols(); j++){
		if (!foundcol[j]){
			log_msg = "Cannot find column: ";
			log_msg += get_col_name(j);
			logger.log(log_msg, LOG_ERROR);
			throw DBE_Error;
		}
	}

	log_msg = "perm: [";
	for (size_t i= 0; i<perm.size(); i++){
		log_msg += std::to_string(perm[i]);
		if (i < perm.size() - 1){
			log_msg += ", ";
		}
	}
	log_msg += "]";
	logger.log(log_msg, LOG_DEBUG);


	// Fetch records
	log_msg = "Starting read...";
	logger.log(log_msg, LOG_INFO);

	std::clock_t c_start = std::clock();

	retcode = SQLExecute (hstmt);
	if (retcode != SQL_NO_DATA){
		check_error(retcode, (char*)"SQLExecute()", hstmt, SQL_HANDLE_STMT);
	}

	for (int i=0; ; i++) {
		retcode = SQLFetch(hstmt);

		//No more data?
		if (retcode == SQL_NO_DATA) {
			break;
		}

		check_error(retcode, (char*)"SQLFetch()", hstmt, SQL_HANDLE_STMT);

		for (int j = 0; j < numCols; j++) {

			if (ColumnDataLen[j] == SQL_NULL_DATA){
				set_col_missing_val(perm[j]);
			}
			else{
				SQLSMALLINT cdt = ColumnDataType[j];
				if (cdt == SQL_C_DOUBLE) {
					set_col_val(*(double*)ColumnData[j], perm[j]);
				}
				else{
					set_col_val((char*)ColumnData[j], perm[j]);
				}
			}
		}
		add_row();
	}

	std::clock_t c_end = std::clock();
	double time_elapsed = 1000.0 * (c_end-c_start) / CLOCKS_PER_SEC;
	log_msg = "Read done in " + numeric_to_fixed(time_elapsed / 1000, 3) + "s";
	logger.log(log_msg, LOG_INFO);
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

	// get the data types supported by the database
	std::unordered_set<SQLSMALLINT> db_supported_types = get_db_supported_types();

	// one of the following data types must be supported by the Database
	// SQL_DOUBLE is the preferred, others might loose precision
	std::vector<SQLSMALLINT> prefnumtypes = {SQL_DOUBLE, SQL_FLOAT, SQL_REAL, SQL_NUMERIC};

	std::unordered_map<SQLSMALLINT, std::string> numtypesmap = {
		{SQL_DOUBLE, "DOUBLE"},
		{SQL_FLOAT, "FLOAT"},
		{SQL_REAL, "REAL"},
		{SQL_NUMERIC, "NUMERIC"}
	};

	// find a compatible data type in the available ones
	bool found = false;
	std::string tmp_str;

	for(size_t i = 0; i < prefnumtypes.size(); i++){

		SQLSMALLINT testtype = prefnumtypes[i];

		if (db_supported_types.find(testtype) != db_supported_types.end()){

			tmp_str = " " + numtypesmap[testtype];
			found = true;
			break;
		}
		else{
			log_msg = "DB does not support " + numtypesmap[testtype];
			logger.log(log_msg, LOG_INFO);
		}
	}

	if (!found){
		log_msg = "No suitable numeric type found to create table.";
		logger.log(log_msg, LOG_ERROR);
		throw DBE_Error;
	}

	// We assume VARCHAR is supported

	std::string stmt = "CREATE TABLE ";
	stmt += table_name;
	stmt += " (";

	for (size_t i = 0; i < ncols(); i++){
		stmt += get_col_name(i);

		if (amplcoltypes[i] == 1){
			stmt += " VARCHAR(255)";
		}
		else{
			stmt += tmp_str;
		}

		//~ if (i + 1 < ncols()){
			stmt += ", ";
		//~ }
	}

	stmt += "PRIMARY KEY (";
	for (size_t i = 0; i < nkeycols(); i++){
		stmt += get_col_name(i);
		if (i + 1 < nkeycols()){
			stmt += ", ";
		}
	}
	stmt += ")";
	stmt += ");";

	return stmt;
};

void
Handler::write_out(){

	log_msg = "<write_out>";
	logger.log(log_msg, LOG_DEBUG);

	if (nrows() == 0){
		log_msg = "No rows to write";
		logger.log(log_msg, LOG_INFO);
		return;
	}

	char buff[1000];

	// Get the insert statement
	std::string sqlstr = get_stmt_insert();
	log_msg = "SQL: ";
	log_msg += sqlstr;
	logger.log(log_msg, LOG_INFO);

	// Prepare Statement
	retcode = SQLPrepare (hstmt, (SQLCHAR*)sqlstr.c_str(), strlen(sqlstr.c_str()));
	check_error(retcode, (char*)"SQLPrepare(SQL_HANDLE_ENV)",
				hstmt, SQL_HANDLE_STMT);

	SQLSMALLINT NumParams;

	// Retrieve number of parameters
	retcode = SQLNumParams(hstmt, &NumParams);
	check_error(retcode, (char*)"SQLNumParams()", hstmt,
				SQL_HANDLE_STMT);

	sprintf(buff, "Number of Result Parameters %i", NumParams);
	log_msg = buff;
	logger.log(log_msg, LOG_INFO);

	// vectors to describe and bind parameters
	ColumnName.resize(ncols());
	std::vector<SQLSMALLINT>    ColumnNameLen(ncols());
	std::vector<SQLSMALLINT>    ColumnDataType(ncols());
	std::vector<SQLULEN>        ColumnDataSize(ncols());
	std::vector<SQLSMALLINT>    ColumnDataDigits(ncols());
	std::vector<SQLSMALLINT>    ColumnDataNullable(ncols());
	ColumnData.resize(ncols());
	//~ std::vector<double>         ColumnDataDouble(ncols());
	std::vector<SQLLEN>         ColumnDataLen(ncols());

	std::vector<SQLSMALLINT>    DataType(ncols());
	std::vector<SQLULEN>        bytesRemaining(ncols());
	std::vector<SQLSMALLINT>    DecimalDigits(ncols());
	std::vector<SQLSMALLINT>    Nullable(ncols());

	std::vector<double> DoubleData(ncols());
	std::vector<int> IntData(ncols());
	std::vector<SQL_DATE_STRUCT> DateData(ncols());
	std::vector<SQL_TIME_STRUCT> TimeData(ncols());
	std::vector<SQL_TIMESTAMP_STRUCT> TimeStampData(ncols());
	std::vector<SQLLEN> LenOrIndPtr(ncols());


	// bind parameters
	for (int i=0; i<NumParams; i++){
		ColumnData[i] = (SQLCHAR *) malloc (MAX_COL_NAME_LEN);

		int ctype = -1;
		int otype = -1;

		if (amplcoltypes[i] == 0){
			retcode = SQLBindParameter(hstmt, i+1, SQL_PARAM_INPUT, SQL_C_DOUBLE,
										SQL_DOUBLE, 0, 0, &DoubleData[i], 0, &LenOrIndPtr[i]);
			ctype = SQL_C_DOUBLE;
			otype = SQL_DOUBLE;
		}
		else if (amplcoltypes[i] == 1){

			retcode = SQLBindParameter(hstmt, i+1, SQL_PARAM_INPUT, SQL_C_CHAR,
										SQL_VARCHAR, MAX_COL_NAME_LEN, 0, ColumnData[i], 0, &LenOrIndPtr[i]);
			ctype = SQL_C_CHAR;
			otype = SQL_VARCHAR;
		}
		else{
			log_msg = "Cannot Bind mixed parameter";
			logger.log(log_msg, LOG_ERROR);
			throw DBE_Error;
		}
		check_error(retcode, (char*)"SQLBindParameter()", hstmt, SQL_HANDLE_STMT);

		log_msg = "Binding parameter ";
		log_msg += std::to_string(i+1);
		log_msg += " '";
		log_msg += get_col_name(i);
		log_msg += "' from C type ";
		log_msg += std::to_string(ctype);
		log_msg += " to ODBC type ";
		log_msg += std::to_string(otype);
		logger.log(log_msg, LOG_DEBUG);
	}

	log_msg = "Starting write...";
	logger.log(log_msg, LOG_INFO);

	std::clock_t c_start = std::clock();

	for (size_t i=0; i<nrows(); i++){
		for (size_t j=0; j<ncols(); j++){

			if (is_missing(i, j)){
				LenOrIndPtr[j] = SQL_NULL_DATA;
			}
			else{
				if (amplcoltypes[j] == 0){

						LenOrIndPtr[j] = NULL;
					//~ if (t_num_types[j] == SQL_INTEGER || t_num_types[j] == SQL_SMALLINT){
						//~ IntData[j] = get_numeric_val(i, j);
					//~ }
					//~ else{
						DoubleData[j] = get_numeric_val(i, j);
					//~ }
				}
				else if (amplcoltypes[j] == 1){
					LenOrIndPtr[j] = SQL_NTS;
					strcpy((char*)ColumnData[j], get_char_val(i, j));
				}
			}
		}
		retcode = SQLExecute(hstmt);
		if (retcode != SQL_NO_DATA){
			check_error(retcode, (char*)"SQLExecute()", hstmt,
						SQL_HANDLE_STMT);
		}
	}
	if (!autocommit){
		retcode = SQLEndTran(SQL_HANDLE_ENV, henv, SQL_COMMIT);
		check_error(retcode, "SQLEndTran()", hstmt,
					SQL_HANDLE_STMT);
	}

	std::clock_t c_end = std::clock();
	double time_elapsed = 1000.0 * (c_end-c_start) / CLOCKS_PER_SEC;
	log_msg = "Write done in " + numeric_to_fixed(time_elapsed / 1000, 3) + "s";
	logger.log(log_msg, LOG_INFO);
};

void
Handler::write_inout(){

	// We should simply pass data in AMPL's native types (double and char) and let the driver do the conversion.
	// Unfortunately mysql is not accepting doubles in the update statement and returns the error
	// The driver reported the following error SQLExecute()
	// HY000:1:1292:[MySQL][ODBC 8.0(a) Driver][mysqld-8.0.26]Truncated incorrect INTEGER value: '1.00000000000000000e+00'
	// even though it accepts doubles in the insert.
	// As a workaround we check the types of each column with get_table_types and convert the data that will go
	// into SQL_INTEGER and SQL_SMALLINT columns to integer before passing the data.

	log_msg = "<write_inout>";
	logger.log(log_msg, LOG_DEBUG);

	if (nrows() == 0){
		log_msg = "No rows to update";
		logger.log(log_msg, LOG_INFO);
		return;
	}

	char buff[1000];

	std::unordered_map<std::string, int> table_types = get_table_types();
	get_ampl_col_types();

	std::string sqlstr;
	if (sql.empty()){
		sqlstr = get_stmt_update();
	}
	else{
		sqlstr = sql;
	}

	log_msg = "SQL: ";
	log_msg += sqlstr;
	logger.log(log_msg, LOG_INFO);

	std::vector<std::string> sql_colnames = get_sql_colnames(sqlstr);

	//~ print_vector(sql_colnames);

	// Prepare Statement
	retcode = SQLPrepare (hstmt, (SQLCHAR*)sqlstr.c_str(), SQL_NTS);
	check_error(retcode, (char*)"SQLPrepare(SQL_HANDLE_ENV)",
				hstmt, SQL_HANDLE_STMT);

	SQLSMALLINT NumParams;

	// Retrieve number of parameters
	retcode = SQLNumParams(hstmt, &NumParams);
	check_error(retcode, (char*)"SQLNumParams()", hstmt,
				SQL_HANDLE_STMT);

	sprintf(buff, "Number of Result Parameters %i", (int)NumParams);
	log_msg = buff;
	logger.log(log_msg, LOG_INFO);

	if ((int)NumParams != (int)sql_colnames.size()){
	}

	// Permutation of columns derived from the update statement
	std::vector<int> perm((int)NumParams, -1);

	for (size_t i=0; i<(int)NumParams; i++){
		for (size_t j=0; j<ncols(); j++){
			if (sql_colnames[i] == get_col_name(j)){
				perm[i] = j;
			}
		}
	}

	//~ for (size_t i=0; i<ndatacols(); i++){
		//~ perm[i] = i + nkeycols();
	//~ }
	//~ for (size_t i=0; i<nkeycols(); i++){
		//~ perm[i + ndatacols()] = i;
	//~ }

	log_msg = "perm: [";
	for (size_t i= 0; i<perm.size(); i++){
		log_msg += std::to_string(perm[i]);
		if (i < perm.size() - 1){
			log_msg += ", ";
		}
	}
	log_msg += "]";
	logger.log(log_msg, LOG_DEBUG);

	// Get the odbc types of the columns in the update statement
	std::vector<int> odbc_types((int)NumParams, -1);

	for (size_t i= 0; i<perm.size(); i++){
		//~ int col = perm[i];
		int col = perm[i];
		std::string colname = get_col_name(col);

		if (table_types.find(colname) != table_types.end()){
			odbc_types[i] = table_types[colname];
		}
		else{
			log_msg += "Cannot find column type";
			logger.log(log_msg, LOG_ERROR);
			throw DBE_Error;
		}
	}

	// Get the odbc types of the columns in the update statement
	std::vector<int> ampl_odbc_types((int)NumParams, -1);

	for (size_t i= 0; i<ncols(); i++){
		int col = i;
		std::string colname = get_col_name(col);

		if (table_types.find(colname) != table_types.end()){
			ampl_odbc_types[i] = table_types[colname];
		}
		else{
			log_msg += "Cannot find column type";
			logger.log(log_msg, LOG_ERROR);
			throw DBE_Error;
		}
	}

	log_msg = "odbc_types: [";
	for (size_t i= 0; i<odbc_types.size(); i++){
		log_msg += std::to_string(odbc_types[i]);
		if (i < odbc_types.size() - 1){
			log_msg += ", ";
		}
	}
	log_msg += "]";
	logger.log(log_msg, LOG_DEBUG);

	// vectors to pass data to odbc
	ColumnData.resize(ncols());
	std::vector<double> DoubleData(ncols());
	std::vector<int> IntData(ncols());
	//~ std::vector<SQLLEN> PartIDInd(ncols(), 0);
	std::vector<SQLLEN> LenOrIndPtr(ncols());

	for (size_t i=0; i<ncols(); i++){
		ColumnData[i] = (SQLCHAR *) malloc (MAX_COL_NAME_LEN);
	}

	// bind parameters
	for (size_t i=0; i<(int)NumParams; i++){

		int amplcol = perm[i];

		if (amplcol == -1){
			log_msg = "Cannot bind unknown column '";
			log_msg += sql_colnames[i];
			log_msg += "'.";
			logger.log(log_msg, LOG_ERROR);
			throw DBE_Error;
		}

		if (amplcoltypes[amplcol] == 0){

			if (ampl_odbc_types[amplcol] == SQL_INTEGER || ampl_odbc_types[amplcol] == SQL_SMALLINT){
				retcode = SQLBindParameter(hstmt, i+1, SQL_PARAM_INPUT, SQL_C_LONG,
											SQL_INTEGER, 0, 0, &IntData[amplcol], 0, &LenOrIndPtr[amplcol]);

				log_msg = get_SQLBindParameter_string(i+1, amplcol, odbc_types[i], SQL_C_LONG, SQL_INTEGER);
			}
			else {
				retcode = SQLBindParameter(hstmt, i+1, SQL_PARAM_INPUT, SQL_C_DOUBLE,
											SQL_DOUBLE, 0, 0, &DoubleData[amplcol], 0, &LenOrIndPtr[amplcol]);

				log_msg = get_SQLBindParameter_string(i+1, amplcol, odbc_types[i], SQL_C_DOUBLE, SQL_DOUBLE);
			}
		}
		else if (amplcoltypes[amplcol] == 1){

			retcode = SQLBindParameter(hstmt, i+1, SQL_PARAM_INPUT, SQL_C_CHAR,
									SQL_VARCHAR, MAX_COL_NAME_LEN, 0, ColumnData[amplcol], 0, &LenOrIndPtr[amplcol]);

			log_msg = get_SQLBindParameter_string(i+1, amplcol, odbc_types[i], SQL_C_CHAR, SQL_VARCHAR);
		}
		else{
			std::cout << "Cannot Bind mixed parameter" << std::endl;
			throw DBE_Error;
		}
		logger.log(log_msg, LOG_DEBUG);
		check_error(retcode, (char*)"SQLBindParameter()", hstmt, SQL_HANDLE_STMT);
	}


	log_msg = "Starting update...";
	logger.log(log_msg, LOG_INFO);

	std::clock_t c_start = std::clock();

	for (size_t i=0; i<nrows(); i++){

		//~ for (size_t j=0; j<ncols(); j++){

			//~ IntData[j] = -10;
			//~ DoubleData[j] = -10;
		//~ }

		for (size_t j=0; j<ncols(); j++){

			if (is_missing(i, j)){
				LenOrIndPtr[j] = SQL_NULL_DATA;
			}
			else{

				if (amplcoltypes[j] == 0){

					LenOrIndPtr[j] = NULL;
					if (ampl_odbc_types[j] == SQL_INTEGER || ampl_odbc_types[j] == SQL_SMALLINT){
						IntData[j] = get_numeric_val(i, j);
					}
					else{
						DoubleData[j] = get_numeric_val(i, j);
					}
				}
				else if (amplcoltypes[j] == 1){
					LenOrIndPtr[j] = SQL_NTS;
					//~ LenOrIndPtr[j] = NULL;
					strcpy((char*)ColumnData[j], get_char_val(i, j));
				}
			}
		}

		//~ std::cout << "double" << std::endl;
		//~ print_vector(DoubleData);
		//~ std::cout << "int" << std::endl;
		//~ print_vector(IntData);
		//~ print_vector(ColumnData);
		//~ print_vector(LenOrIndPtr);

		retcode = SQLExecute(hstmt);
		if (retcode != SQL_NO_DATA){
			check_error(retcode, (char*)"SQLExecute()", hstmt,
						SQL_HANDLE_STMT);
		}
	}

	if (!autocommit){
		retcode = SQLEndTran(SQL_HANDLE_ENV, henv, SQL_COMMIT);

		check_error(retcode, "SQLEndTran()", hstmt,
					SQL_HANDLE_STMT);
	}

	std::clock_t c_end = std::clock();
	double time_elapsed = 1000.0 * (c_end-c_start) / CLOCKS_PER_SEC;
	log_msg = "Update done in " + numeric_to_fixed(time_elapsed / 1000, 3) + "s";
	logger.log(log_msg, LOG_INFO);
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

	allowed_kargs = {"autocommit", "write", "DRIVER", "SQL", "DSN"};
};


void
Handler::validate_arguments(){

	log_msg = "<validate_arguments>";
	logger.log(log_msg, LOG_DEBUG);

	for(const auto it: user_kargs){

		std::string key = it.first;

		//~ if (key == "autocommit"){
		if (compare_strings_lower(key, "autocommit")){
			autocommit = get_bool_karg(key);
		}
		//~ else if (key == "write"){
		else if (compare_strings_lower(key, "write")){
			std::string val = it.second;

			if (inout != "OUT"){
				log_msg = "Option 'write' applies only to OUT table declarations. Ignoring option '";
				log_msg += key;
				log_msg += "=";
				log_msg += val;
				log_msg += "'.";
				logger.log(log_msg, LOG_WARNING);
			}
			else if (compare_strings_lower(val, "append")){
				log_msg = "Option '";
				log_msg += key;
				log_msg += "' set to '";
				log_msg += val;
				log_msg += "'.";
				logger.log(log_msg, LOG_INFO);
				write = "APPEND";
			}
			else if (compare_strings_lower(val, "delete")){
				log_msg = "Option '";
				log_msg += key;
				log_msg += "' set to '";
				log_msg += val;
				log_msg += "'.";
				logger.log(log_msg, LOG_INFO);
				write = "DELETE";
			}
			else if (compare_strings_lower(val, "drop")){
				log_msg = "Option '";
				log_msg += key;
				log_msg += "' set to '";
				log_msg += val;
				log_msg += "'.";
				logger.log(log_msg, LOG_INFO);
				write = "DROP";
			}
			else{
				log_msg = "Invalid argument '";
				log_msg += val;
				log_msg += "' for argument '";
				log_msg += key;
				log_msg += "'.";
				logger.log(log_msg, LOG_ERROR);
				throw DBE_Error;
			}
		}
		//~ else if (key == "DRIVER"){
		else if (compare_strings_lower(key, "DRIVER")){
			driver = "Driver=" + it.second;
		}
		//~ else if (key == "DSN"){
		else if (compare_strings_lower(key, "DSN")){
			driver = "DSN=" + it.second;
		}
		else if (compare_strings_lower(key, "SQL")){
			sql = it.second;
		}
	}

	//~ std::string tempstr;
	//~ for (size_t i=0; i<ampl_args.size(); i++){

		//~ std::string arg = ampl_args[i];

		//~ tempstr = "DRIVER=";
		//~ if (!arg.compare(0, tempstr.size(), tempstr)){
			//~ driver = arg;
		//~ }

		//~ tempstr = "SQL=";
		//~ if (!arg.compare(0, tempstr.size(), tempstr)){
			//~ sql = arg.substr(tempstr.size());
			//~ if (is_writer){
				//~ log_msg = "SQL declaration only accepted when reading data. Ignoring: " + arg;
				//~ logger.log(log_msg, LOG_WARNING);
			//~ }
		//~ }

		//~ tempstr = "DSN=";
		//~ if (!arg.compare(0, tempstr.size(), tempstr)){
			//~ dsn = arg;
		//~ }
	//~ }

	//~ if (inout != "IN"){
	if (is_writer){
		get_ampl_col_types();
	}

	alloc_and_connect();

	bool exists = table_exists();

	if (!exists){
		if (is_writer){
			table_create();
			inout = "OUT";
		}
		else{
			log_msg = "Could not find table " + table_name;
			logger.log(log_msg, LOG_ERROR);
			throw DBE_Error;
		}
	}
	else{
		if (inout == "OUT"){
			if (write == "DELETE"){
				table_delete();
			}
			else if (write == "DROP"){
				table_drop();
				table_create();
			}
		}
	}
};


void
Handler::alloc_and_connect(){

	log_msg = "<alloc_and_connect>";
	logger.log(log_msg, LOG_DEBUG);

	// Allocate environment
	retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
	check_error(retcode, (char*)"SQLAllocHandle(SQL_HANDLE_ENV)",
				henv, SQL_HANDLE_ENV);

	// Set ODBC Verion
	retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION,
										(SQLPOINTER*)SQL_OV_ODBC3, 0);
	check_error(retcode, (char*)"SQLSetEnvAttr(SQL_ATTR_ODBC_VERSION)",
				henv, SQL_HANDLE_ENV);

	// Allocate Connection
	retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
	check_error(retcode, (char*)"SQLAllocHandle(SQL_HANDLE_DBC)",
				henv, SQL_HANDLE_DBC);

	// Set Login Timeout
	retcode = SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);
	check_error(retcode, (char*)"SQLSetConnectAttr(SQL_LOGIN_TIMEOUT)",
				hdbc, SQL_HANDLE_DBC);

	// Set Auto Commit
	if (!autocommit){
		retcode = SQLSetConnectAttr(hdbc, SQL_ATTR_AUTOCOMMIT, 0, 0);
		check_error(retcode, (char*)"SQLSetConnectAttr(SQL_ATTR_AUTOCOMMIT)",
															hdbc, SQL_HANDLE_DBC);
	}

	std::string connstr;
	if (!driver.empty()){connstr = driver;}
	else if (!dsn.empty()){connstr = dsn;}
	else{
		log_msg = "No connection string or DSN specified.";
		logger.log(log_msg, LOG_ERROR);
		throw DBE_Error;
	}

	log_msg = "connection string: ";
	log_msg += connstr;
	logger.log(log_msg, LOG_INFO);

	retcode = SQLDriverConnect(hdbc, NULL, (SQLCHAR*)connstr.c_str(), SQL_NTS,
					 NULL, 0, NULL, SQL_DRIVER_COMPLETE);
	check_error(retcode, (char*)"SQLDriverConnect(DATASOURCE)",
                hdbc, SQL_HANDLE_DBC);

	// Allocate a statement handle
	SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
	check_error(retcode, (char*)"SQLAllocHandle(SQL_HANDLE_STMT)",
				hstmt, SQL_HANDLE_STMT);






	//~ // Allocate a statement handle
	//~ SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
	//~ check_error(retcode, "SQLAllocHandle(SQL_HANDLE_STMT)",
				//~ hstmt, SQL_HANDLE_STMT);




	//~ check_error(retcode, "SQLConnect(DSN:DATASOURCE;)",
				//~ hdbc, SQL_HANDLE_DBC);
	//~ retcode = SQLConnect(hdbc, connstr.c_str(), SQL_NTS,
							   //~ (SQLCHAR*) NULL, 0, NULL, 0);

	//~ std::cout << "SQLConnect:" << retcode << std::endl;

	//~ std::string tmp = "SQLDriverConnect(";
	//~ tmp += "SQLDriverConnect(

	//~ check_error(retcode, "SQLDriverConnect(SQL_ATTR_AUTOCOMMIT)",
				//~ hdbc, SQL_HANDLE_DBC);


	//~ // Connect to DSN
	//~ retcode = SQLConnect(hdbc, (SQLCHAR*) "DATASOURCE", SQL_NTS,
							   //~ (SQLCHAR*) NULL, 0, NULL, 0);
	//~ check_error(retcode, "SQLConnect(DSN:DATASOURCE;)",
				//~ hdbc, SQL_HANDLE_DBC);
};

bool
Handler::table_exists(){

	log_msg = "<table_exists>";
	logger.log(log_msg, LOG_DEBUG);

	SQLSMALLINT nrcols; // number of result columns
	bool exists = false;

	retcode = SQLTables(hstmt, NULL, 0, NULL, 0, (SQLCHAR*)table_name.c_str(), SQL_NTS, NULL, 0);
	check_error(retcode, "SQLTables()", hstmt, SQL_HANDLE_STMT);
	retcode = SQLNumResultCols(hstmt, &nrcols);
	check_error(retcode, "SQLNumResultCols()", hstmt, SQL_HANDLE_STMT);

	if (nrcols != 5){
		log_msg = "Invalid result from SQLTables ";
		log_msg += std::to_string(nrcols);
		logger.log(log_msg, LOG_ERROR);
		throw DBE_Error;
	}

	while ((retcode = SQLFetch(hstmt)) == SQL_SUCCESS) {
		SQLUSMALLINT i;

		// Loop through the columns
		//~ for (i = 1; i <= nrcols; i++) {
			SQLLEN  indicator;
			SQLCHAR buf[255];
			// Retrieve column data as a string
			// Table name is at position 3
			retcode = SQLGetData(hstmt, 3, SQL_C_CHAR, buf, sizeof(buf), &indicator);

			if (retcode != SQL_SUCCESS){
				//~ break;
			}

			std::string tmp_str = (char*)buf;

			if (tmp_str == table_name){
				exists = true;
				//~ break;
			}
		//~ }
	}

	if (exists){
		log_msg = "Table ";
		log_msg += table_name;
		log_msg += " found.";
		logger.log(log_msg, LOG_INFO);
	}
	else{
		log_msg = "Cannot find table ";
		log_msg += table_name;
		logger.log(log_msg, LOG_INFO);
	}

	retcode = SQLFreeStmt(hstmt, SQL_CLOSE);
	check_error(retcode, (char*)"SQLFreeStmt()", hstmt, SQL_HANDLE_STMT);

	return exists;
};

void
Handler::table_create(){

	log_msg = "<table_create>";
	logger.log(log_msg, LOG_DEBUG);

	std::string sqlstr = get_stmt_create();
	log_msg = "SQL: " + sqlstr;
	logger.log(log_msg, LOG_INFO);

	retcode = SQLExecDirect(hstmt, (SQLCHAR*)sqlstr.c_str(), SQL_NTS);
	if (retcode != SQL_NO_DATA){
		check_error(retcode, (char*)"SQLExecDirect(SQL_HANDLE_ENV)",
				hstmt, SQL_HANDLE_STMT);
	}
	retcode = SQLFreeStmt(hstmt, SQL_CLOSE);
	check_error(retcode, (char*)"SQLFreeStmt()", hstmt, SQL_HANDLE_STMT);
};

void
Handler::table_delete(){

	log_msg = "<table_delete>";
	logger.log(log_msg, LOG_DEBUG);

	std::string sqlstr = get_stmt_delete();
	log_msg = "SQL: " + sqlstr;
	logger.log(log_msg, LOG_INFO);

	retcode = SQLExecDirect(hstmt, (SQLCHAR*)sqlstr.c_str(), SQL_NTS);
	if (retcode != SQL_NO_DATA){
		check_error(retcode, (char*)"SQLExecDirect(SQL_HANDLE_ENV)",
				hstmt, SQL_HANDLE_STMT);
	}
	retcode = SQLFreeStmt(hstmt, SQL_CLOSE);
	check_error(retcode, (char*)"SQLFreeStmt()", hstmt, SQL_HANDLE_STMT);
};

void
Handler::table_drop(){

	log_msg = "<table_drop>";
	logger.log(log_msg, LOG_DEBUG);

	std::string sqlstr = get_stmt_drop();
	log_msg = "SQL: " + sqlstr;
	logger.log(log_msg, LOG_INFO);

	retcode = SQLExecDirect(hstmt, (SQLCHAR*)sqlstr.c_str(), SQL_NTS);
	if (retcode != SQL_NO_DATA){
		check_error(retcode, (char*)"SQLExecDirect(SQL_HANDLE_ENV)",
				hstmt, SQL_HANDLE_STMT);
	}
	retcode = SQLFreeStmt(hstmt, SQL_CLOSE);
	check_error(retcode, (char*)"SQLFreeStmt()", hstmt, SQL_HANDLE_STMT);
};

void
Handler::get_ampl_col_types(){

	log_msg = "<get_ampl_col_types>";
	logger.log(log_msg, LOG_DEBUG);

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
			log_msg = "Column '";
			log_msg += get_col_name(j);
			log_msg += "' has numerical and string types";
			logger.log(log_msg, LOG_ERROR);
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
			log_msg = "Could not deduce column type of column '";
			log_msg += get_col_name(j);
			log_msg += "' (column is empty)";
			logger.log(log_msg, LOG_ERROR);
			throw DBE_Error;
		}
	}

	log_msg = "amplcoltypes: [";
	for (size_t i= 0; i<amplcoltypes.size(); i++){
		log_msg += std::to_string(amplcoltypes[i]);
		if (i < amplcoltypes.size() - 1){
			log_msg += ", ";
		}
	}
	log_msg += "]";
	logger.log(log_msg, LOG_DEBUG);
};

void
Handler::check_error(
	SQLRETURN e,
	const char *s,
	SQLHANDLE h,
	SQLSMALLINT t
){
	if (e != SQL_SUCCESS && e != SQL_SUCCESS_WITH_INFO){
		extract_error(s, h, t);
	}
};

void
Handler::extract_error(
	const char *fn,
	SQLHANDLE handle,
	SQLSMALLINT type)
{
	SQLINTEGER i = 0;
	SQLINTEGER NativeError;
	SQLCHAR SQLState[7];
	SQLCHAR MessageText[256];
	SQLSMALLINT TextLength;
	SQLRETURN ret;

	char buff[1000];
	log_msg = "";
	sprintf(buff, "The driver reported the following error %s\n", fn);
	log_msg += buff;
	do
	{
		ret = SQLGetDiagRec(type, handle, ++i, SQLState, &NativeError,
							MessageText, sizeof(MessageText), &TextLength);
		if (SQL_SUCCEEDED(ret)) {
			sprintf(buff, "%s:%ld:%ld:%s\n",
					SQLState, (long) i, (long) NativeError, MessageText);
			log_msg += buff;
		}
    }
    while( ret == SQL_SUCCESS );

	logger.log(log_msg, LOG_ERROR);
	throw DBE_Error;
};


void
Handler::get_odbc_col_types(){

	//https://docs.microsoft.com/en-us/sql/odbc/reference/syntax/sqlcolumns-function?view=sql-server-ver15

	log_msg = "<get_odbc_col_types>";
	logger.log(log_msg, LOG_DEBUG);

	int STR_LEN = 255;  
	int REM_LEN = 255;

	SQLCHAR szSchema[255];
	SQLCHAR szCatalog[255];
	SQLCHAR szColumnName[255];
	SQLCHAR szTableName[255];
	SQLCHAR szTypeName[255];
	SQLCHAR szRemarks[255];
	SQLCHAR szColumnDefault[255];
	SQLCHAR szIsNullable[255];
	  
	SQLINTEGER ColumnSize;  
	SQLINTEGER BufferLength;  
	SQLINTEGER CharOctetLength;  
	SQLINTEGER OrdinalPosition;  
	  
	SQLSMALLINT DataType;  
	SQLSMALLINT DecimalDigits;  
	SQLSMALLINT NumPrecRadix;  
	SQLSMALLINT Nullable;  
	SQLSMALLINT SQLDataType;  
	SQLSMALLINT DatetimeSubtypeCode;  
	  
	// Declare buffers for bytes available to return  
	SQLLEN cbCatalog;  
	SQLLEN cbSchema;  
	SQLLEN cbTableName;  
	SQLLEN cbColumnName;  
	SQLLEN cbDataType;  
	SQLLEN cbTypeName;  
	SQLLEN cbColumnSize;  
	SQLLEN cbBufferLength;  
	SQLLEN cbDecimalDigits;  
	SQLLEN cbNumPrecRadix;  
	SQLLEN cbNullable;  
	SQLLEN cbRemarks;  
	SQLLEN cbColumnDefault;  
	SQLLEN cbSQLDataType;  
	SQLLEN cbDatetimeSubtypeCode;  
	SQLLEN cbCharOctetLength;  
	SQLLEN cbOrdinalPosition;  
	SQLLEN cbIsNullable;  

	retcode = SQLColumns(hstmt, NULL, 0, NULL, 0, (SQLCHAR*)table_name.c_str(), SQL_NTS, NULL, 0); 
	check_error(retcode, (char*)"SQLColumns(SQL_HANDLE_STMT)", hstmt, SQL_HANDLE_STMT);

	SQLBindCol(hstmt, 1, SQL_C_CHAR, szCatalog, STR_LEN,&cbCatalog);  
	SQLBindCol(hstmt, 2, SQL_C_CHAR, szSchema, STR_LEN, &cbSchema);  
	SQLBindCol(hstmt, 3, SQL_C_CHAR, szTableName, STR_LEN,&cbTableName);  
	SQLBindCol(hstmt, 4, SQL_C_CHAR, szColumnName, STR_LEN, &cbColumnName);  
	SQLBindCol(hstmt, 5, SQL_C_SSHORT, &DataType, 0, &cbDataType);  
	SQLBindCol(hstmt, 6, SQL_C_CHAR, szTypeName, STR_LEN, &cbTypeName);  
	SQLBindCol(hstmt, 7, SQL_C_SLONG, &ColumnSize, 0, &cbColumnSize);  
	SQLBindCol(hstmt, 8, SQL_C_SLONG, &BufferLength, 0, &cbBufferLength);  
	SQLBindCol(hstmt, 9, SQL_C_SSHORT, &DecimalDigits, 0, &cbDecimalDigits);  
	SQLBindCol(hstmt, 10, SQL_C_SSHORT, &NumPrecRadix, 0, &cbNumPrecRadix);  
	SQLBindCol(hstmt, 11, SQL_C_SSHORT, &Nullable, 0, &cbNullable);
	SQLBindCol(hstmt, 12, SQL_C_CHAR, szRemarks, REM_LEN, &cbRemarks);  
	SQLBindCol(hstmt, 13, SQL_C_CHAR, szColumnDefault, STR_LEN, &cbColumnDefault);  
	SQLBindCol(hstmt, 14, SQL_C_SSHORT, &SQLDataType, 0, &cbSQLDataType);  
	SQLBindCol(hstmt, 15, SQL_C_SSHORT, &DatetimeSubtypeCode, 0, &cbDatetimeSubtypeCode);  
	SQLBindCol(hstmt, 16, SQL_C_SLONG, &CharOctetLength, 0, &cbCharOctetLength);  
	SQLBindCol(hstmt, 17, SQL_C_SLONG, &OrdinalPosition, 0, &cbOrdinalPosition);  
	SQLBindCol(hstmt, 18, SQL_C_CHAR, szIsNullable, STR_LEN, &cbIsNullable);  

	int pos = 0;
	while (SQL_SUCCESS == retcode){
		retcode = SQLFetch(hstmt);

		//No more data?
		if (retcode == SQL_NO_DATA) {
			break;
		}

		std::cout << (char*)szCatalog << ", ";
		std::cout << (char*)szSchema << ", ";
		std::cout << (char*)szTableName << ", ";
		std::cout << (char*)szColumnName << ", ";
		std::cout << DataType << ", ";
		std::cout << (char*)szTypeName << ", ";
		std::cout << ColumnSize << ", ";
		std::cout << BufferLength << ", ";
		std::cout << DecimalDigits << ", ";
		std::cout << NumPrecRadix << ", ";
		std::cout << Nullable << ", ";
		std::cout << (char*)szRemarks << ", ";
		std::cout << (char*)szColumnDefault << ", ";
		std::cout << SQLDataType << ", ";
		std::cout << DatetimeSubtypeCode << ", ";
		std::cout << CharOctetLength << ", ";
		std::cout << OrdinalPosition << ", ";
		std::cout << (char*)szIsNullable << ", ";
		std::cout << std::endl;

		t_num_types[pos] = DataType;
		t_str_types[pos] = (char*)szTypeName;
		pos += 1;
	}

	log_msg = "\t\tncol\t\tstype\t\tntype";
	logger.log(log_msg, LOG_DEBUG);

	for (int i=0;i<pos;i++){
		log_msg = "\t\t";
		log_msg += std::to_string(i);
		log_msg += "\t\t";
		log_msg += t_str_types[i];
		log_msg += "\t\t";
		log_msg += std::to_string(t_num_types[i]);
		logger.log(log_msg, LOG_DEBUG);
	}

	retcode = SQLFreeStmt(hstmt, SQL_CLOSE);
	check_error(retcode, (char*)"SQLFreeStmt()", hstmt, SQL_HANDLE_STMT);
};


std::unordered_set<SQLSMALLINT>
Handler::get_db_supported_types(){

	std::unordered_set<SQLSMALLINT> db_supported_types;

	SQLCHAR typeName[128];
	SQLSMALLINT dataType;
	SQLINTEGER columnSize;

	SQLLEN typeName_ind, dataType_ind, columnSize_ind;

	retcode = SQLGetTypeInfo(hstmt, SQL_ALL_TYPES);
	check_error(retcode, (char*)"SQLGetTypeInfo", hstmt, SQL_HANDLE_STMT);

	retcode = SQLBindCol(hstmt, 1, SQL_C_CHAR,
						(SQLPOINTER) typeName,
						(SQLLEN) sizeof(typeName), &typeName_ind);
	check_error(retcode, (char*)"SQLBindCol(1)", hstmt, SQL_HANDLE_STMT);

	retcode = SQLBindCol(hstmt, 2, SQL_C_SHORT,
						(SQLPOINTER) &dataType,
						(SQLLEN) sizeof(dataType), &dataType_ind);
	check_error(retcode, (char*)"SQLBindCol(2)", hstmt, SQL_HANDLE_STMT);

	retcode = SQLBindCol(hstmt, 3, SQL_C_ULONG,
						(SQLPOINTER) &columnSize,
						(SQLLEN) sizeof(columnSize), &columnSize_ind);
	check_error(retcode, (char*)"SQLBindCol(2)", hstmt, SQL_HANDLE_STMT);

	//~ printf("SQL Data Type             Type Name                 Value"
																//~ "Max Size\n");
	//~ printf("------------------------- ------------------------- -------"
																//~ "--------\n");

	//~ std::cout << "DB supported data types" << std::endl;

	// Fetch each row, and display
	while ((retcode = SQLFetch(hstmt)) == SQL_SUCCESS) {

		//~ std::cout << typeName;
		//~ std::cout << ", ";
		//~ std::cout << dataType;
		//~ std::cout << ", ";
		//~ std::cout << columnSize;
		//~ std::cout << std::endl;

		db_supported_types.insert(dataType);
	}

	retcode = SQLFreeStmt(hstmt, SQL_CLOSE);
	check_error(retcode, (char*)"SQLFreeStmt()", hstmt, SQL_HANDLE_STMT);

	log_msg = "db_supported_types = [";
	for (const auto& x : db_supported_types)
	{
		log_msg += std::to_string(x);
		log_msg += ", ";
	}
	log_msg += "];";
	logger.log(log_msg, LOG_DEBUG);

	return db_supported_types;
};

std::string
Handler::get_timestamp_info(TIMESTAMP_STRUCT* ts){

	std::cout << "TIMESTAMP" << std::endl;
	std::cout << ts->year << ", "; 
	std::cout << ts->month << ", "; 
	std::cout << ts->day << ", "; 
	std::cout << ts->hour << ", "; 
	std::cout << ts->minute << ", "; 
	std::cout << ts->second << ", "; 
	std::cout << ts->fraction << ", "; 
	std::cout << std::endl;

	char buff[100];

	sprintf(buff, "%02d-%02d-%02d %02d:%02d:%02d.%03d", ts->year, ts->month, ts->day, ts->hour, ts->minute, ts->second, ts->fraction);

	std::cout << "buff: " << buff << std::endl;

	return buff;
};

std::string 
Handler::get_date_info(tagDATE_STRUCT* ts){

	std::cout << "tagDATE_STRUCT" << std::endl;
	std::cout << ts->year << ", "; 
	std::cout << ts->month << ", "; 
	std::cout << ts->day << ", "; 
	//~ std::cout << ((TIMESTAMP_STRUCT*)ColumnData[j])->hour << ", "; 
	//~ std::cout << ((TIMESTAMP_STRUCT*)ColumnData[j])->minute << ", "; 
	//~ std::cout << ((TIMESTAMP_STRUCT*)ColumnData[j])->second << ", "; 
	//~ std::cout << ((TIMESTAMP_STRUCT*)ColumnData[j])->fraction << ", "; 
	std::cout << std::endl;

	char buff[100];

	sprintf(buff, "%02d-%02d-%02d", ts->year, ts->month, ts->day);

	std::cout << "buff: " << buff << std::endl;

	return buff;



};

std::string 
Handler::get_time_info(tagTIME_STRUCT* ts){

	std::cout << "tagTIME_STRUCT" << std::endl;
	//~ std::cout << ((TIMESTAMP_STRUCT*)ColumnData[j])->year << ", "; 
	//~ std::cout << ((TIMESTAMP_STRUCT*)ColumnData[j])->month << ", "; 
	//~ std::cout << ((TIMESTAMP_STRUCT*)ColumnData[j])->day << ", "; 
	std::cout << ts->hour << ", "; 
	std::cout << ts->minute << ", "; 
	std::cout << ts->second << ", "; 
	//~ std::cout << ((TIMESTAMP_STRUCT*)ColumnData[j])->fraction << ", "; 
	std::cout << std::endl;

	char buff[100];

	sprintf(buff, "%02d:%02d:%02d", ts->hour, ts->minute, ts->second);

	std::cout << "buff: " << buff << std::endl;

	return buff;

};

void
Handler::set_date_info(std::string & date_str, SQL_DATE_STRUCT & ds){

	//YYYY-MM-DD
	//sscanf

	std::string year_str = date_str.substr(0, 4);
	std::string month_str = date_str.substr(5, 2);
	std::string day_str = date_str.substr(8, 2);

	std::cout << date_str << ":" << date_str.size() << std::endl;
	std::cout << year_str << std::endl;
	std::cout << month_str << std::endl;
	std::cout << day_str << std::endl;

	ds.year = std::stoi(year_str);
	ds.month = std::stoi(month_str);
	ds.day = std::stoi(day_str);
};

void
Handler::set_time_info(std::string & date_str, SQL_TIME_STRUCT & ts){

	//HH:MM:SS

	std::string hour_str = date_str.substr(0, 2);
	std::string min_str = date_str.substr(3, 2);
	std::string sec_str = date_str.substr(6, 2);

	std::cout << date_str << ":" << date_str.size() << std::endl;
	std::cout << hour_str << std::endl;
	std::cout << min_str << std::endl;
	std::cout << sec_str << std::endl;

	ts.hour = std::stoi(hour_str);
	ts.minute = std::stoi(min_str);
	ts.second = std::stoi(sec_str);
};


void
Handler::set_timestamp_info(std::string & date_str, SQL_TIMESTAMP_STRUCT & ds){

	//YYYY-MM-DD HH:MM:SS.PREC
	//0123456789


	std::string year_str = date_str.substr(0, 4);
	std::string month_str = date_str.substr(5, 2);
	std::string day_str = date_str.substr(8, 2);

	std::string hour_str = date_str.substr(11, 2);
	std::string min_str = date_str.substr(14, 2);
	std::string sec_str = date_str.substr(17, 2);

	std::string fraction_str = date_str.substr(20, 3);

	std::cout << date_str << ":" << date_str.size() << std::endl;
	std::cout << year_str << std::endl;
	std::cout << month_str << std::endl;
	std::cout << day_str << std::endl;
	std::cout << hour_str << std::endl;
	std::cout << min_str << std::endl;
	std::cout << sec_str << std::endl;
	std::cout << fraction_str << std::endl;


	ds.year = std::stoi(year_str);
	ds.month = std::stoi(month_str);
	ds.day = std::stoi(day_str);
	ds.hour = std::stoi(hour_str);
	ds.minute = std::stoi(min_str);
	ds.second = std::stoi(sec_str);
	ds.fraction = std::stoi(fraction_str);
};


void
Handler::describe_cols(int ncols){

	std::vector<SQLUSMALLINT> ColNumber(ncols); 
	std::vector<SQLCHAR*>     ColName(ncols); 
	std::vector<SQLSMALLINT>  BufLen(ncols); 
	std::vector<SQLSMALLINT>  NameLenPtr(ncols); 
	std::vector<SQLSMALLINT>  DataTypePtr(ncols); 
	std::vector<SQLULEN>      ColSizePtr(ncols); 
	std::vector<SQLSMALLINT>  DecDigitsPtr(ncols); 
	std::vector<SQLSMALLINT>  NullPtr(ncols); 

	int buff_len = 255;

	for (int i=0; i<ncols; i++){
		ColName[i] = (SQLCHAR *) malloc (buff_len);
	}

	for (int i=0; i<ncols; i++){

		retcode = SQLDescribeCol(
			hstmt,
			i+1,
			ColName[i],
			buff_len,
			&NameLenPtr[i],
			&DataTypePtr[i],
			&ColSizePtr[i],
			&DecDigitsPtr[i],
			&NullPtr[i]
		);

		check_error(retcode, "SQLDescribeCol(SQL_HANDLE_ENV)",
				hstmt, SQL_HANDLE_STMT);

	}

	print_vector(ColNumber);
	print_vector(ColName);
	print_vector(NameLenPtr);
	print_vector(DataTypePtr);
	print_vector(ColSizePtr);
	print_vector(DecDigitsPtr);
	print_vector(NullPtr);
};



std::unordered_map<std::string, int>
Handler::get_table_types(){

	//https://docs.microsoft.com/en-us/sql/odbc/reference/syntax/sqlcolumns-function?view=sql-server-ver15

	log_msg = "<get_table_types>";
	logger.log(log_msg, LOG_DEBUG);

	std::unordered_map<std::string, int> tabletypes;

	int STR_LEN = 255;  
	int REM_LEN = 255;

	SQLCHAR szSchema[255];
	SQLCHAR szCatalog[255];
	SQLCHAR szColumnName[255];
	SQLCHAR szTableName[255];
	SQLCHAR szTypeName[255];
	SQLCHAR szRemarks[255];
	SQLCHAR szColumnDefault[255];
	SQLCHAR szIsNullable[255];
	  
	SQLINTEGER ColumnSize;  
	SQLINTEGER BufferLength;  
	SQLINTEGER CharOctetLength;  
	SQLINTEGER OrdinalPosition;  
	  
	SQLSMALLINT DataType;  
	SQLSMALLINT DecimalDigits;  
	SQLSMALLINT NumPrecRadix;  
	SQLSMALLINT Nullable;  
	SQLSMALLINT SQLDataType;  
	SQLSMALLINT DatetimeSubtypeCode;  
	  
	// Declare buffers for bytes available to return  
	SQLLEN cbCatalog;  
	SQLLEN cbSchema;  
	SQLLEN cbTableName;  
	SQLLEN cbColumnName;  
	SQLLEN cbDataType;  
	SQLLEN cbTypeName;  
	SQLLEN cbColumnSize;  
	SQLLEN cbBufferLength;  
	SQLLEN cbDecimalDigits;  
	SQLLEN cbNumPrecRadix;  
	SQLLEN cbNullable;  
	SQLLEN cbRemarks;  
	SQLLEN cbColumnDefault;  
	SQLLEN cbSQLDataType;  
	SQLLEN cbDatetimeSubtypeCode;  
	SQLLEN cbCharOctetLength;  
	SQLLEN cbOrdinalPosition;  
	SQLLEN cbIsNullable;  

	retcode = SQLColumns(hstmt, NULL, 0, NULL, 0, (SQLCHAR*)table_name.c_str(), SQL_NTS, NULL, 0); 
	check_error(retcode, (char*)"SQLColumns(SQL_HANDLE_STMT)", hstmt, SQL_HANDLE_STMT);

	SQLBindCol(hstmt, 1, SQL_C_CHAR, szCatalog, STR_LEN,&cbCatalog);  
	SQLBindCol(hstmt, 2, SQL_C_CHAR, szSchema, STR_LEN, &cbSchema);  
	SQLBindCol(hstmt, 3, SQL_C_CHAR, szTableName, STR_LEN,&cbTableName);  
	SQLBindCol(hstmt, 4, SQL_C_CHAR, szColumnName, STR_LEN, &cbColumnName);  
	SQLBindCol(hstmt, 5, SQL_C_SSHORT, &DataType, 0, &cbDataType);  
	SQLBindCol(hstmt, 6, SQL_C_CHAR, szTypeName, STR_LEN, &cbTypeName);  
	SQLBindCol(hstmt, 7, SQL_C_SLONG, &ColumnSize, 0, &cbColumnSize);  
	SQLBindCol(hstmt, 8, SQL_C_SLONG, &BufferLength, 0, &cbBufferLength);  
	SQLBindCol(hstmt, 9, SQL_C_SSHORT, &DecimalDigits, 0, &cbDecimalDigits);  
	SQLBindCol(hstmt, 10, SQL_C_SSHORT, &NumPrecRadix, 0, &cbNumPrecRadix);  
	SQLBindCol(hstmt, 11, SQL_C_SSHORT, &Nullable, 0, &cbNullable);
	SQLBindCol(hstmt, 12, SQL_C_CHAR, szRemarks, REM_LEN, &cbRemarks);  
	SQLBindCol(hstmt, 13, SQL_C_CHAR, szColumnDefault, STR_LEN, &cbColumnDefault);  
	SQLBindCol(hstmt, 14, SQL_C_SSHORT, &SQLDataType, 0, &cbSQLDataType);  
	SQLBindCol(hstmt, 15, SQL_C_SSHORT, &DatetimeSubtypeCode, 0, &cbDatetimeSubtypeCode);  
	SQLBindCol(hstmt, 16, SQL_C_SLONG, &CharOctetLength, 0, &cbCharOctetLength);  
	SQLBindCol(hstmt, 17, SQL_C_SLONG, &OrdinalPosition, 0, &cbOrdinalPosition);  
	SQLBindCol(hstmt, 18, SQL_C_CHAR, szIsNullable, STR_LEN, &cbIsNullable);  

	while (SQL_SUCCESS == retcode){
		retcode = SQLFetch(hstmt);

		//No more data?
		if (retcode == SQL_NO_DATA) {
			break;
		}
		//~ std::cout << (char*)szCatalog << ", ";
		//~ std::cout << (char*)szSchema << ", ";
		//~ std::cout << (char*)szTableName << ", ";
		//~ std::cout << (char*)szColumnName << ", ";
		//~ std::cout << DataType << ", ";
		//~ std::cout << (char*)szTypeName << ", ";
		//~ std::cout << ColumnSize << ", ";
		//~ std::cout << BufferLength << ", ";
		//~ std::cout << DecimalDigits << ", ";
		//~ std::cout << NumPrecRadix << ", ";
		//~ std::cout << Nullable << ", ";
		//~ std::cout << (char*)szRemarks << ", ";
		//~ std::cout << (char*)szColumnDefault << ", ";
		//~ std::cout << SQLDataType << ", ";
		//~ std::cout << DatetimeSubtypeCode << ", ";
		//~ std::cout << CharOctetLength << ", ";
		//~ std::cout << OrdinalPosition << ", ";
		//~ std::cout << (char*)szIsNullable << ", ";
		//~ std::cout << std::endl;

		tabletypes[(char*)szColumnName] = (int)DataType;
	}

	log_msg = "tabletypes:";
	logger.log(log_msg, LOG_DEBUG);

	for (auto& it: tabletypes){
		log_msg = "\t";
		log_msg += it.first;
		log_msg += "\t";
		log_msg += std::to_string(it.second);
		logger.log(log_msg, LOG_DEBUG);
	}

	retcode = SQLFreeStmt(hstmt, SQL_CLOSE);
	check_error(retcode, (char*)"SQLFreeStmt()", hstmt, SQL_HANDLE_STMT);

	return tabletypes;
};


std::vector<std::string>
Handler::get_sql_colnames(const std::string & sql){

	std::vector<std::string> colnames;
	std::vector<std::string> tempvec;

	int first = 0;
	int count = 0;
	bool instring = false;

	for (int i=0; i<sql.size(); i++){

		char c = sql[i];

		//~ std::cout << c << std::endl;

		if (isspace((int)c) || c == ','){
			if (instring){
				tempvec.push_back(sql.substr(first, count));
			}
			instring = false;
			count = 0;
		}
		else if (c == '=' || c == '?' || c == ';'){
			std::string tempstr = std::string(1, c);
			tempvec.push_back(tempstr);
			instring = false;
			count = 0;
		}
		else if (c == '('){
			if (instring){
				tempvec.push_back(sql.substr(first, count));
			}
			instring = false;
			count = 0;
			//~ print_vector(tempvec);
			process_tokens(tempvec, colnames);
			tempvec.clear();
		}
		else if (c == ')'){
			if (instring){
				tempvec.push_back(sql.substr(first, count));
			}
			instring = false;
			count = 0;
			//~ print_vector(tempvec);
			process_list(tempvec, colnames);
			tempvec.clear();
		}
		else{
			if (!instring){
				instring = true;
				first = i;
			}
			count += 1;
		}
	}
	process_tokens(tempvec, colnames);

	return colnames;
};

void
Handler::process_tokens(const std::vector<std::string> & tokens, std::vector<std::string> & colnames){

	if (tokens.size() < 3){
		return;
	}
	for (int i=0; i<tokens.size()-2; i++){
		if (tokens[i+1] == "=" && tokens[i+2] == "?"){
			colnames.push_back(tokens[i]);
		}
	}
};

void
Handler::process_list(const std::vector<std::string> & tokens, std::vector<std::string> & colnames){

	for (int i=0; i<tokens.size(); i++){
		if (tokens[i] != "?"){
			colnames.push_back(tokens[i]);
		}
	}
};



std::string Handler::get_SQLBindParameter_string(int paramnum, int amplcol, int ttype, int ctype, int sqltype){

	std::string temp;

	std::string msg = "Binding param ";
	msg += std::to_string(paramnum);
	msg += " (";
	msg += get_col_name(amplcol);
	msg += ") of type ";

	if (odbc_types_map.find(ttype) != odbc_types_map.end()){
		temp = odbc_types_map[ttype];
	}
	else{
		temp = std::to_string(ttype);
	}
	msg += temp;
	msg += " : ";
	msg += c_types_map[ctype];
	msg += " -> ";
	msg += odbc_types_map[sqltype];
	return msg;
};








