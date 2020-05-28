#include "ampltableconnector.hpp"


static int
Read_Amplcsv(AmplExports *ae, TableInfo *TI){

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
Write_Amplcsv(AmplExports *ae, TableInfo *TI){

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

	// write a description of the handlers
	static char info[] = "amplcsv\n"
		"Simple csv reader for AMPL\n";

	// Inform AMPL about the handlers
	add_table_handler(Read_Amplcsv, Write_Amplcsv, info, 0, 0);
};


Connector::Connector(){
	is_writer = false;
	sep = (char*)",";
	quotechar = (char*)"\"";
	quotestrings = false;
	has_header = true;
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
	handler_names.push_back("amplcsv");
	handler_names.push_back("amplcsv.dll");
};


void
Connector::register_handler_extensions(){

	log_msg = "<register_handler_extensions>";
	logger.log(log_msg, LOG_DEBUG);

	// use something like the following to add extension names, this will be compatible with older
	// compiler versions

	handler_extensions.push_back("csv");
	handler_extensions.push_back("txt");

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

		// check if the string is a potential file of the given extensions
		if (is_handler_extensions(arg)){
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

		// check separator
		if (key == "sep"){
			if (val == ","){
				// default
			}
			else if (val == ";"){
				sep = (char*)";";
			}
			else if (val == ":"){
				sep = (char*)":";
			}
			else if (val == "tab"){
				sep = (char*)"\t";
			}
			else if (val == "space"){
				sep = (char*)" ";
			}
			else{
				log_msg = "Ignoring sep option: sep=" + val;
				logger.log(log_msg, LOG_WARNING);
			}
		}
		// check quotation character
		else if (key == "quotechar"){
			if (val == "double"){
				// default
			}
			else if (val == "single"){
				quotechar = (char*)"'";
			}
			else{
				log_msg = "Ignoring quotechar option: quotechar=" + val;
				logger.log(log_msg, LOG_WARNING);
			}
		}
		// check header option
		else if (key == "header"){
			if (val == "true"){
				// default
			}
			else if (val == "false"){
				has_header = false;
			}
			else{
				log_msg = "Ignoring header option: header=" + val;
				logger.log(log_msg, LOG_WARNING);
			}
		}
		// check quotestrings option
		else if (key == "quotestrings"){
			if (val == "false"){
				// default
			}
			else if (val == "true"){
				quotestrings = true;
			}
			else{
				log_msg = "Ignoring quotestrings option: quotestrings=" + val;
				logger.log(log_msg, LOG_WARNING);
			}
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
Connector::parse_row(std::string & str, std::vector<std::string> & row){

	bool is_first = true;
	int i = 0;
	int lb = 0;
	int ub = 0;

	while (true){
		if (str[i] == *quotechar && is_first){
			is_first = false;
			lb = i;
			while (true){
				i += 1;
				if (i == str.size()){
					i -= 1;
					break;
				}
				if (str[i] == *quotechar && i + 1 < str.size() && str[i + 1] != *quotechar){
					ub = i;
					break;
				}
			}
		}
		else if (str[i] == *sep){
			if (is_first){
				row.push_back(std::string());
			}
			else{
				ub = i;
				row.push_back(str.substr(lb, ub-lb));
			}
			is_first = true;
		}
		else if (i == str.size()){
			ub = i;
			row.push_back(str.substr(lb, ub-lb));
			break;
		}
		else{
			if (is_first){
				is_first = false;
				lb = i;
			}
		}
		i += 1;
	}
};


void
Connector::validate_header(const std::vector<std::string> & header, std::vector<int> & perm){

	std::map<std::string, int> csv_col_map;
	std::map<std::string, int> ampl_col_map;

	// get a map of the columns in the external representation of the table 
	for (int i = 0; i < header.size(); i++){
		std::string tmp_str = header[i];
		if (quotestrings){
			try_unquote_string(tmp_str, quotechar);
		}
		csv_col_map[tmp_str] = i;
	}

	int tcols = TI->arity + TI->ncols;

	// confirm all ampl columns are in the external table
	for (int i = 0; i < tcols; i++){
		std::string temp_str = TI->colnames[i];
		if (csv_col_map.find(temp_str) == csv_col_map.end()){
			log_msg = "Could not find column " + temp_str + " in the external table.";
			logger.log(log_msg, LOG_ERROR);
			throw DBE_Error;
		}
	}

	// get a map for the columns in AMPL's table
	for (int i = 0; i < tcols; i++){
		std::string temp_str = TI->colnames[i];
		ampl_col_map[temp_str] = i;
	}

	perm.resize(tcols);

	for (int i = 0; i < tcols; i++){

		std::string tmp_str = header[i];

		if (quotestrings){
			try_unquote_string(tmp_str, quotechar);
		}

		if (ampl_col_map.find(tmp_str) != ampl_col_map.end()){
			perm[i] = ampl_col_map[tmp_str];
		}
		else{
			log_msg = "Could not find external column name " + tmp_str + " in AMPL's table.";
			logger.log(log_msg, LOG_WARNING);
			//~ throw DBE_Error;
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

	std::string str; // string to read the information of a single row of the csv file
	std::string tmp_str;
	std::ifstream infile(filepath.c_str()); // stream to read the data in the csv file

	if (!infile){
		log_msg = "Could not open " + filepath + " to read data.";
		logger.log(log_msg, LOG_ERROR);
		throw DBE_Error;
	}

	int nfields = 0; // number of fields (columns) in the csv table
	int tcols = TI->arity + TI->ncols; // number of columns in AMPLs table

	std::vector<std::string> header; // vector to store the strings in the csv header
	std::vector<std::string> row; // vector to store the strings in the csv rows

	// for each column i in the csv perm[i] gives the position of the same column in AMPLs table
	// this allows for the column order to be different in both tables 
	std::vector<int> perm;  

	int row_count = 0;

	// if a header is defined parse the header to get the number of fields (columns) in the csv and
	// and calculate perm to get the correspondence of position of the columns in the external table
	// to AMPLs table. 
	if (has_header){

		safeGetline(infile, str);
		parse_row(str, header);
		nfields = header.size();
		validate_header(header, perm);
		row_count += 1;
	}
	// otherwise we assume the number of fields equals the number of columns in AMPLs table and give
	// a direct correspondence in perm
	else{
		nfields = tcols;
		for (int i = 0; i < tcols; i++){
			perm.push_back(i);
		}
	}

	row.resize(nfields); // allocate space for the expected number of fields 

	// iterate rows of file
	while (true) {

		safeGetline(infile, str);

		if (!infile.good()){break;}

		row.clear();
		row_count += 1;

		parse_row(str, row);

		if (row.size() != nfields){
			log_msg = "Invalid number of fields when reading row ";
			log_msg += numeric_to_string(row_count);
			log_msg += ". Expected ";
			log_msg += numeric_to_string(nfields);
			log_msg += " got ";
			log_msg += numeric_to_string(row.size());
			logger.log(log_msg, LOG_ERROR);
			throw DBE_Error;
		}

		for (int j = 0; j < row.size(); j++){
			send_val_to_ampl_Dbcol(row[j], perm[j]);
		}

		DbCol * db = TI->cols;
		if ((*TI->AddRows)(TI, db, 1)){
			log_msg = "Error with AddRows";
			logger.log(log_msg, LOG_DEBUG);
			throw DBE_Error;
		}
	}
};


void
ReadConnector::send_val_to_ampl_Dbcol(std::string & val, int col){

	if (val.empty()){
		TI->cols[col].sval[0] = TI->Missing;
		return;
	}

	char* se;
	double t;

	// check if val is a number
	t = strtod(val.c_str(), &se);
	if (!*se) {/* valid number */
		TI->cols[col].sval[0] = 0;
		TI->cols[col].dval[0] = t;
	}
	else{
		// check for quoted char
		if (quotestrings){
			try_unquote_string(val, quotechar);

			if (val.empty()){
				TI->cols[col].sval[0] = TI->Missing;
				return;
			}
		}
		TI->cols[col].sval[0] = const_cast<char*>(val.c_str());;
	}
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

	FILE *f;
	f = fopen(filepath.c_str(), "w");

	if (!f){
		log_msg = "write_out: could not open " + filepath;
		logger.log(log_msg, LOG_ERROR);
		throw DBE_Error;
	}

	const int tcols = TI->arity + TI->ncols;

	// write header
	if (has_header){
		for (int j = 0; j < tcols; j++){

			if (quotestrings){
				fprintf (f, "%s%s%s", quotechar, TI->colnames[j], quotechar);
			}
			else{
				fprintf (f, "%s", TI->colnames[j]);
			}
			// add separator
			if (j < tcols - 1){
				fprintf (f, "%s", sep);
			}
		}
		fprintf (f, "\n");
	}

	// write data iterating by rows and columns
	for (int i = 0; i < TI->nrows; i++){
		for (int j = 0; j < tcols; j++){

			// check string value
			if (TI->cols[j].sval && TI->cols[j].sval[i]){

				if (TI->cols[j].sval[i] == TI->Missing){
					continue;
				}
				if (quotestrings){
					fprintf (f, "%s%s%s", quotechar, TI->cols[j].sval[i], quotechar);
				}
				else{
					fprintf (f, "%s", TI->cols[j].sval[i]);
				}
			}
			// otherwise numeric value
			else{
				fprintf (f, "%.g", TI->cols[j].dval[i]);
			}
			// add separator
			if (j < tcols - 1){
				fprintf (f, "%s", sep);
			}
		}
		fprintf (f, "\n");
	}
	fclose(f);
};


void
WriteConnector::write_inout(){

	log_msg = "<write_inout>";
	logger.log(log_msg, LOG_DEBUG);

	std::string str; // string to read the information of a single row of the csv file
	std::string tmp_str;
	std::ifstream infile(filepath.c_str()); // stream to read the data in the csv file

	if (!infile){
		log_msg = "Could not open " + filepath + " to read data.";
		logger.log(log_msg, LOG_ERROR);
		throw DBE_Error;
	}

	int nfields = 0; // number of fields (columns) in the csv table
	int tcols = TI->arity + TI->ncols; // number of columns in AMPLs table

	std::vector<std::string> header; // vector to store the strings in the csv header
	std::vector<std::string> row; // vector to store the strings in the csv rows

	// for each column i in the csv perm[i] gives the position of the same column in AMPLs table
	// this allows for the column order to be different in both tables 
	std::vector<int> perm;  

	int row_count = 0;

	// if a header is defined parse the header to get the number of fields (columns) in the csv and
	// and calculate perm to get the correspondence of position of the columns in the external table
	// to AMPLs table. 
	if (has_header){

		//~ std::getline(infile, str);
		safeGetline(infile, str);
		parse_row(str, header);
		nfields = header.size();
		validate_header(header, perm);
		row_count += 1;
	}
	// otherwise we assume the number of fields equals the number of columns in AMPLs table and give
	// a direct correspondence in perm
	else{
		nfields = tcols;
		for (int i = 0; i < tcols; i++){
			perm.push_back(i);
		}
	}

	// temporary file to write data
	std::string new_file = "./amplcsvtempfile.csv";

	FILE *f;
	f = fopen(new_file.c_str(), "w");

	if (!f){
		log_msg = "write_inout: could not open " + filepath;
		logger.log(log_msg, LOG_ERROR);
		throw DBE_Error;
	}

	// write header
	if (has_header){
		for (int j = 0; j < tcols; j++){

			if (quotestrings){
				fprintf (f, "%s%s%s", quotechar, TI->colnames[j], quotechar);
			}
			else{
				fprintf (f, "%s", TI->colnames[j]);
			}
			// add separator
			if (j < tcols - 1){
				fprintf (f, "%s", sep);
			}
		}
		fprintf (f, "\n");
	}

	// write AMPL's table and get a map of used keys
	std::map<std::vector<std::string>,int> used_keys_map;
	std::vector<std::string> temp_keys;

	for (int i = 0; i < TI->nrows; i++){

		temp_keys.clear();

		for (int j = 0; j < tcols; j++){

			tmp_str.clear();

			if (TI->cols[perm[j]].sval && TI->cols[perm[j]].sval[i]){

				if (TI->cols[j].sval[i] == TI->Missing){
					continue;
				}
				if (quotestrings){
					fprintf (f, "%s%s%s", quotechar, TI->cols[j].sval[i], quotechar);
					tmp_str = quotechar; 
					tmp_str += TI->cols[j].sval[i]; 
					tmp_str += quotechar;
				}
				else{
					fprintf (f, "%s", TI->cols[j].sval[i]);
					tmp_str = TI->cols[j].sval[i];
				}

				if (perm[j] < TI->arity){
					temp_keys.push_back(tmp_str);
				}
			}
			else{
				// numeric value
				fprintf (f, "%.g", TI->cols[j].dval[i]);

				if (perm[j] < TI->arity){
					temp_keys.push_back(numeric_to_string(TI->cols[perm[j]].dval[i]));
				}
			}
			if (j < tcols - 1){
				fprintf (f, "%s", sep);
			}
		}
		fprintf (f, "\n");

		used_keys_map[temp_keys] = i;
	}

	// now we read the rows in the input file and if the keys are not in used_keys_map we write
	// the row to the output file
	while (true) {

		safeGetline(infile, str);

		if (!infile.good()){break;}

		row.clear();
		temp_keys.clear();

		parse_row(str, row);
		row_count += 1;

		if (row.size() != nfields){

			std::cout << str << std::endl;

			log_msg = "Invalid number of fields when reading row ";
			log_msg += numeric_to_string(row_count);
			log_msg += ". Expected ";
			log_msg += numeric_to_string(nfields);
			log_msg += " got ";
			log_msg += numeric_to_string(row.size());
			logger.log(log_msg, LOG_ERROR);
			throw DBE_Error;
		}

		get_keys(row, perm, temp_keys);

		if (used_keys_map.find(temp_keys) == used_keys_map.end()){
			fprintf (f, "%s\n", str.c_str());
		}
		// we could also track duplicate rows here, but we'll assume everything is ok
	}

	infile.close();
	fclose(f);

	// swap temp file for the original one
	remove(filepath.c_str());
	copy_file(new_file, filepath);
	//~ remove(new_file.c_str());
};


void
WriteConnector::generate_file(){

	log_msg = "<generate_file>";
	logger.log(log_msg, LOG_DEBUG);

	std::ofstream o(filepath.c_str());
};


void
WriteConnector::get_keys(
	std::vector<std::string> & row,
	std::vector<int> & perm,
	std::vector<std::string> & keyvec
){
	for (int i=0; i<row.size(); i++){
		if (perm[i] < TI->arity){
			keyvec.push_back(row[i]);
		}
	}
};




