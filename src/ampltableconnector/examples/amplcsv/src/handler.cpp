#include "handler.hpp"


static int
Read_AMPLcsv(AmplExports *ae, TableInfo *TI){

	int res = DBE_Done;
	Handler cn;

	try{
		cn.add_ampl_connections(ae, TI);
		cn.prepare();
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
Write_AMPLcsv(AmplExports *ae, TableInfo *TI){

	int res = DBE_Done;
	Handler cn;
	cn.is_writer = true;

	try{
		cn.add_ampl_connections(ae, TI);
		cn.prepare();
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

	// write a description of the handlers
	static char info[] = "amplcsv\n"
		"Write table handler description and help\n";

	// Inform AMPL about the handlers
	ae->Add_table_handler(Read_AMPLcsv, Write_AMPLcsv, info, 0, 0);
};


Handler::Handler(){

	version = "amplcsv - alpha 0.0.0";

	// set the values for ampl_kargs_map as apropriate
	ampl_kargs_map["sep"] = ",";
	ampl_kargs_map["quote"] = "none";
	ampl_kargs_map["header"] = "true";

	// you can use the values from tha maps directly or convert them to attributes latter
	sep = ",";
	quotechar = "";
	quotestrings = false;
	has_header = true;
};


void
Handler::read_in(){

	log_msg = "<read_in>";
	logger.log(log_msg, LOG_DEBUG);

	std::string str; // string to read the information of a single row of the csv file
	std::string tmp_str;
	std::ifstream infile(filepath.c_str()); // stream to read the data from the csv file

	if (!infile){
		log_msg = "Could not open " + filepath + " to read data.";
		logger.log(log_msg, LOG_ERROR);
		throw DBE_Error;
	}

	std::size_t nfields = 0; // number of fields (columns) in the csv table

	std::vector<std::string> header; // vector to store the strings in the csv header
	std::vector<std::string> row; // vector to store the strings in the sucessive csv rows

	// for each column i in the csv perm[i] gives the position of the same column in AMPLs table
	// this allows for the column order to be different in both tables 
	std::vector<int> perm;  

	int row_count = 0;

	// if a header is defined parse the header to get the number of fields (columns) in the csv and
	// and calculate perm to get the correspondence of position of the columns in the external table
	// to AMPLs table. 
	if (has_header){

		str = get_csv_row(infile);
		header = parse_row(str);
		nfields = header.size();
		validate_header(header, perm);
		row_count += 1;
	}
	// otherwise we assume the number of fields equals the number of columns in AMPLs table and give
	// a direct correspondence in perm
	else{
		nfields = ncols();
		for (int i = 0; i < ncols(); i++){
			perm.push_back(i);
		}
	}

	allocate_row_size(nfields);

	// iterate rows of file
	while (true) {

		//~ safeGetline(infile, str);
		str = get_csv_row(infile);

		//~ if (!infile.good()){break;}
		if (str.empty()){break;}

		//~ row.clear();
		row_count += 1;

		//~ row.clear();
		//~ parse_row(str, row);
		row = parse_row(str, nfields);

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

		for (std::size_t j = 0; j < row.size(); j++){
			send_val_to_ampl(row[j], perm[j]);
		}

		add_row();
	}
};


void
Handler::write_out(){

	log_msg = "<write_out>";
	logger.log(log_msg, LOG_DEBUG);

	FILE *f;
	f = fopen(filepath.c_str(), "w");

	if (!f){
		log_msg = "write_out: could not open " + filepath;
		logger.log(log_msg, LOG_ERROR);
		throw DBE_Error;
	}

	// write header
	if (has_header){
		for (int j = 0; j < ncols(); j++){

			if (quotestrings){
				ampl_fprintf (f, "%s%s%s", quotechar.c_str(), get_col_name(j), quotechar.c_str());
			}
			else{
				ampl_fprintf (f, "%s", get_col_name(j));
			}
			// add separator
			if (j < ncols() - 1){
				ampl_fprintf (f, "%s", sep.c_str());
			}
		}
		ampl_fprintf (f, "\n");
	}

	// write data iterating by rows and columns
	for (int i = 0; i < nrows(); i++){
		for (int j = 0; j < ncols(); j++){

			// check if element is a string
			if (is_char_val(i, j)){
				// if value is missing don't write anything
				if (is_missing(i, j)){}
				else if (quotestrings){
					ampl_fprintf (f, "%s%s%s", quotechar.c_str(), get_char_val(i, j), quotechar.c_str());
				}
				else{
					ampl_fprintf (f, "%s", get_char_val(i, j));
				}
			}
			// otherwise element is numeric
			else{
				ampl_fprintf (f, "%.g", get_numeric_val(i, j));
			}
			// add separator
			if (j < ncols() - 1){
				ampl_fprintf (f, "%s", sep.c_str());
			}
		}
		ampl_fprintf (f, "\n");
	}
	fclose(f);
};


void
Handler::write_inout(){

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

	std::size_t nfields = 0; // number of fields (columns) in the csv table

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
		//~ safeGetline(infile, str);
		str = get_csv_row(infile);
		header = parse_row(str);
		nfields = header.size();
		validate_header(header, perm);
		row_count += 1;
	}
	// otherwise we assume the number of fields equals the number of columns in AMPLs table and give
	// a direct correspondence in perm
	else{
		nfields = ncols();
		for (int i = 0; i < ncols(); i++){
			perm.push_back(i);
		}
	}

	// temporary file to write data
	std::string new_file = filepath + ".temp";

	FILE *f;
	f = fopen(new_file.c_str(), "w");

	if (!f){
		log_msg = "write_inout: could not open " + filepath;
		logger.log(log_msg, LOG_ERROR);
		throw DBE_Error;
	}

	// write header
	if (has_header){
		for (int j = 0; j < ncols(); j++){

			if (quotestrings){
				ampl_fprintf (f, "%s%s%s", quotechar.c_str(), get_col_name(j), quotechar.c_str());
			}
			else{
				ampl_fprintf (f, "%s", get_col_name(j));
			}
			// add separator
			if (j < ncols() - 1){
				ampl_fprintf (f, "%s", sep.c_str());
			}
		}
		ampl_fprintf (f, "\n");
	}

	// write AMPL's table and get a map of used keys
	std::map<std::vector<std::string>,int> used_keys_map;
	std::vector<std::string> temp_keys;

	for (int i = 0; i < nrows(); i++){

		temp_keys.clear();

		for (int j = 0; j < ncols(); j++){

			tmp_str.clear();

			if (is_char_val(i, j)){

				if (get_char_val(i, j) == TI->Missing){
					continue;
				}
				if (quotestrings){
					ampl_fprintf (f, "%s%s%s", quotechar.c_str(), get_char_val(i, j), quotechar.c_str());
					tmp_str = quotechar; 
					tmp_str += get_char_val(i, j); 
					tmp_str += quotechar;
				}
				else{
					ampl_fprintf (f, "%s", get_char_val(i, j));
					tmp_str = get_char_val(i, j);
				}

				if (perm[j] < nkeycols()){
					temp_keys.push_back(tmp_str);
				}
			}
			else{
				// numeric value
				ampl_fprintf (f, "%.g", get_numeric_val(i, j));

				if (perm[j] < nkeycols()){
					temp_keys.push_back(numeric_to_string(get_numeric_val(i, j)));
				}
			}
			if (j < ncols() - 1){
				ampl_fprintf (f, "%s", sep.c_str());
			}
		}
		ampl_fprintf (f, "\n");

		used_keys_map[temp_keys] = i;
	}

	// now we read the rows in the input file and if the keys are not in used_keys_map we write
	// the row to the output file
	while (true) {

		//~ safeGetline(infile, str);
		str = get_csv_row(infile);

		if (str.empty()){break;}

		//~ row.clear();
		temp_keys.clear();

		row = parse_row(str);
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
			ampl_fprintf (f, "%s\n", str.c_str());
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
Handler::generate_table(){

	log_msg = "<generate_table>";
	logger.log(log_msg, LOG_DEBUG);

	std::ofstream ofs(filepath.c_str());
};


void
Handler::register_handler_names(){

	log_msg = "<register_handler_names>";
	logger.log(log_msg, LOG_DEBUG);

	handler_names.push_back("amplcsv");
	handler_names.push_back("amplcsv.dll");
};


void
Handler::register_handler_extensions(){

	log_msg = "<register_handler_extensions>";
	logger.log(log_msg, LOG_DEBUG);

	handler_extensions.push_back("csv");
	handler_extensions.push_back("txt");
};


std::vector<std::string>
Handler::parse_row(const std::string & str, int row_size){

	std::vector<std::string> row;
	if (row_size > 0){
		row.reserve(row_size);
	}

	bool is_first = true;
	std::size_t i = 0;
	std::size_t lb = 0;
	std::size_t ub = 0;

	while (true){
		if (str[i] == quotechar[0] && is_first){
			is_first = false;
			lb = i;
			while (true){
				i += 1;
				if (i == str.size()){
					i -= 1;
					break;
				}
				if (str[i] == quotechar[0] && i + 1 < str.size() && str[i + 1] != quotechar[0]){
					ub = i;
					break;
				}
			}
		}
		else if (str[i] == sep[0]){
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
	return row;
};


void
Handler::validate_header(const std::vector<std::string> & header, std::vector<int> & perm){

	std::map<std::string, int> csv_col_map;
	std::map<std::string, int> ampl_col_map;

	// get a map of the columns in the external representation of the table 
	for (std::size_t i = 0; i < header.size(); i++){
		std::string tmp_str = header[i];
		if (quotestrings){
			tmp_str = try_unquote_string(tmp_str, quotechar);
		}
		csv_col_map[tmp_str] = i;
	}

	// confirm all ampl columns are in the external table
	for (int i = 0; i < ncols(); i++){
		std::string temp_str = get_col_name(i);
		if (csv_col_map.find(temp_str) == csv_col_map.end()){
			log_msg = "Could not find column " + temp_str + " in the external table.";
			logger.log(log_msg, LOG_ERROR);
			throw DBE_Error;
		}
	}

	// get a map for the columns in AMPL's table
	for (int i = 0; i < ncols(); i++){
		std::string temp_str = get_col_name(i);
		ampl_col_map[temp_str] = i;
	}

	perm.resize(ncols());

	for (int i = 0; i < ncols(); i++){

		std::string tmp_str = header[i];

		if (quotestrings){
			tmp_str = try_unquote_string(tmp_str, quotechar);
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
Handler::get_keys(
	std::vector<std::string> & row,
	std::vector<int> & perm,
	std::vector<std::string> & keyvec
){
	for (std::size_t i=0; i<row.size(); i++){
		if (perm[i] < nkeycols()){
			keyvec.push_back(row[i]);
		}
	}
};


void
Handler::send_val_to_ampl(std::string val, int col){

	if (val.empty()){
		set_col_missing_val(col);
	}

	char* se;
	double t;

	// check if val is a number
	t = ampl_strtod(val.c_str(), &se);
	if (!*se) {//valid number
		set_col_val(t, col);
	}
	else{
		// check for quoted char
		if (quotestrings){
			val = try_unquote_string(val, quotechar);

			if (val.empty()){
				set_col_missing_val(col);
			}
		}
		set_col_val(val, col);
	}
};


void
Handler::validate_arguments(){

	log_msg = "<validate_arguments>";
	logger.log(log_msg, LOG_DEBUG);

	// check if ampl_args_map was changed by the user
	/*
	for (std::size_t i = 0; i < used_args.size(); i++){
		if (used_args[i] == "something"){
			do something
		}
	}
	*/

	// check if ampl_kargs_map was changed by the user
	std::string key;
	std::vector<std::string> user_options;
	std::vector<std::string> handler_vals;

	for (std::size_t i = 0; i < used_kargs.size(); i++){

		key = used_kargs[i];

		if (key == "sep"){
			user_options.clear();
			handler_vals.clear();

			user_options.push_back(",");
			user_options.push_back(";");
			user_options.push_back("space");
			user_options.push_back("tab");

			handler_vals.push_back(",");
			handler_vals.push_back(";");
			handler_vals.push_back(" ");
			handler_vals.push_back("\t");

			//~ // for recent versions of C++
			//~ user_options = {",", ";", ":", "space", "tab"};
			//~ handler_vals = {",", ";", ":", " ", "\t"};

			sep = get_map_karg(key, user_options, handler_vals);
		}
		else if (key == "quote"){
			user_options.clear();
			handler_vals.clear();

			user_options.push_back("none");
			user_options.push_back("single");
			user_options.push_back("double");

			handler_vals.push_back("");
			handler_vals.push_back("'");
			handler_vals.push_back("\"");

			//~ // for recent versions of C++
			//~ user_options = {"none", "single", "double"};
			//~ handler_vals = {"", "'", "\""};

			quotechar = get_map_karg(key, user_options, handler_vals);

			// if we define quotechar quotestrings is set to true
			if (ampl_kargs_map[key] == "single" || ampl_kargs_map[key] == "double"){
				quotestrings = true;
			}
		}
		else if (key == "header"){
			has_header = get_bool_karg(key);
		}
	}
};


std::string
try_unquote_string(std::string str, const std::string & quotechar){

	size_t n = str.size();
	if (str[0] == quotechar[0] && str.size() > 1 && str[n-1] == quotechar[0]){
		str = str.substr(1, n-2);
	}
	return str;
};


std::istream&
safe_get_line(std::istream& is, std::string& t)
{
	t.clear();

	// The characters in the stream are read one-by-one using a std::streambuf.
	// That is faster than reading them one-by-one using the std::istream.
	// Code that uses streambuf this way must be guarded by a sentry object.
	// The sentry object performs various tasks,
	// such as thread synchronization and updating the stream state.

	std::istream::sentry se(is, true);
	std::streambuf* sb = is.rdbuf();

	// original code for reference
	// for(;;) {
		// int c = sb->sbumpc();
		// switch (c) {
		// case '\n':
			// return is;
		// case '\r':
			// if(sb->sgetc() == '\n')
				// sb->sbumpc();
			// return is;
		// case std::streambuf::traits_type::eof():
			// // Also handle the case when the last line has no line ending
			// if(t.empty())
				// is.setstate(std::ios::eofbit);
			// return is;
		// default:
			// t += (char)c;
		// }
	// }

	for(;;) {
		int c = sb->sbumpc();
		if (c == '\n'){
			return is;
		}
		else if (c == '\r'){
			if(sb->sgetc() == '\n'){
				sb->sbumpc();
			}
			return is;
		}
		else if (c == std::streambuf::traits_type::eof()){
			// Also handle the case when the last line has no line ending
			if(t.empty()){
				is.setstate(std::ios::eofbit);
			}
			return is;
		}
		else{
			t += (char)c;
		}
	}
};


std::string
get_csv_row(std::istream& is)
{
	std::string t;

	// The characters in the stream are read one-by-one using a std::streambuf.
	// That is faster than reading them one-by-one using the std::istream.
	// Code that uses streambuf this way must be guarded by a sentry object.
	// The sentry object performs various tasks,
	// such as thread synchronization and updating the stream state.

	std::istream::sentry se(is, true);
	std::streambuf* sb = is.rdbuf();

	// original code for reference
	// for(;;) {
		// int c = sb->sbumpc();
		// switch (c) {
		// case '\n':
			// return is;
		// case '\r':
			// if(sb->sgetc() == '\n')
				// sb->sbumpc();
			// return is;
		// case std::streambuf::traits_type::eof():
			// // Also handle the case when the last line has no line ending
			// if(t.empty())
				// is.setstate(std::ios::eofbit);
			// return is;
		// default:
			// t += (char)c;
		// }
	// }

	for(;;) {
		int c = sb->sbumpc();
		if (c == '\n'){
			break;
		}
		else if (c == '\r'){
			if(sb->sgetc() == '\n'){
				sb->sbumpc();
			}
			break;
		}
		else if (c == std::streambuf::traits_type::eof()){
			// Also handle the case when the last line has no line ending
			if(t.empty()){
				is.setstate(std::ios::eofbit);
			}
			break;
		}
		else{
			t += (char)c;
		}
	}
	return t;
};








