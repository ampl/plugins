#include "handler.hpp"


static int
Read_sqlite3(AmplExports *ae, TableInfo *TI){

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
Write_sqlite3(AmplExports *ae, TableInfo *TI){

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
	add_table_handler(ae, Read_sqlite3, Write_sqlite3, const_cast<char *>(doc.c_str()), 0, 0);
};

// Adapt the functions bellow to meet your requirements

Handler::~Handler(){
	log_msg = "<dtor>";
	logger.log(log_msg, LOG_DEBUG);
	sqlite3_close(db);
};

void
Handler::read_in(){

	log_msg = "<read_in>";
	logger.log(log_msg, LOG_DEBUG);

	sqlite3_stmt *stmt;
	char *zErrMsg = 0;
	int row = 0;
	int bytes;
	const unsigned char *text;
	int rc;

	// get the SQL statement
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

	rc = sqlite3_prepare_v2(db, sqlstr.c_str(), -1, &stmt, NULL);

	if (rc != SQLITE_OK){
		std::cout << "sqlite3_prepare_v2 returned " << rc << std::endl;
	}

	int nc = sqlite3_column_count(stmt);
	std::cout << "ncols:" << nc << std::endl;


	std::vector<std::string> sqlite_colnames;

	for(int i=0; i<nc; i++){

		fprintf(stdout, "%d. %s\n", i, sqlite3_column_name(stmt, i));
		sqlite_colnames.push_back(sqlite3_column_name(stmt, i));
		//~ const char *colname = sqlite3_column_name(stmt, i);
		//~ std::cout << "column name: " << zErrMsg << std::endl;
	}

	for(int i=0; i<nc; i++){
		std::cout << "column name vec: " << i <<", "<< sqlite_colnames[i] << std::endl;
	}


	while(1){
		rc = sqlite3_step(stmt);
		if (rc == SQLITE_ROW){
			for(int i=0; i<nc; i++){

				int ctype = sqlite3_column_type(stmt, i);

				if (ctype == SQLITE_NULL){
					set_col_missing_val(i);
				}
				else if (ctype == SQLITE_INTEGER){
					set_col_val((double)sqlite3_column_int64(stmt, i), i);
				}
				else if (ctype == SQLITE_FLOAT){
					set_col_val(sqlite3_column_double(stmt, i), i);
				}
				else{
					set_col_val((char*)sqlite3_column_text(stmt, i), i);
				}
			}
			add_row();
		}
		else if (rc == SQLITE_DONE){
			break;
		}
		else{
			fprintf(stderr, "Failed.\n");
			//~ return 1;
			break;
		}
	}
	sqlite3_finalize(stmt);
};

void
Handler::write_out(){

	log_msg = "<write_out>";
	logger.log(log_msg, LOG_DEBUG);

	// get the db statement
	std::string sqlstr = get_stmt_insert();

	log_msg = "SQL: ";
	log_msg += sqlstr;
	logger.log(log_msg, LOG_INFO);

	// prepare the statement
	sqlite3_stmt *stmt;
	int rc;
	rc = sqlite3_prepare_v2(db, sqlstr.c_str(), -1, &stmt, NULL);
	std::cout << "sqlite3_prepare_v2: " << rc << std::endl;
	if (rc != SQLITE_OK){
		log_msg = "Error calling sqlite3_prepare_v2";
		logger.log(log_msg, LOG_ERROR);
		sqlite3_finalize(stmt);
		throw DBE_Error;
	}

	bool all_good = true;
	sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL);
	log_msg = "BEGIN TRANSACTION;";
	logger.log(log_msg, LOG_DEBUG);

	for (size_t i=0; i<nrows(); i++){
		for (size_t j=0; j<ncols(); j++){

			if (is_missing(i, j)){
				// nothing to do
			}
			else{
				if (amplcoltypes[j] == 0){
					double tempd = get_numeric_val(i, j);
					sqlite3_bind_double(stmt, j+1, tempd);
				}
				else if (amplcoltypes[j] == 1){
					char* tmp = get_char_val(i, j);
					sqlite3_bind_text(stmt, j+1, tmp, -1, NULL);
				}
				else{
					// should never get here
					// unless you add a new AMPL column type
				}
			}
		}
		rc = sqlite3_step(stmt);
		if (rc != SQLITE_DONE){
			all_good = false;
			log_msg = "main loop: sqlite3_step failed with code: " + std::to_string(rc);
			logger.log(log_msg, LOG_DEBUG);
			break;
		}

		rc = sqlite3_reset(stmt);
		if (rc != SQLITE_OK){
			all_good = false;
			log_msg = "main loop: sqlite3_reset failed with code: " + std::to_string(rc);
			logger.log(log_msg, LOG_DEBUG);
			break;
		}
	}
	validate_transaction(all_good);
	sqlite3_finalize(stmt);
};

void
Handler::validate_transaction(bool all_good){

	if (all_good){
		sqlite3_exec(db, "END TRANSACTION;", NULL, NULL, NULL);
		log_msg = "END TRANSACTION;";
		logger.log(log_msg, LOG_DEBUG);
	}
	else{
		sqlite3_exec(db, "ROLLBACK TRANSACTION;", NULL, NULL, NULL);
		log_msg = "ROLLBACK TRANSACTION;";
		logger.log(log_msg, LOG_DEBUG);
	}
};


void
Handler::write_inout(){

	log_msg = "<write_inout>";
	logger.log(log_msg, LOG_DEBUG);

	// Get the db statement
	std::string sqlstr = get_stmt_update();
	log_msg = "SQL: ";
	log_msg += sqlstr;
	logger.log(log_msg, LOG_INFO);

	sqlite3_stmt *stmt;
	int rc;
	rc = sqlite3_prepare_v2(db, sqlstr.c_str(), -1, &stmt, NULL);
	std::cout << "sqlite3_prepare_v2: " << rc << std::endl;
	if (rc != SQLITE_OK){
		log_msg = "Error calling sqlite3_prepare_v2";
		logger.log(log_msg, LOG_ERROR);
		sqlite3_finalize(stmt);
		throw DBE_Error;
	}


	for(int i=0; i<ncols(); i++){
		std::cout << i << ", " << get_col_name(i) << ", " << amplcoltypes[i] << std::endl;
	}



	bool all_good = true;
	sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL);
	log_msg = "BEGIN TRANSACTION;";
	logger.log(log_msg, LOG_DEBUG);

	for (size_t i = 0; i < nrows(); i++){

		int pos = 0;

		for (size_t j = 0; j < ndatacols(); j++){

			if (is_missing(i, j)){
				//~ std::cout << "NULL" << ", ";
			}
			else{
				if (amplcoltypes[nkeycols()+j] == 0){
					double tempd = get_numeric_val(i, nkeycols()+j);
					sqlite3_bind_double(stmt, j+1, tempd);
				}
				else if (amplcoltypes[nkeycols()+j] == 1){
					char* tmp = get_char_val(i, nkeycols()+j);
					sqlite3_bind_text(stmt, j+1, tmp, -1, NULL);
				}
				else{
					// should never get here
				}
			}
		}

		for (size_t j = 0; j < nkeycols(); j++){

			if (is_missing(i, j)){
				//~ std::cout << "NULL" << ", ";
			}
			else{
				if (amplcoltypes[j] == 0){
					double tempd = get_numeric_val(i, j);
					sqlite3_bind_double(stmt, j+1+ndatacols(), tempd);
				}
				else if (amplcoltypes[j] == 1){
					char* tmp = get_char_val(i, j);
					sqlite3_bind_text(stmt, j+1+ndatacols(), tmp, -1, NULL);
				}
				else{
					// should never get here
				}
			}
		}
		rc = sqlite3_step(stmt);
		if (rc != SQLITE_DONE){
			all_good = false;
			log_msg = "main loop: sqlite3_step failed with code: " + std::to_string(rc);
			logger.log(log_msg, LOG_DEBUG);
			break;
		}

		rc = sqlite3_reset(stmt);
		if (rc != SQLITE_OK){
			all_good = false;
			log_msg = "main loop: sqlite3_reset failed with code: " + std::to_string(rc);
			logger.log(log_msg, LOG_DEBUG);
			break;
		}
	}
	validate_transaction(all_good);
	sqlite3_finalize(stmt);
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
			stmt += " VARCHAR";
		}
		else{
			stmt += " ";
			stmt += "REAL";
		}
		if (i < ncols() - 1){
			stmt += ", ";
		}
	}

	if(create_primary_keys){
		stmt += ", PRIMARY KEY (";
		for (size_t i = 0; i < nkeycols(); i++){
			stmt += get_col_name(i);
			if (i + 1 < nkeycols()){
				stmt += ", ";
			}
		}
		stmt += ")";
	}
	stmt += ");";

	return stmt;
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
	handler_extensions = {"db"};
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

	allowed_kargs = {"primarykeys", "write", "SQL"};
};


void
Handler::validate_arguments(){

	log_msg = "<validate_arguments>";
	logger.log(log_msg, LOG_DEBUG);

	for(const auto it: user_kargs){

		std::string key = it.first;

		if (compare_strings_lower(key, "primarykeys")){
			create_primary_keys = get_bool_karg(key);
		}

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
		else if (compare_strings_lower(key, "SQL")){
			sql = it.second;
		}
	}

	if (is_writer){
		get_ampl_col_types();
	}

	int rc = sqlite3_open(filepath.c_str(), &db);

	if(rc){
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		throw DBE_Error;;
	}

	bool exists = table_exists();

	if (!exists){
		if (is_writer){
			table_create();
			inout = "OUT";
		}
		else{
			log_msg = "Could not find table " + table_name;
			logger.log(log_msg, LOG_WARNING);
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


bool
Handler::table_exists(){

	log_msg = "<table_exists>";
	logger.log(log_msg, LOG_DEBUG);

	sqlite3_stmt *stmt;

	std::string sqlstr = "select count(type) from sqlite_master where type='table' and name='"+table_name+"';";

	std::cout << "query: " << sqlstr << std::endl;

	int rc;
	rc = sqlite3_prepare_v2(db, sqlstr.c_str(), -1, &stmt, NULL);
	std::cout << "sqlite3_prepare_v2: " << rc << std::endl;
	rc = sqlite3_step(stmt);
	std::cout << "sqlite3_step: " << rc << std::endl;

	int exists = sqlite3_column_int64(stmt, 0);
	std::cout << "exists: " << exists << std::endl;

	sqlite3_finalize(stmt);

	if (exists == 1){
		log_msg = "Table ";
		log_msg += table_name;
		log_msg += " found.";
		logger.log(log_msg, LOG_INFO);
		return true;
	}
	else{
		log_msg = "Cannot find table ";
		log_msg += table_name;
		logger.log(log_msg, LOG_INFO);
		return false;
	}
};

void
Handler::table_create(){

	log_msg = "<table_create>";
	logger.log(log_msg, LOG_DEBUG);

	std::string sqlstr = get_stmt_create();
	log_msg = "SQL: " + sqlstr;
	logger.log(log_msg, LOG_INFO);

	sqlite3_stmt *stmt;
	int rc;
	rc = sqlite3_prepare_v2(db, sqlstr.c_str(), -1, &stmt, NULL);
	std::cout << "sqlite3_prepare_v2: " << rc << std::endl;
	rc = sqlite3_step(stmt);
	std::cout << "sqlite3_step: " << rc << std::endl;
	sqlite3_finalize(stmt);
};

void
Handler::table_delete(){

	log_msg = "<table_delete>";
	logger.log(log_msg, LOG_DEBUG);

	std::string sqlstr = get_stmt_delete();
	log_msg = "SQL: " + sqlstr;
	logger.log(log_msg, LOG_INFO);

	sqlite3_stmt *stmt;
	int rc;
	rc = sqlite3_prepare_v2(db, sqlstr.c_str(), -1, &stmt, NULL);
	std::cout << "sqlite3_prepare_v2: " << rc << std::endl;
	//~ sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL);
	rc = sqlite3_step(stmt);
	std::cout << "sqlite3_step: " << rc << std::endl;
	//~ sqlite3_exec(db, "END TRANSACTION;", NULL, NULL, NULL);
	sqlite3_finalize(stmt);

};

void
Handler::table_drop(){

	log_msg = "<table_drop>";
	logger.log(log_msg, LOG_DEBUG);

	std::string sqlstr = get_stmt_drop();
	log_msg = "SQL: " + sqlstr;
	logger.log(log_msg, LOG_INFO);

	sqlite3_stmt *stmt;
	int rc;
	rc = sqlite3_prepare_v2(db, sqlstr.c_str(), -1, &stmt, NULL);
	std::cout << "sqlite3_prepare_v2: " << rc << std::endl;
	rc = sqlite3_step(stmt);
	std::cout << "sqlite3_step: " << rc << std::endl;
	sqlite3_finalize(stmt);
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
