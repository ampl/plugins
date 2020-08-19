#include "masterheader.hpp"


Connector::Connector(){
	is_writer = false;
};


Connector::~Connector(){};


// Table info functions

int
Connector::nkeycols(){
	return TI->arity;
};


int
Connector::ndatacols(){
	return TI->ncols;
};


int
Connector::ncols(){
	return TI->arity + TI->ncols;
};


int
Connector::nrows(){
	return TI->nrows;
};


void
Connector::set_col_val(double val, int col){
	TI->cols[col].dval[0] = val;
};


void
Connector::set_col_val(std::string & val, int col){
	TI->cols[col].sval[0] = const_cast<char*>(val.c_str());
};


void
Connector::set_col_val(char* val, int col){
	TI->cols[col].sval[0] = val;
};


void
Connector::add_row(){

	// pass data to AMPL
	DbCol * db = TI->cols;
	if ((*TI->AddRows)(TI, db, 1)){
		log_msg = "Error with AddRows";
		logger.log(log_msg, LOG_ERROR);
		throw DBE_Error;
	}
};


double
Connector::get_numeric_val(int row, int col){
	return TI->cols[col].dval[row];
};


char*
Connector::get_char_val(int row, int col){
	return TI->cols[col].sval[row];
};


bool
Connector::is_char_val(int row, int col){

	if (TI->cols[col].sval && TI->cols[col].sval[row]){
		return true;
	}
	return false;
};


bool
Connector::is_numeric_val(int row, int col){
	return !is_char_val(row, col);
};


char*
Connector::get_col_name(int col){
	return TI->colnames[col];
};

// AMPL Export functions

int
Connector::ampl_fprintf(FILE* stream, const char* format, ...){

	va_list va;
	va_start (va, format);
	int res = ae->FprintF(stream, format, va);
	va_end(va);
	return res;
};


int
Connector::ampl_printf(const char* format, ...){

	va_list va;
	va_start (va, format);
	int res =  ae->PrintF(format, va);
	va_end(va);
	return res;
};


int
Connector::ampl_sprintf(char* str, const char* format, ...){

	va_list va;
	va_start (va, format);
	int res = ae->SprintF(str, format, va);
	va_end(va);
	return res;
};


int
Connector::ampl_vfprintf(FILE* stream, const char* format, va_list arg){
	return ae->VfprintF(stream, format, arg);
};


int
Connector::ampl_vsprintf(char* buffer, const char* format, va_list arg){
	return ae->VsprintF(buffer, format, arg);
};


double
Connector::ampl_strtod(const char* str, char** endptr){
	return ae->Strtod(str, endptr);
};


void
Connector::add_ampl_connections(AmplExports *ae, TableInfo *TI){

	this->ae = ae;
	this->TI = TI;

	if (ae == NULL){
		std::cout << "Connector: could not add ampl exports." << std::endl;
		throw DBE_Error;
	}

	if (TI == NULL){
		std::cout << "Connector: could not add table info." << std::endl;
		throw DBE_Error;
	}
	// pass connections to the logger
	logger.add_info(ae, TI);

	// log the name and version of the handler
	log_msg = version;
	logger.log(log_msg, LOG_INFO);

	register_handler_names();
	register_handler_extensions();
};


void
Connector::parse_arguments(){

	log_msg = "<parse_arguments>";
	logger.log(log_msg, LOG_DEBUG);

	// at least the table handler must be declared
	if (TI->nstrings == 0){
		log_msg = "No table handler declared.\n";
		logger.log(log_msg, LOG_ERROR);
		throw DBE_Refuse;
	}

	// first string must be the table handler
	bool found = false;
	std::string tmp_str = TI->strings[0];
	for (int i = 0; i < handler_names.size(); i++){
		if (compare_strings_lower(handler_names[i], tmp_str)){
			found = true;
			break;
		}
	}
	if (!found){
		log_msg = "No table handler declaration found.\n";
		logger.log(log_msg, LOG_ERROR);
		throw DBE_Refuse;
	}

	// get the name of the table
	table_name = TI->tname;
	if (table_name.size() == 0){
		log_msg = "Could not get the name of the table.\n";
		logger.log(log_msg, LOG_ERROR);
		throw DBE_Error;
	}

	// log table name
	log_msg = "table: " + table_name;
	logger.log(log_msg, LOG_INFO);


	// check value for inout
	if ((TI->flags & DBTI_flags_IN) && (TI->flags & DBTI_flags_OUT)){
		inout = "INOUT";
	}
	else if (TI->flags & DBTI_flags_IN){
		inout = "IN";
	}
	else if (TI->flags & DBTI_flags_OUT){
		inout = "OUT";
	}
	else{
		log_msg = "unsuported inout flag";
		logger.log(log_msg, LOG_ERROR);
		throw DBE_Error;
	}

	// log inout
	log_msg = "inout: " + inout;
	logger.log(log_msg, LOG_INFO);

	// parse remaining args and check for verbose
	for (int i = 1; i < TI->nstrings; i++){

		std::string arg_string = TI->strings[i];
		size_t eq_pos = std::string(TI->strings[i]).find("=");

		// single argument
		if (eq_pos == std::string::npos){

			// check verbose
			if (arg_string == "verbose"){
				// set basic level of verbose (warnings)
				log_msg = "verbose: 1";
				logger.log(log_msg, LOG_INFO);
				logger.set_level(1);
			}
			// add for posterior validation
			else{
				args.push_back(arg_string);
			}
		}
		// key, value arguments with "=" separator
		else{
			std::string key = arg_string.substr(0, eq_pos); 

			if (eq_pos < arg_string.size() - 1){

				std::string val = arg_string.substr(eq_pos + 1); 

				// ignore value if key already exists
				if (kargs_map.find(key) != kargs_map.end()){
					log_msg = "key: " + key + " with val: " + kargs_map[key] + " already exists, ignoring: " + arg_string;
					logger.log(log_msg, LOG_WARNING);
				}
				// check verbose
				else if (key == "verbose"){
					int verbose_level = std::atoi(val.c_str());
					if (verbose_level > 0){
						log_msg = "verbose: " + val;
						logger.log(log_msg, LOG_INFO);
						logger.set_level(verbose_level);
					}
				}
				// add to map for posterior validation
				else{
					kargs.push_back(key);
					kargs_map[key] = val;
				}
			}
			// discard incomplete statements like "something="
			else{
				log_msg = "Could not parse " + arg_string;
				logger.log(log_msg, LOG_WARNING);
			}
		}
	}

	// print previous messages in logger (if requested)
	if (logger.level > 0){
		logger.print_log();
	}
};


void
Connector::validate_arguments(){

	log_msg = "<validate_arguments>";
	logger.log(log_msg, LOG_DEBUG);

	// validate single arguments
	bool has_alias = false;
	bool has_file = false;

	for (int i = 0; i < args.size(); i++){
		std::string arg = args[i];

		// arguments
		if (arg == "mydummyarg"){
			// do something
		}
		// check if the string is a potential file of the given extensions
		else if (is_handler_extensions(arg)){
			if (!has_file){
				filepath = arg;
				has_file = true;
				log_msg = "filepath: " + arg;
				logger.log(log_msg, LOG_INFO);
			}
			else{
				log_msg = "ignoring argument: " + arg;
				logger.log(log_msg, LOG_WARNING);
			}
		}
		// check for an alias
		else{
			if (!has_alias){
				table_name = arg;
				has_alias = true;
				log_msg = "using alias: " + arg;
				logger.log(log_msg, LOG_WARNING);
			}
			// already has alias, ignore argument
			else{
				log_msg = "ignoring argument: " + arg;
				logger.log(log_msg, LOG_WARNING);
			}
		}
	}

	// validate key/val arguments
	for (int i = 0; i < kargs.size(); i++){
		std::string key = kargs[i];
		std::string val = kargs_map[key];

		if (key == "mydummykey"){
			// do something with key/val
		}
		else{
			log_msg = "Ignoring key argument: " + key + " with val: " + kargs_map[key];
			logger.log(log_msg, LOG_WARNING);
		}
	}
};


bool
Connector::is_handler_extensions(const std::string & arg){

	log_msg = "<is_handler_extensions>";
	logger.log(log_msg, LOG_DEBUG);

	// no extensions to validate
	if (handler_extensions.size() == 0){
		return false;
	}

	std::string extension = get_file_extension(arg);

	// not a potential extension
	if (extension.empty()){
		return false;
	}

	for (int i = 0; i < handler_extensions.size(); i++){
		if (extension == handler_extensions[i]){
			return true;
		}
	}
	return false;
};


void
Connector::validate_filepath(){

	log_msg = "<validate_filepath>";
	logger.log(log_msg, LOG_DEBUG);

	if (filepath.empty()){
		filepath = table_name;

		// see if we can add extension
		if (handler_extensions.size() > 0){
			filepath += ".";
			filepath += handler_extensions[0];
			log_msg = "filepath updated: " + filepath;
			logger.log(log_msg, LOG_WARNING);
		}
		else{
			log_msg = "Could not add extension to filepath: " + filepath;
			logger.log(log_msg, LOG_WARNING);
		}
	}
};


void
Connector::prepare(){

	log_msg = "<prepare>";
	logger.log(log_msg, LOG_DEBUG);

	parse_arguments();
	validate_arguments();
	validate_filepath();

	// check if filepath exists
	if(!check_file_exists(filepath)){

		if (!is_writer){
			log_msg = "Cannot find source to read data.";
			logger.log(log_msg, LOG_ERROR);
			throw DBE_Error;
		}
		else{
			// write as an OUT table as there is nothing to update
			inout = "OUT";
			generate_table();

			log_msg = "generating file: " + filepath;
			logger.log(log_msg, LOG_WARNING);
		}
	}
};


void
Connector::run(){

	log_msg = "<run>";
	logger.log(log_msg, LOG_DEBUG);

	if (!is_writer){
		read_in();
	}
	else{
		if (inout == "OUT"){
			write_out();
		}
		else if (inout == "INOUT"){
			write_inout();
		}
	}
};
