#include "handler.hpp"

static int
Read_AMPLcsv(AmplExports *ae, TableInfo *TI){

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
Write_AMPLcsv(AmplExports *ae, TableInfo *TI){

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

	// write a description of the handlers
	//~ static char info[] = "amplcsv\n"
		//~ "Write table handler description and help\n";
	//~ static char info[] = {doc};

	// Inform AMPL about the handlers
	//~ ae->Add_table_handler(Read_AMPLcsv, Write_AMPLcsv, info, 0, 0);
	add_table_handler(ae, Read_AMPLcsv, Write_AMPLcsv, const_cast<char *>(doc.c_str()), 0, 0);
};


//~ Handler::Handler(AmplExports *ae, TableInfo *TI){

	//~ handler_version = name + " - " + version;

	//~ sep = ",";
	//~ quotechar = "";
	//~ quotestrings = false;
	//~ has_header = true;
	//~ use_header = true;
//~ };

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

	check_bom(infile);

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
		for (size_t i = 0; i < ncols(); i++){
			perm.push_back(i);
		}
	}

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
			if (perm[j] != -1){
				send_val_to_ampl(row[j], perm[j]);
			}
		}

		add_row();
	}
};


void
Handler::write_out(){

	log_msg = "<write_out>";
	logger.log(log_msg, LOG_DEBUG);

	std::vector<std::string> header;

	// read header before overwritting file
	if (use_header){
		header = get_header_csv();
	}

	FileHandler f = get_file_handler(filepath, "w");

	// overwrite data without including header
	if (!has_header){
		write_data_ampl(f);
		return;
	}

	// overwrite data
	if (!use_header){
		header = get_header_ampl();
		write_header(f, header);
		write_data_ampl(f);
		return;
	}

	// header might be empty (file just created)
	if (header.size() == 0){
		header = get_header_ampl();
		write_header(f, header);
		write_data_ampl(f);
		return;
	}

	std::vector<int> perm = validate_header(header);

	write_header(f, header);
	write_data_perm(f, perm);
};

std::vector<std::string>
Handler::get_header_ampl(){

	log_msg = "<get_header_ampl>";
	logger.log(log_msg, LOG_DEBUG);

	std::vector<std::string> header;
	for (size_t j = 0; j < ncols(); j++){
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

	check_bom(infile);

	std::string str = get_csv_row(infile);

	if (str.size() == 0){
		return header;
	}

	header = parse_row(str);

	infile.close();

	return header;
};




void
Handler::write_header(FileHandler & f, std::vector<std::string> & header){

	log_msg = "<write_header>";
	logger.log(log_msg, LOG_DEBUG);

	for (size_t j = 0; j < header.size(); j++){

		if (quotestrings){
			f.fprintf ("%s%s%s", quotechar.c_str(), header[j].c_str(), quotechar.c_str());
		}
		else{
			f.fprintf ("%s", header[j].c_str());
		}
		// add separator
		if (j < header.size() - 1){
			f.fprintf ("%s", sep.c_str());
		}
	}
	f.fprintf ("\n");
};




void
Handler::write_data_ampl(FileHandler & f){

	log_msg = "<write_data_ampl>";
	logger.log(log_msg, LOG_DEBUG);

	std::clock_t c_start = std::clock();

	// write data iterating by rows and columns
	for (size_t i = 0; i < nrows(); i++){
		for (size_t j = 0; j < ncols(); j++){

			// check if element is a string
			if (is_char_val(i, j)){
				// if value is missing don't write anything
				if (is_missing(i, j)){}
				else if (quotestrings){
					f.fprintf ("%s%s%s", quotechar.c_str(), get_char_val(i, j), quotechar.c_str());
				}
				else{
					f.fprintf ("%s", get_char_val(i, j));
				}
			}
			// otherwise element is numeric
			else{
				f.fprintf ("%.g", get_numeric_val(i, j));
			}
			// add separator
			if (j < ncols() - 1){
				f.fprintf ("%s", sep.c_str());
			}
		}
		f.fprintf ("\n");
	}
	std::clock_t c_end = std::clock();
	double time_elapsed = 1000.0 * (c_end-c_start) / CLOCKS_PER_SEC;
	log_msg = "write_data_ampl done in: " + numeric_to_fixed(time_elapsed / 1000, 3);
	logger.log(log_msg, LOG_DEBUG);
};










void
Handler::write_data_perm(FileHandler & f, std::vector<int>& perm){

	log_msg = "<write_data_perm>";
	logger.log(log_msg, LOG_DEBUG);

	std::clock_t c_start = std::clock();

	for (size_t i = 0; i < nrows(); i++){

		for (size_t j = 0; j < perm.size(); j++){

			if (perm[j] != -1){

				int ampl_col = perm[j];

				if (is_char_val(i, ampl_col)){

					if (is_missing(i, j)){}
					else if (quotestrings){
						f.fprintf ("%s%s%s", quotechar.c_str(), get_char_val(i, ampl_col), quotechar.c_str());
					}
					else{
						f.fprintf ("%s", get_char_val(i, ampl_col));
					}
				}
				else{
					// numeric value
					f.fprintf ("%.g", get_numeric_val(i, ampl_col));
				}
			}
			if (j < perm.size() - 1){
				f.fprintf ("%s", sep.c_str());
			}
		}
		f.fprintf ("\n");
	}
	std::clock_t c_end = std::clock();
	double time_elapsed = 1000.0 * (c_end-c_start) / CLOCKS_PER_SEC;
	log_msg = "write_data_perm done in: " + numeric_to_fixed(time_elapsed / 1000, 3);
	logger.log(log_msg, LOG_DEBUG);
};



void
Handler::write_inout(){

	log_msg = "<write_inout>";
	logger.log(log_msg, LOG_DEBUG);

	std::clock_t c_start = std::clock();

	std::string str; // string to read the information of a single row of the csv file
	std::string tmp_str;
	std::ifstream infile(filepath.c_str()); // stream to read the data in the csv file

	if (!infile){
		log_msg = "Could not open " + filepath + " to read data.";
		logger.log(log_msg, LOG_ERROR);
		throw DBE_Error;
	}

	check_bom(infile);

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
		init_nfields = ncols();
		for (size_t i = 0; i < ncols(); i++){
			perm.push_back(i);
		}
	}

	// temporary file to write data
	std::string new_file = filepath + ".temp";

	FileHandler f = get_file_handler(new_file, "w");

	// write header
	if (has_header){
		write_header(f, header);
	}

	// get a map of used keys
	std::map<std::vector<std::string>, int> used_keys_map = get_used_keys_map(perm);
	// if row from AMPLs table was already processed
	std::vector<bool> rowdone(nrows(), false);

	//~ std::map<std::vector<std::string>, int>::iterator it = used_keys_map.begin();

	//~ while (it != used_keys_map.end()){

		//~ print_vector(it->first);
		//~ std::cout << it->second << std::endl;

		//~ it++;
	//~ }

	// read rows from the csv and check if they are also in AMPLs table

	std::vector<std::string> temp_keys;

	while (true) {

		//~ safeGetline(infile, str);
		std::string str = get_csv_row(infile);

		if (str.empty()){break;}

		//~ row.clear();
		temp_keys.clear();

		std::vector<std::string> row = parse_row(str);
		row_count += 1;

		if (row.size() != init_nfields){

			log_msg = "Invalid number of fields when reading row ";
			log_msg += numeric_to_string(row_count);
			log_msg += ". Expected ";
			log_msg += numeric_to_string(init_nfields);
			log_msg += " got ";
			log_msg += numeric_to_string(row.size());
			logger.log(log_msg, LOG_ERROR);
			throw DBE_Error;
		}

		get_keys(row, perm, temp_keys);

		if (used_keys_map.find(temp_keys) == used_keys_map.end()){

			// the row is only in the csv
			// we check if the row needs new columns and write it back

			if (row.size() < perm.size()){
				for (size_t i = 0; i < perm.size() - row.size(); i++){
					str += sep;
				}
			}
			f.fprintf ("%s\n", str.c_str());
		}
		else{
			// the row is in ampl and in the csv
			// we update the fields and write the data

			int i = used_keys_map[temp_keys];
			rowdone[i] = true;

			for (size_t j = 0; j < perm.size(); j++){

				if (perm[j] != -1){

					int ampl_col = perm[j];

					if (is_char_val(i, ampl_col)){

						if (is_missing(i, j)){}
						else if (quotestrings){
							f.fprintf ("%s%s%s", quotechar.c_str(), get_char_val(i, ampl_col), quotechar.c_str());
						}
						else{
							f.fprintf ("%s", get_char_val(i, ampl_col));
						}
					}
					else{
						// numeric value
						f.fprintf ("%.g", get_numeric_val(i, ampl_col));
					}
				}
				else{
					f.fprintf ("%s", row[j].c_str());
				}
				if (j < perm.size() - 1){
					f.fprintf ("%s", sep.c_str());
				}
			}
			f.fprintf ("\n");
		}
	}

	// write rows that are in AMPL but not in the external table
	for (size_t i = 0; i < nrows(); i++){

		if (!rowdone[i]){
			for (size_t j = 0; j < perm.size(); j++){

				if (perm[j] != -1){

					int ampl_col = perm[j];

					if (is_char_val(i, ampl_col)){

						if (is_missing(i, j)){}
						else if (quotestrings){
							f.fprintf ("%s%s%s", quotechar.c_str(), get_char_val(i, ampl_col), quotechar.c_str());
						}
						else{
							f.fprintf ("%s", get_char_val(i, ampl_col));
						}
					}
					else{
						// numeric value
						f.fprintf ("%.g", get_numeric_val(i, ampl_col));
					}
				}
				if (j < perm.size() - 1){
					f.fprintf ("%s", sep.c_str());
				}
			}
			f.fprintf ("\n");
		}
	}

	infile.close();
	f.close();

	// swap temp file for the original one
	remove(filepath.c_str());
	copy_file(new_file, filepath);
	remove(new_file.c_str());

	std::clock_t c_end = std::clock();
	double time_elapsed = 1000.0 * (c_end-c_start) / CLOCKS_PER_SEC;
	log_msg = "write_inout done in: " + numeric_to_fixed(time_elapsed / 1000, 3);
	logger.log(log_msg, LOG_DEBUG);
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

	handler_names ={"amplcsv", "amplcsv.dll"};
};

void
Handler::register_handler_extensions(){

	log_msg = "<register_handler_extensions>";
	logger.log(log_msg, LOG_DEBUG);

	handler_extensions = {"csv", "txt"};
};

void
Handler::register_handler_args(){

	log_msg = "<register_handler_args>";
	logger.log(log_msg, LOG_DEBUG);

	allowed_args = {"overwrite"};
};

void
Handler::register_handler_kargs(){

	log_msg = "<register_handler_kargs>";
	logger.log(log_msg, LOG_DEBUG);

	allowed_kargs ={"sep", "quote", "header"};
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
		if (i == str.size()){
			ub = i;
			row.push_back(str.substr(lb, ub-lb));
			break;
		}
		else if (str[i] == quotechar[0] && is_first){
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
	for (size_t i = 0; i < ncols(); i++){
		std::string temp_str = get_col_name(i);
		ampl_col_map[temp_str] = i;
	}

	perm.resize(header.size());

	// get a map of the columns in the external representation of the table 
	for (std::size_t i = 0; i < header.size(); i++){
		std::string tmp_str = header[i];
		if (quotestrings){
			tmp_str = try_unquote_string(tmp_str, quotechar);
			header[i] = tmp_str;
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
	for (size_t i = 0; i < ncols(); i++){
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
	//~ for (std::size_t i = 0; i < user_args.size(); i++){
	for (const auto& elem: user_args){
		if (elem == "overwrite"){
			use_header = false;
		}
	}

	// check if ampl_kargs_map was changed by the user
	std::string key;
	std::vector<std::string> user_options;
	std::vector<std::string> handler_vals;

	//~ for (std::size_t i = 0; i < user_kargs.size(); i++){

	for(const auto it: user_kargs){

		key = it.first;

		if (key == "sep"){
			user_options.clear();
			handler_vals.clear();

			user_options = {"comma", "semicolon", "colon", "space", "tab"};
			handler_vals = {",", ";", ":", " ", "\t"};

			sep = get_map_karg(key, user_options, handler_vals);
		}
		else if (key == "quote"){
			user_options.clear();
			handler_vals.clear();

			user_options = {"none", "single", "double"};
			handler_vals = {"", "'", "\""};

			quotechar = get_map_karg(key, user_options, handler_vals);

			// if we define quotechar quotestrings is set to true
			if (user_kargs[key] == "single" || user_kargs[key] == "double"){
				quotestrings = true;
			}
		}
		else if (key == "header"){
			has_header = get_bool_karg(key);

			if (!has_header){
				use_header = false;
			}
		}
		else{ // should never get here
			log_msg = "Discarding argument: " + key + "=";
		logger.log(log_msg, LOG_WARNING);
		}
	}

	if (!has_header){
		log_msg = "no_header";
		logger.log(log_msg, LOG_INFO);
	}

	if (!use_header && has_header){
		log_msg = "overwrite_header";
		logger.log(log_msg, LOG_INFO);
	}

	log_msg = "sep: \'" + sep + "\'";
	logger.log(log_msg, LOG_INFO);

	if (user_kargs["quote"] != "none"){
		log_msg = "quote: \'" + quotechar + "\'";
		logger.log(log_msg, LOG_INFO);
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


std::map<std::vector<std::string>, int>
Handler::get_used_keys_map(std::vector<int>& perm){

	std::string tmp_str;
	std::map<std::vector<std::string>, int> used_keys_map;
	std::vector<std::string> temp_keys;

	for (size_t i = 0; i < nrows(); i++){

		temp_keys.clear();

		for (size_t j = 0; j < perm.size(); j++){

			if (perm[j] != -1){

				int ampl_col = perm[j];

				tmp_str.clear();

				if (is_char_val(i, ampl_col)){

					if (is_missing(i, ampl_col)){
						continue;
					}
					if (quotestrings){
						tmp_str = quotechar;
						tmp_str += get_char_val(i, ampl_col);
						tmp_str += quotechar;
					}
					else{
						tmp_str = get_char_val(i, ampl_col);
					}

					if (perm[j] < nkeycols()){
						temp_keys.push_back(tmp_str);
					}
				}
				else{
					// numeric value
					if (perm[j] < nkeycols()){
						temp_keys.push_back(numeric_to_string(get_numeric_val(i, ampl_col)));
					}
				}
			}
		}
		used_keys_map[temp_keys] = i;
	}
	return used_keys_map;
};


void
Handler::write_remaining_rows(
	std::ifstream & infile,
	std::map<std::vector<std::string>, int> & used_keys_map,
	std::vector<int> & perm,
	FileHandler & f,
	int init_nfields
)
{
	std::vector<std::string> temp_keys;
	int row_count = 0;

	while (true) {

		//~ safeGetline(infile, str);
		std::string str = get_csv_row(infile);

		if (str.empty()){break;}

		//~ row.clear();
		temp_keys.clear();

		std::vector<std::string> row = parse_row(str);
		row_count += 1;

		if (row.size() != init_nfields){

			log_msg = "Invalid number of fields when reading row ";
			log_msg += numeric_to_string(row_count);
			log_msg += ". Expected ";
			log_msg += numeric_to_string(init_nfields);
			log_msg += " got ";
			log_msg += numeric_to_string(row.size());
			logger.log(log_msg, LOG_ERROR);
			throw DBE_Error;
		}

		get_keys(row, perm, temp_keys);

		if (used_keys_map.find(temp_keys) == used_keys_map.end()){

			if (row.size() < perm.size()){
				for (size_t i = 0; i < perm.size() - row.size(); i++){
					str += sep;
				}
			}
			f.fprintf ("%s\n", str.c_str());
		}
		// we could also track duplicate rows here, but we'll assume everything is ok
	}
};

void
Handler::check_bom(std::ifstream & infile){

	char a,b,c;
	a = infile.get();
	b = infile.get();
	c = infile.get();
	if (a != (char)0xEF || b != (char)0xBB || c != (char)0xBF) {
		infile.clear();
		infile.seekg(0, std::ios::beg);
	}
	else {
		log_msg = "BOM found";
		logger.log(log_msg, LOG_INFO);
	}
};





