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
	ampl_kargs_map["use_header"] = "true";

	// you can use the values from tha maps directly or convert them to attributes latter
	sep = ",";
	quotechar = "";
	quotestrings = false;
	has_header = true;
	use_header = true;
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
		perm = validate_header(header);
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
		log_msg += " to write data.";
		logger.log(log_msg, LOG_ERROR);
		throw DBE_Error;
	}

	// overwrite data without including header
	if (!has_header){
		write_data_ampl(f);
		fclose(f);
		return;
	}

	std::vector<std::string> header;

	// overwrite data
	if (!use_header){
		header = get_header_ampl();
		write_header(f, header);
		write_data_ampl(f);
		fclose(f);
		return;
	}

	// read header from the data file
	header = get_header_csv();

	// file might be empty (just created)
	if (header.size() == 0){
		header = get_header_ampl();
		write_header(f, header);
		write_data_ampl(f);
		fclose(f);
		return;
	}

	std::vector<int> perm = validate_header(header);

	write_header(f, header);
	write_data_perm(f, perm);
	fclose(f);
};



std::vector<std::string>
Handler::get_header_ampl(){

	log_msg = "<get_header_ampl>";
	logger.log(log_msg, LOG_DEBUG);

	std::vector<std::string> header;
	for (int j = 0; j < ncols(); j++){
		header.push_back(get_col_name(j));
	}
	return header;
};

std::vector<std::string>
Handler::get_header_csv(){

	log_msg = "<get_header_csv>";
	logger.log(log_msg, LOG_DEBUG);

	std::vector<std::string> header;

	std::ifstream infile(filepath.c_str()); // stream to read the data in the csv file

	if (!infile){
		log_msg = "Could not open " + filepath + " to read data.";
		logger.log(log_msg, LOG_ERROR);
		throw DBE_Error;
	}

	std::string str = get_csv_row(infile);

	if (str.size() == 0){
		return header;
	}

	header = parse_row(str);

	infile.close();

	std::cout << header.size() << std::endl;

	return header;
};




void
Handler::write_header(FILE *f, std::vector<std::string>& header){

	log_msg = "<write_header>";
	logger.log(log_msg, LOG_DEBUG);

	for (int j = 0; j < header.size(); j++){

		if (quotestrings){
			ampl_fprintf (f, "%s%s%s", quotechar.c_str(), header[j].c_str(), quotechar.c_str());
		}
		else{
			ampl_fprintf (f, "%s", header[j].c_str());
		}
		// add separator
		if (j < ncols() - 1){
			ampl_fprintf (f, "%s", sep.c_str());
		}
	}
	ampl_fprintf (f, "\n");
};




void
Handler::write_data_ampl(FILE *f){

	log_msg = "<write_data_ampl>";
	logger.log(log_msg, LOG_DEBUG);

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
};










void
Handler::write_data_perm(FILE *f, std::vector<int>& perm){

	log_msg = "<write_data_perm>";
	logger.log(log_msg, LOG_DEBUG);

	for (int i = 0; i < nrows(); i++){

		for (int j = 0; j < perm.size(); j++){

			if (perm[j] != -1){

				int ampl_col = perm[j];

				if (is_char_val(i, ampl_col)){

					if (is_missing(i, j)){}
					else if (quotestrings){
						ampl_fprintf (f, "%s%s%s", quotechar.c_str(), get_char_val(i, ampl_col), quotechar.c_str());
					}
					else{
						ampl_fprintf (f, "%s", get_char_val(i, ampl_col));
					}
				}
				else{
					// numeric value
					ampl_fprintf (f, "%.g", get_numeric_val(i, ampl_col));
				}
			}
			if (j < perm.size() - 1){
				ampl_fprintf (f, "%s", sep.c_str());
			}
		}
		ampl_fprintf (f, "\n");
	}
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

	std::size_t init_nfields = 0; // number of fields (columns) in the csv table
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
		init_nfields = header.size();
		perm = validate_header(header);
		nfields = header.size();
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
		write_header(f, header);
	}

	// write AMPL's table and get a map of used keys
	std::map<std::vector<std::string>,int> used_keys_map;
	std::vector<std::string> temp_keys;

	for (int i = 0; i < nrows(); i++){

		temp_keys.clear();

		for (int j = 0; j < nfields; j++){

			if (perm[j] != -1){

				int ampl_col = perm[j];



				tmp_str.clear();

				if (is_char_val(i, ampl_col)){

					if (get_char_val(i, ampl_col) == TI->Missing){
						continue;
					}
					if (quotestrings){
						ampl_fprintf (f, "%s%s%s", quotechar.c_str(), get_char_val(i, ampl_col), quotechar.c_str());
						tmp_str = quotechar;
						tmp_str += get_char_val(i, ampl_col);
						tmp_str += quotechar;
					}
					else{
						ampl_fprintf (f, "%s", get_char_val(i, ampl_col));
						tmp_str = get_char_val(i, ampl_col);
					}

					if (perm[j] < nkeycols()){
						temp_keys.push_back(tmp_str);
					}
				}
				else{
					// numeric value
					ampl_fprintf (f, "%.g", get_numeric_val(i, ampl_col));

					if (perm[j] < nkeycols()){
						temp_keys.push_back(numeric_to_string(get_numeric_val(i, ampl_col)));
					}
				}



			}

			if (j < nfields - 1){
				ampl_fprintf (f, "%s", sep.c_str());
			}
		}
		ampl_fprintf (f, "\n");
		used_keys_map[temp_keys] = i;
	}



/*
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
*/

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

		if (row.size() != init_nfields){

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

			if (row.size() < nfields){
				for (int i=0; i<nfields-row.size(); i++){
					
					str += sep;
				}
			}

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

	std::cout << "std: " << str.size() << std::endl;

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


std::vector<int>
Handler::validate_header(std::vector<std::string> & header){

	log_msg = "<validate_header>";
	logger.log(log_msg, LOG_DEBUG);

	std::map<std::string, int> csv_col_map;
	std::map<std::string, int> ampl_col_map;
	std::vector<int> perm;

	// get a map for the columns in AMPL's table
	for (int i = 0; i < ncols(); i++){
		std::string temp_str = get_col_name(i);
		ampl_col_map[temp_str] = i;
	}

	perm.resize(header.size());

	// get a map of the columns in the external representation of the table 
	for (std::size_t i = 0; i < header.size(); i++){
		std::string tmp_str = header[i];
		if (quotestrings){
			tmp_str = try_unquote_string(tmp_str, quotechar);
		}

		// log columns that were found by the table handler
		log_msg = "Found column \'";
		log_msg += tmp_str;
		log_msg += "\'";
		logger.log(log_msg, LOG_DEBUG);

		// check for duplicate names
		if (csv_col_map.find(tmp_str) != csv_col_map.end()){
			log_msg = "Duplicate column in the external table.\n";
			log_msg += "Column name \'" + header[i] + "\' at column ";
			log_msg += numeric_to_string(i);
			log_msg += " already defined at column ";
			log_msg += csv_col_map[tmp_str];
			log_msg += ".";
			logger.log(log_msg, LOG_ERROR);
			throw DBE_Error;
		}
		csv_col_map[tmp_str] = i;

		// check if the column is in AMPLs table
		if (ampl_col_map.find(tmp_str) != ampl_col_map.end()){
			perm[i] = ampl_col_map[tmp_str];
		}
		else{
			perm[i] = -1;
			log_msg = "Could not find external column name " + tmp_str + " in AMPL's table.";
			logger.log(log_msg, LOG_DEBUG);
		}
	}

	// confirm all ampl columns are in the external table
	for (int i = 0; i < ncols(); i++){
		std::string temp_str = get_col_name(i);
		if (csv_col_map.find(temp_str) == csv_col_map.end()){
			// If a key column is missing we throw an error
			if (i < nkeycols()){
				log_msg = "Could not find column " + temp_str + " in the external table.";
				logger.log(log_msg, LOG_ERROR);
				throw DBE_Error;
			}
			// Otherwise we add the value column to the external table
			else{
				header.push_back(temp_str);
				perm.push_back(i);
			}
		}
	}
	return perm;
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
		else if (key == "use_header"){
			use_header = get_bool_karg(key);
		}
	}

	log_msg = "sep: " + sep;
	logger.log(log_msg, LOG_DEBUG);
	log_msg = "quotechar: " + ampl_kargs_map["quotechar"];
	logger.log(log_msg, LOG_DEBUG);
	log_msg = "quotestrings: " + ampl_kargs_map["quotestrings"];
	logger.log(log_msg, LOG_DEBUG);
	log_msg = "header: " + ampl_kargs_map["header"];
	logger.log(log_msg, LOG_DEBUG);
	log_msg = "use_header: " + ampl_kargs_map["use_header"];
	logger.log(log_msg, LOG_DEBUG);
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








