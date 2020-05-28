#include "ampltableconnector.hpp"


static int
Read_Example(AmplExports *ae, TableInfo *TI){

	int res = DBE_Done;
	ReadConnector rc;

	try{
		rc.add_ampl_connections(ae, TI);
		rc.prepare();
		rc.run();
	}
	catch (DBE e){
		// something went wrong
		// the error must be in the logger
		res = e;
	}
	return res;
};


static int
Write_Example(AmplExports *ae, TableInfo *TI){

	int res = DBE_Done;
	WriteConnector wc;

	try{
		wc.add_ampl_connections(ae, TI);
		wc.prepare();
		wc.run();
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

	printf("Add meaningfull names to Read_Example and Write_Example in funcadd() and in the two  previous functions. Also update the string version in the header file. Remove this print afterwards.\n");

	// write a description of the handlers
	static char info[] = "handler name\n"
		"Write table handler description and help\n";

	// Inform AMPL about the handlers
	add_table_handler(Read_Example, Write_Example, info, 0, 0);
};


Connector::Connector(){
	is_writer = false;
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
Connector::register_handler_names(){

	log_msg = "<register_handler_names>";
	logger.log(log_msg, LOG_DEBUG);

	// use something like the following to add handler names, this will be compatible with older
	// compiler versions
	handler_names.push_back("examplehandler");
	handler_names.push_back("examplehandler.dll");
};


void
Connector::register_handler_extensions(){

	log_msg = "<register_handler_extensions>";
	logger.log(log_msg, LOG_DEBUG);

	// use something like the following to add extension names, this will be compatible with older
	// compiler versions

	handler_extensions.push_back("hxt");

	// if we need to create a file the first handler extension will be used
	// handler_extensions might be empty, for example if it is defined in kargs_map 
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
ReadConnector::prepare(){

	log_msg = "<prepare>";
	logger.log(log_msg, LOG_DEBUG);

	parse_arguments();
	validate_arguments();
	validate_filepath();

	// check if filepath exists
	if(!check_file_exists(filepath)){

		log_msg = "Cannot find source to read data.";
		logger.log(log_msg, LOG_ERROR);
		throw DBE_Error;
	}
};


void
ReadConnector::run(){

	log_msg = "<run>";
	logger.log(log_msg, LOG_DEBUG);

	read_in();
};


void
ReadConnector::read_in(){

	log_msg = "<read_in>";
	logger.log(log_msg, LOG_DEBUG);

	// In this method you will get the data from your external representation of the table and pass
	// it to AMPL.

	// TI->arity is the number of indexing columns in AMPLs representation of the table
	// TI->ncols is the number of data columns in AMPLs representation of the table
	// TI->colnames[j] holds the name of column j
	// TI->cols[j] is a DBcol Struct that has a pointer to a pointer to hold characters values and a
	// pointer to real to hold numeric values
	//
	//	typedef struct
	//	DbCol {
	//		real	*dval;
	//		char	**sval;
	//	} DbCol;
	//
	// for example to pass a string to column i use
	//     TI->cols[i].sval[0] = mystring;
	//
	// if you want to pass the numeric value 5 to column i you can do
	//     TI->cols[i].sval[0] = NULL;
	//     TI->cols[i].dval[0] = 5;
	//
	// After filling all the values pass the row to AMPL with
	// *TI->AddRows

	// A small example follows. Consider a table with 10 rows and 3 columns (keys, string_values and
	// numeric_values) with data contained in vectors. The table is iterated row by row and each row
	// is individually passed to AMPL. Use this code with the file ../tests/test_in.run 

	int nrows = 10;

	std::vector<std::string> keys;
	std::vector<std::string> string_values;
	std::vector<real> numeric_values;

	keys.reserve(nrows);
	string_values.reserve(nrows);
	numeric_values.reserve(nrows);

	// generate some data
	for (int i = 0; i < nrows; i++){
		keys.push_back("k" + numeric_to_string(i + 1));
		string_values.push_back("val" + numeric_to_string(i + 1));
		numeric_values.push_back(2 * (i + 1));
	}

	// iterate rows of data
	for (int i = 0; i < nrows; i++){

		TI->cols[0].sval[0] = const_cast<char*>(keys[i].c_str());
		TI->cols[1].sval[0] = const_cast<char*>(string_values[i].c_str());
		TI->cols[2].dval[0] = numeric_values[i];

		// pass data to AMPL
		DbCol * db = TI->cols;
		if ((*TI->AddRows)(TI, db, 1)){
			log_msg = "Error with AddRows";
			logger.log(log_msg, LOG_ERROR);
			throw DBE_Error;
		}
	}

	// Note that in the previous example each column had a numeric or string type. However AMPL has
	// symbolic sets/parameters that can contain both numeric and string values. To pass data to an
	// AMPL column in that situation use something like:
	//	if (is_string){
	//		TI->cols[i].sval[0] = mystring;
	//	}
	//	else{
	//		TI->cols[i].sval[0] = NULL;
	//		TI->cols[i].dval[0] = 5;
	//	}
	// A detailed description of table handlers management is available at
	// https://ampl.com/netlib/ampl/tables/index.html

	printf("Implement read_in() as needed and remove this print afterwards.\n");
};


WriteConnector::WriteConnector(){
	is_writer = true;
};


void
WriteConnector::prepare(){

	log_msg = "<prepare>";
	logger.log(log_msg, LOG_DEBUG);

	parse_arguments();
	validate_arguments();
	validate_filepath();

	// check if filepath already exists
	if(!check_file_exists(filepath)){

		// write as an OUT table as there is nothing to update
		inout = "OUT";
		generate_file();

		log_msg = "generating file: " + filepath;
		logger.log(log_msg, LOG_WARNING);
	}
};


void
WriteConnector::run(){

	log_msg = "<run>";
	logger.log(log_msg, LOG_DEBUG);

	if (inout == "OUT"){
		write_out();
	}
	else if (inout == "INOUT"){
		write_inout();
	}
};


void
WriteConnector::write_out(){

	log_msg = "<write_out>";
	logger.log(log_msg, LOG_DEBUG);

	// This method should overwrite the external representation of the table with the data in AMPL's
	// table.
	// The following code is an example on how to get the data from AMPL, printing the column names
	// and the data in AMPL's representation of the table.

	// TI->arity is the number of indexing columns in AMPLs representation of the table
	// TI->ncols is the number of data columns in AMPLs representation of the table
	// tcols is the total number od columns in the table

	const int tcols = TI->arity + TI->ncols;

	// TI->colnames[j] holds the name of column j

	// iterate the column names
	for (int j = 0; j < tcols; j++){
		printf("%s", TI->colnames[j]);
		if (j < tcols - 1){
			printf("\t");
		}
	}
	printf("\n");

	// TI->nrows is the number of rows in AMPLs representation of the table

	// iterate rows and columns printing data
	for (int i = 0; i < TI->nrows; i++){
		for (int j = 0; j < tcols; j++){

			if (TI->cols[j].sval && TI->cols[j].sval[i]){
				// string value
				printf("%s", TI->cols[j].sval[i]);
			}
			else{
				// numeric value
				printf("%g", TI->cols[j].dval[i]);
			}
			if (j < tcols - 1){
				printf("\t");
			}
		}
		printf("\n");
	}

	// A detailed description of table handlers management is available at
	// https://ampl.com/netlib/ampl/tables/index.html

	printf("Implement write_out() as needed and remove this print afterwards.\n");
};


void
WriteConnector::write_inout(){

	log_msg = "<write_inout>";
	logger.log(log_msg, LOG_DEBUG);

	// Unlike write_out() this method should update the external representation of the table with
	// the data in AMPL's table.
	// For an example on how to get the data from AMPL see write_out() .

	// A detailed description of table handlers management is available at
	// https://ampl.com/netlib/ampl/tables/index.html

	// implement the method as needed and remove the following error
	log_msg = "write_inout() not implemented";
	logger.log(log_msg, LOG_ERROR);
	throw DBE_Error;
};


void
WriteConnector::generate_file(){

	log_msg = "<generate_file>";
	logger.log(log_msg, LOG_DEBUG);

	// Implement a method to generate a file (or something else) for the developed handler with
	// name filepath.

	// Implement the method as needed and remove the following error
	log_msg = "generate_file() not implemented";
	logger.log(log_msg, LOG_ERROR);
	throw DBE_Error;
};
