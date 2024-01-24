#include "ampl_xl.hpp"


static int
Read_ampl_xl(AmplExports *ae, TableInfo *TI){

	ExcelReadManager em;
	em.add_info(ae, TI);
	return em.run();

};


static int
Write_ampl_xl(AmplExports *ae, TableInfo *TI){

	ExcelWriteManager em;
	em.add_info(ae, TI);
	return em.run();
};


void
funcadd(AmplExports *ae){

	/* description of handlers */

	static char info[] = "amplxl\n"
	"Table handler for .xlsx and .xlsm files:\n"
	"one or two strings (an optional 'amplxl' and the file name,\n"
	"ending in \".xlsx\" or \".xlsm\") expected before \":[...]\".";

	/* Inform AMPL about the .example handlers */

	add_table_handler(Read_ampl_xl, Write_ampl_xl, info, 0, 0);


};


ExcelColumnManager::ExcelColumnManager(){

	upcase_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	nchars = 26;
	for (int i=0; i<upcase_chars.size(); i++){
		char_pos[upcase_chars[i]] = i;
	}
};




void
ExcelColumnManager::next(std::string &astring){

	int p = astring.size() - 1;

	while(1){

		char test_char = astring[p];
		int test_char_pos = char_pos[test_char];
		int new_char_index = (test_char_pos + 1) % nchars;
		astring[p] = upcase_chars[new_char_index];

		if (new_char_index == 0){
			p -=1;
		}
		else{
			break;
		}

		if (p == -1){
			astring += "A";
			break;
		}
	}

};



ExcelManager::ExcelManager(){

	range_first_row = -1;
	range_last_row = -1;
	has_range = false;
	break_mode = false;
	verbose = 0;
	write = "drop";
	backup = true;
	is2D = false;
	isReader = true;
	tableType = TABLE_RANGE;
	updateRange = false;
	report_error = true;
};


int
ExcelManager::get_sheet_from_zip(){

	if (data_sheet.size() > 3 && data_sheet.substr(0, 3) == "/xl"){
		excel_iner_file = data_sheet.substr(1, data_sheet.size());
	}
	else{
		excel_iner_file = "xl/" + data_sheet;
	}

	return myunzip(excel_path, excel_iner_file, temp_folder);
};


ExcelWriteManager::ExcelWriteManager(){
	isReader = false;
};


std::string
ExcelManager::numeric_to_scientific(real num){
	// We need 2 extra significant digit to interchange values between AMPL and amplxl without 
	// losing precision. For example, with only digits10 + 1, for the number 1/21 we get in ampl
	// 0.047619047619047616
	// after writing to a spreadsheet and reading back again with amplxl we get
	// 0.04761904761904762
	// We are using sprintf with g format because Excel seems to reject anything with trailling
	// zeros issuing the message "We found a problem with some content in "*.xlsx". Do you want us 
	// to try to recover as much as we can? If you trust the source of this workbook, click yes".
	// This method is not implemented as an independent function because it needs AmplExports to use
	// AMPL's custom sprintf.
	char buf[100];
	int precision = std::numeric_limits<real>::digits10 + 2;
	sprintf(buf, "%.*g", precision, num);
	return std::string(buf);
};


void
ExcelManager::log_table_coords(
	const std::string & first_col,
	const std::string & last_col,
	int first_row,
	int last_row
){
	std::string msg;

	msg = "Estimated table coords: ";
	msg += first_col;
	msg += ", ";
	msg += numeric_to_string(first_row);
	msg += ", ";
	msg += last_col;
	msg += ", ";
	msg += numeric_to_string(last_row);
	logger.log(msg, LOG_DEBUG);
};


void
ExcelManager::log_missing_column(int col){

	std::string msg;
	msg = "Could not find column \'";
	msg += TI->colnames[col];
	msg += "\' in spreadsheet table header";
	logger.log(msg, LOG_ERROR);
};


void
ExcelManager::log_last_row_change(int initial_row, int updated_row){

	std::string msg;
	msg = "table last row changed from ";
	msg += numeric_to_string(initial_row);
	msg += " to ";
	msg += numeric_to_string(updated_row);
	logger.log(msg, LOG_WARNING);
};





int
ExcelManager::add_info(AmplExports *ae, TableInfo *TI){

	this->ae = ae;
	this->TI = TI;

	if (ae == NULL){
		printf("amplxl: could not add ampl exports\n");
		return 1;
	}


	if (TI == NULL){
		printf("amplxl: could not add table info\n");
		return 1;
	}
	logger.add_info(ae, TI);

	return 0;
};


void
ExcelManager::set_logger(Logger & logger){
	logger = logger;
};



int
ExcelManager::prepare(){

	std::string msg;

	// at least the table handler must be declared
	if (TI->nstrings == 0){
		msg = "amplxl: no table handler declared.\n";
		logger.log(msg, LOG_ERROR);
		return DB_Refuse;
	}

	// first string must be the table handler
	if (std::string(TI->strings[0]) != "amplxl"){
		msg = "amplxl: no table handler declared.\n";
		logger.log(msg, LOG_ERROR);
		return DB_Refuse;
	}

	excel_path = get_excel_path(TI);
	table_name = TI->tname;

	std::string arg_string;
	std::string comp_string;
	std::string option_string;
	int has_alias = 0;
	int n = 0;

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
		msg = "unsuported flag";
		logger.log(msg, LOG_ERROR);
		return DB_Error;
	}

	// first string holds table handler name
	// we need to parse remaining ones

	// first search for verbose only
	for (int i = 0; i < TI->nstrings; i++){

		arg_string = TI->strings[i];
		comp_string = "verbose=";

		if (arg_string == "verbose"){
			verbose = 1;
		}
		else if (arg_string.substr(0, comp_string.size()) == comp_string){

			option_string = arg_string.substr(comp_string.size());
			std::istringstream iss(option_string);
			iss >> verbose;
		}
	}

	// set logger level
	if (verbose > 0){
		logger.set_level(verbose);
	}

	msg = "amplxl " + version;
	logger.log(msg, LOG_WARNING);

	if (!excel_path.empty()){
		msg = std::string("file: ") + excel_path;
	}
	msg = std::string("inout: ") + inout;
	logger.log(msg, LOG_INFO);

	msg = std::string("verbose: ") + numeric_to_string(verbose);
	logger.log(msg, LOG_INFO);

	// parse remaining args
	for (int i = 0; i < TI->nstrings; i++){

		arg_string = TI->strings[i];
		std::string extension = get_file_extension(arg_string);
		size_t eq_pos = std::string(TI->strings[i]).find("=");

		// exclude handler
		if (arg_string == "amplxl"){
			continue;
		}
		// exclude oxml file
		else if (extension == "xlsm" || extension == "xlsx"){
			continue;
		}
		// exclude verbose
		else if (arg_string == "verbose"){
			continue;
		}
		// 2D table
		else if (arg_string == "2D"){
			is2D = true;
		}
		// parse args with equal "="
		else if (eq_pos != std::string::npos){

			std::string verbose_op = "verbose=";
			std::string write_op = "write=";
			std::string back_op = "backup=";

			// ignore verbose
			if (arg_string.substr(0, verbose_op.size()) == verbose_op){
				continue;
			}
			// write option
			else if (arg_string.substr(0, write_op.size()) == write_op){

				option_string = arg_string.substr(write_op.size());

				if (option_string == "drop"){
					write = "drop";
				}
				else if (option_string == "append"){
					write = "append";
				}
				else{
					//~ printf("\tamplxl: ignoring write option: %s\n", TI->strings[i]);
					msg = "ignoring write option: ";
					msg += TI->strings[i];
					logger.log(msg, LOG_WARNING);
				}
			}
			// backup option
			else if (arg_string.substr(0, back_op.size()) == back_op){

				option_string = arg_string.substr(back_op.size());

				if (option_string == "true"){
					backup = true;
				}
				else if (option_string == "false"){
					backup = false;
				}
				else{
					//~ printf("\tamplxl: ignoring backup option: %s\n", TI->strings[i]);
					msg = "ignoring backup option: ";
					msg += TI->strings[i];
					logger.log(msg, LOG_WARNING);
				}
			}
		}
		else{

			if (has_alias == 0){
				table_name = TI->strings[i];
				has_alias = 1;
				//~ printf("\tamplxl: using alias: %s\n", TI->strings[i]);
				msg = "using alias: ";
				msg += TI->strings[i];
				logger.log(msg, LOG_WARNING);
			}
			else{
				//~ printf("\tamplxl: ignoring option: %s\n", TI->strings[i]);
				msg = "ignoring option: ";
				msg += TI->strings[i];
				logger.log(msg, LOG_WARNING);
			}
		}
	}

	// no file declared
	if (excel_path.empty()){
		excel_path = table_name + ".xlsx";
	}

	// check file already exists
	if(!check_file_exists(excel_path)){

		//if no file declared write as an out table
		if (inout == "INOUT"){

			if (isReader){
				msg = "Could not find " + excel_path;
				logger.log(msg, LOG_ERROR);
				return DB_Error;
			}
			inout = "OUT";
		}

		// we create the non existing file with the declared name
		if (inout == "OUT"){

			write = "drop";
			msg = "Declared file does not exist. Creating file ";
			msg += excel_path;
			msg += " with sheet ";
			msg += table_name;
			msg += " to write data.";
			logger.log(msg, LOG_WARNING);

			int res = 0;
			res = oxml_build_file2(excel_path, table_name);

			if (res){
				// Failed to build oxml
				msg = "Could not create oxml file, please confirm that the folders to the defined file exist.";
				logger.log(msg, LOG_ERROR);
				return DB_Error;
			}
		}
		// IN file must exist beforehand
		else{
			msg = "Could not find file ";
			msg += excel_path;
			logger.log(msg, LOG_ERROR);
			return DB_Error;
		}
	}
	// file exists
	else{
		if (inout != "IN" && backup){
			// if backup file already exists we do nothing
			// (to avoid issues with multiple writes to the same table)
			std::string backup_path = excel_path + ".amplbak";
			if (!check_file_exists(backup_path)){
				my_copy_file(excel_path, backup_path);
			}
		}
	}

	if (inout != "IN" && inout != "INOUT"){
		msg = "write option: ";
		msg += write;
		logger.log(msg, LOG_INFO);
	}
	if (backup){
		msg = "backup: true";
	}
	else{
		msg = "backup: false";
	}
	logger.log(msg, LOG_INFO);

	return 0;
};

int
ExcelManager::manage_workbook(){

	std::string msg;
	int result = 0;

	msg = "Manage workbook...";
	logger.log(msg, LOG_INFO);

	// extract workbook
	excel_iner_file = "xl/workbook.xml";
	result = myunzip(excel_path, excel_iner_file, temp_folder);

	if (result){
		msg = "Cannot extract workbook. Is the file open in another application?";
		logger.log(msg, LOG_ERROR);
		return 1;
	}

	// get info from workbook
	excel_file = "workbook.xml";
	join_path(temp_folder, excel_file, final_path);
	result = parse_workbook();

	if (result){
		msg = "Cannot parse workbook";
		logger.log(msg, LOG_ERROR);
		return 1;
	}

	if (excel_range.empty()){
		// assume the name of the sheet equals the name of the table
		range_sheet = table_name;
		tableType = TABLE_SHEET;
	}
	else{
		has_range = true;
		result = parse_excel_range();

		if (range_first_row == range_last_row){
			tableType = TABLE_HEADER;
		}

		if (result){
			msg = "Cannot parse range";
			logger.log(msg, LOG_ERROR);
			return 1;
		}
	}

	msg = "Table type: " + numeric_to_string(tableType);
	logger.log(msg, LOG_DEBUG);

	//~ sheet_rel = sheet_rel_map[range_sheet];

	std::map<std::string,std::string>::iterator it = sheet_rel_map.find(range_sheet);
	if (it == sheet_rel_map.end()){
		// cannot find table
		// if inout is OUT we create a new sheet with the table name
		if (!isReader){

			result = oxml_add_new_sheet(excel_path, table_name);

			if (result){
				msg = "cannot add new sheet";
				logger.log(msg, LOG_ERROR);
				return 1;
			}

			// new sheet, we write everything
			inout = "OUT";

			// remove current workbook file
			remove(final_path.c_str());

			// try again
			manage_workbook();
		}
		else{
			msg = "could not find a range or sheet named " + table_name;
			logger.log(msg, LOG_ERROR);
			return 1;
		}
	}
	else{
		sheet_rel = it->second;
	}

	msg = "Manage workbook done!";
	logger.log(msg, LOG_INFO);

	return 0;
};

int
ExcelManager::manage_relations(){

	std::string msg;

	msg = "Manage relations...";
	logger.log(msg, LOG_INFO);

	int result = 0;

	// extract relations
	excel_iner_file = "xl/_rels/workbook.xml.rels";
	result = myunzip(excel_path, excel_iner_file, temp_folder);

	if (result){
		msg = "Could not extract relations.";
		logger.log(msg, LOG_ERROR);
		return 1;
	}

	// parse excel relations to get the actual name of the sheet
	excel_file = "workbook.xml.rels";
	join_path(temp_folder, excel_file, final_path);
	result = get_excel_sheet(final_path);

	if (result){
		msg = "Could not parse relations.";
		logger.log(msg, LOG_ERROR);
		return 1;
	}

	msg = "Manage relations done!";
	logger.log(msg, LOG_INFO);

	return 0;
};

int
ExcelManager::manage_shared_strings(){

	std::string msg;
	msg = "Manage shared strings...";
	logger.log(msg, LOG_INFO);

	int result = 0;

	result = has_shared_strings(excel_path);

	if (result == 0){

		msg = "File has no shared strings table";
		logger.log(msg, LOG_INFO);

		// reading a file without shared strings, probably all are inline
		if (inout == "IN"){
			return 0;
		}
		else{
			msg = "Adding shared strings to file";
			logger.log(msg, LOG_INFO);

			int res = oxml_add_shared_strings(excel_path);

			if (res){
				msg = "Could not add shared strings to file";
				logger.log(msg, LOG_ERROR);
				return 1;
			}
		}
	}
	else if (result == -1){
		msg = "Could not scan shared strings";
		logger.log(msg, LOG_ERROR);
		return 1;
	}

	// extract shared strings
	excel_iner_file = "xl/sharedStrings.xml";
	result = myunzip(excel_path, excel_iner_file, temp_folder);

	if (result){
		msg = "Could not extract shared strings";
		logger.log(msg, LOG_ERROR);
		return 1;
	}

	// load shared strings
	excel_file = "sharedStrings.xml";
	join_path(temp_folder, excel_file, final_path);
	result = get_shared_strings();

	if (result){
		msg = "Could not parse shared strings";
		logger.log(msg, LOG_ERROR);
		return 1;
	}

	msg = "Manage shared strings done!";
	logger.log(msg, LOG_INFO);

	return 0;
};

int
ExcelReadManager::manage_data(){

	std::string msg;

	msg = "Manage data...";
	logger.log(msg, LOG_INFO);

	int result = 0;

	result = get_sheet_from_zip();

	if (result){
		msg = "Could not extract sheet " + excel_iner_file;
		logger.log(msg, LOG_ERROR);
		return 1;
	}

	excel_file = data_sheet.substr(data_sheet.find_last_of("/") + 1); 
	join_path(temp_folder, excel_file, final_path);

	pugi::xml_document doc;
	pugi::xml_node node;
	pugi::xml_parse_result pugi_result;
	pugi::xml_node_iterator it;
	pugi::xml_node row_child;
	pugi::xml_node excel_cell;
	const char* row_attr = "r";

	pugi_result = doc.load_file(final_path.c_str());

	if (!pugi_result){
		msg = "Could not load sheet";
		logger.log(msg, LOG_ERROR);
		return 1;
	}

	node = doc.child("worksheet").child("sheetData");

	// estimate coordinates of table
	int first_row = 1;
	int last_row = EXCEL_MAX_ROWS;
	std::string first_col = "A";
	std::string last_col = "A";

	if (has_range){
		first_row = range_first_row;
		first_col = range_first_col;
	}
	else{
		result = get_table_top_left_coords(node, first_row, first_col);
	}

	// check the headers of the table and adjust last_col
	result = check_columns(node, first_row, first_col, last_col);

	if (result == -2 || result == -3){
		return 1;
	}
	else if (result != -1){
		log_missing_column(result);
		return 1;
	}

	break_mode = true;
	if (has_range){
		if (range_first_row != range_last_row){
			break_mode = false;
			last_row = range_last_row;
		}
	}
	log_table_coords(first_col, last_col, first_row, last_row);

	first_row += 1;
	result = parse_data(node, first_row, last_row, first_col, last_col);

	if (result && report_error){
		msg = "Could not parse data";
		logger.log(msg, LOG_ERROR);
		return 1;
	}
	else if (result){
		msg = "Could not parse data";
		logger.log(msg, LOG_WARNING);
		return 1;
	}

	msg = "Manage data done!";
	logger.log(msg, LOG_INFO);

	return 0;
};

int
ExcelReadManager::run(){

	int result = 0;
	std::string msg;

	result = create_temp_folder();
	if (result){
		return DB_Error;
	}

	result = prepare();
	if (result){
		return result;
	}

	result = manage_workbook();
	if (result){
		return DB_Error;
	}

	result = manage_relations();
	if (result){
		return DB_Error;
	}

	result = manage_shared_strings();
	if (result){
		return DB_Error;
	}

	if (is2D){
		result = manage_data2D();
	}
	else{
		result = manage_data();
	}
	if (result){
		return DB_Error;
	}

	result = clean_temp_folder();
	if (result){
		msg = "Could not clean temp folder";
		logger.log(msg, LOG_INFO);
	}

	msg = "amplxl: all done!";
	logger.log(msg, LOG_WARNING);

	return DB_Done;
};

int
ExcelWriteManager::run(){

	int result = 0;
	std::string msg;

	result = create_temp_folder();
	if (result){
		return DB_Error;
	}

	result = prepare();
	if (result){
		return result;
	}

	result = manage_workbook();
	if (result){
		return DB_Error;
	}

	result = manage_relations();
	if (result){
		return DB_Error;
	}

	result = manage_shared_strings();
	if (result){
		return DB_Error;
	}

	if (!validate_table_utf8_compatible()){
		return DB_Error;
	}

	if (is2D){
		result = manage_data2D();
	}
	else{
		result = manage_data();
	}
	if (result){
		return DB_Error;
	}

	result = clean_temp_folder();
	if (result){
		msg = "Could not clean temp folder";
		logger.log(msg, LOG_INFO);
	}

	msg = "amplxl: all done!";
	logger.log(msg, LOG_WARNING);

	return DB_Done;
};




std::string
get_excel_path(TableInfo *TI){

	std::string temp_string;
	std::string extension;

	for (int i=0; i < TI->nstrings; i++){

		temp_string = std::string(TI->strings[i]);
		extension = get_file_extension(temp_string);

		if (extension == std::string("xlsm") || extension == std::string("xlsx")){
			return temp_string;
		}
	}
	return std::string();
};

std::string
get_file_extension(const std::string& filepath){

	if(filepath.find_last_of(".") != std::string::npos)
		return filepath.substr(filepath.find_last_of(".") + 1);
	return "";
};


void
join_path(
	std::string &temp_folder,
	std::string &excel_file,
	std::string &path
){
	path = temp_folder;

#if defined _WIN32 || defined _WIN64
	path += "\\";
#else
	path += "/";
#endif
	path += excel_file;

}




int
ExcelManager::parse_workbook(){

	std::string msg;

	pugi::xml_document doc;
	pugi::xml_node node;
	pugi::xml_parse_result result;
	pugi::xml_node_iterator it;

	result = doc.load_file(final_path.c_str());

	if (!result){
		msg = "Could not load workbook";
		logger.log(msg, LOG_ERROR);
		return 1;
	}

	// get the excel range of the given name
	node = doc.child("workbook").child("definedNames");

	for (it = node.begin(); it != node.end(); ++it){

		msg = std::string("definedName: ") + it->attribute("name").value();
		logger.log(msg, LOG_DEBUG);

		if (it->attribute("name").value() == table_name){
			excel_range = it->child_value();
		}
	}

	// get the relations of excel sheets
	node = doc.child("workbook").child("sheets");

	for (it = node.begin(); it != node.end(); ++it){

		std::string name = it->attribute("name").value();
		std::string r_id = it->attribute("r:id").value();
		sheet_rel_map[name] = r_id;

		msg = "sheet rel map: " + name + " : " + r_id;
		logger.log(msg, LOG_DEBUG);
	}

	msg = "xl range: " + excel_range;
	logger.log(msg, LOG_DEBUG);

	return 0;
};

int
ExcelManager::parse_excel_range(){

	std::string msg;

	// copy excel_range as it will be modified by strtok
	std::string to_split = excel_range;
	char* pch;

	pch = strtok(&to_split[0u], "!$:;");

	while(pch != NULL){
		split.push_back(pch);
		pch = strtok(NULL, "!$:;");
	}

	try{
		if (split.size() == 3){
			range_sheet = std::string(split[0]);
			range_first_col = std::string(split[1]);
			range_last_col = std::string(split[1]);
			range_first_row = atoi(split[2]);
			range_last_row = atoi(split[2]);
		}
		else if (split.size() == 5){
			range_sheet = std::string(split[0]);
			range_first_col = std::string(split[1]);
			range_last_col = std::string(split[3]);
			range_first_row = atoi(split[2]);
			range_last_row = atoi(split[4]);
		}
		else{
			// range should have 3 or 5 elements
			msg = "Could not parse range ";
			msg += excel_range;
			logger.log(msg, LOG_DEBUG);
			return 1;
		}
	}
	catch(int e){
		// could not convert one of the elements
		msg = "Could not convert range ";
		msg += excel_range;
		logger.log(msg, LOG_DEBUG);
		return 1;
	}

	unquote_string(range_sheet);

	return 0;
};


int
ExcelManager::check_columns(
	const pugi::xml_node &node,
	const int first_row,
	const std::string &first_col,
	std::string &last_col
){
	// get the columns names in the row
	std::map<std::string, std::string> excel_col_map;
	int res = parse_header(first_col, last_col, first_row, node, excel_col_map);

	if (res){
		// error in logger
		return -3;
	}

	if (excel_col_map.size() == 0){
		std::string msg = "Parse header found 0 columns.";
		logger.log(msg, LOG_ERROR);
		return -2;
	}

	// get the spreadsheet columns for the ampl column names
	std::string ampl_col_name;
	const int ampl_ncols = TI->arity + TI->ncols;
	ampl_to_excel_cols.resize(ampl_ncols);

	for (int i = 0; i < ampl_ncols; i++){

		ampl_col_name = TI->colnames[i];

		std::map<std::string,std::string>::iterator it = excel_col_map.find(ampl_col_name);

		if (it != excel_col_map.end()){
			ampl_to_excel_cols[i] = it->second;
		}
		else{
			// if reading missing column generates error
			if (isReader){
				return i;
			}
			// otherwise add column to table
			ecm.next(last_col);
			ampl_to_excel_cols[i] = last_col;
			add_missing_column(node, ampl_col_name, first_row, last_col);
		}
	}
	return -1;
};



int
ExcelManager::add_missing_column(
	pugi::xml_node node,
	const std::string & col_name,
	int row,
	std::string & col
){
	pugi::xml_node xl_row = node.find_child_by_attribute(row_attr, numeric_to_string(row).c_str());
	pugi::xml_node xl_cell = get_xl_cell(xl_row, row, col);

	if (!xl_cell){
		xl_cell = row_insert_cell(xl_row, row, col);
	}

	set_cell_string_value(xl_cell, col_name, row, col);

	if (tableType == TABLE_RANGE || tableType == TABLE_HEADER){
		range_last_col = col;
		updateRange = true;
	}

	return 0;
};





int
ExcelManager::get_excel_sheet(std::string &path){

	std::string msg;

	pugi::xml_document doc;
	pugi::xml_node node;
	pugi::xml_parse_result result;
	pugi::xml_node_iterator it;

	result = doc.load_file(path.c_str());

	if (!result){
		msg = "Could not load sheet";
		logger.log(msg, LOG_ERROR);
		return 1;
	}

	node = doc.child("Relationships");

	for (it = node.begin(); it != node.end(); ++it){

		std::string name = it->attribute("Id").value();

		if (name == sheet_rel){

			data_sheet = it->attribute("Target").value();
		}
	}

	return 0;
};


int
ExcelManager::get_shared_strings(){

	std::string msg;

	pugi::xml_document doc;
	pugi::xml_node node;
	pugi::xml_parse_result result;
	pugi::xml_node_iterator it;

	result = doc.load_file(final_path.c_str());

	if (!result){
		msg = "Could not load shared strings";
		logger.log(msg, LOG_ERROR);
		return 1;
	}

	node = doc.child("sst");

	for (it = node.begin(); it != node.end(); ++it){

		shared_strings.push_back(it->first_child().child_value());
	}

	return 0;
};


void
inspect_ti(AmplExports *ae, TableInfo *TI){

	printf("<inspect TI:>\n");

	char* tempchar;

	if (TI->tname != NULL){
		printf("\ttname: %s", TI->tname);
	}
	printf("\tnstrings: %d\n", TI->nstrings);
	printf("\tstrings:\n");
	for (int i=0; i<TI->nstrings; i++){
		printf("\t\t%s\n", TI->strings[i]);
	}
	printf("\tarity: %d\n", TI->arity);
	printf("\tncols: %d\n", TI->ncols);
	printf("\tcolnames:\n");
	for (int i=0; i<TI->arity + TI->ncols; i++){
		printf("\t\t%s\n", TI->colnames[i]);
	}
	if (TI->Missing != NULL){
		printf("Missing: %s\n", TI->Missing);
	}
	if (TI->Errmsg != NULL){
		printf("\tErrmsg: %s\n", TI->Errmsg);
	}
	printf("flags: %d\n", TI->flags);
	printf("flags_bit IN: %d\n", TI->flags & DBTI_flags_IN);
	printf("flags_bit OUT: %d\n", TI->flags & DBTI_flags_OUT);
	printf("flags_bit INSET: %d\n", TI->flags & DBTI_flags_INSET);
	printf("nrows: %d\n", TI->nrows);
	printf("maxrows: %d\n", TI->maxrows);

	printf("<inspect TI done>\n");
};


int
ExcelManager::parse_data(
	const pugi::xml_node &node,
	const int first_row,
	const int last_row,
	const std::string first_col,
	const std::string last_col
){
	std::string msg;

	msg = "Parse data...";
	logger.log(msg, LOG_INFO);

	std::clock_t start_time = get_time();

	int ampl_ncols = TI->ncols + TI->arity;
	DbCol *db;

	std::string iter_col = first_col;
	std::vector<std::string> temp_strings(ampl_ncols);
	std::vector<int> is_string(ampl_ncols);

	pugi::xml_node excel_cell;
	pugi::xml_node row_child;

	char* se;
	double t;
	const char* row_attr = "r";

	std::stringstream strs;
	std::string row_id_str;
	char* row_id;

	//~ row_id = &std::to_string(i)[0u];
	strs.str(std::string()); // clear stringstream
	strs << first_row;
	row_id_str = strs.str();
	row_id = &row_id_str[0u];

	row_child = node.find_child_by_attribute(row_attr, row_id);

	// iterate by rows of excel range
	for (int i = first_row; i <= last_row; i++){

		//~ row_id = &std::to_string(i)[0u];
		strs.str(std::string()); // clear stringstream
		strs << i;
		row_id_str = strs.str();
		row_id = &row_id_str[0u];

		if (row_child.attribute(row_attr).value() != row_id_str){

			row_child = node.find_child_by_attribute(row_attr, row_id);
		}

		if (break_mode && !row_child){
			break;
		}

		for (int j = 0; j < temp_strings.size(); j++){
			temp_strings[j].clear();
			is_string[j] = 0;
		}

		bool has_content = false;

		for(int j = 0; j < ampl_ncols; j++){

			iter_col = ampl_to_excel_cols[j];

			// concatenate iter_col and i to get the excel cell information
			std::string cell_adress = iter_col + row_id_str;

			// get the element
			excel_cell = row_child.find_child_by_attribute(row_attr, cell_adress.c_str());

			if (excel_cell){

				// check if the cell holds a string or a numeric value
				// if the node has a "t" attribute with value "s" it's child node holds the position of a shared string
				// if the attribute is not present the child holds a numeric value

				std::string value = excel_cell.child("v").child_value();

				if (excel_cell.attribute("t").value() == std::string("s")){
					value = shared_strings[std::atoi(value.c_str())];
					is_string[j] = 1;
				}
				else if (excel_cell.attribute("t").value() == std::string("inlineStr")){
					value = excel_cell.child("is").child("t").child_value();
					is_string[j] = 1;
				}

				if (value.length() > 0){
					temp_strings[j] = value;
					has_content = true;
				}
				else{
					msg = "empty value for cell " + cell_adress;
					logger.log(msg, LOG_DEBUG);
				}
			}

			ecm.next(iter_col);
		}

		if (!has_content){
			break;
		}

		db = TI->cols;
		for (int j=0; j<TI->ncols + TI->arity; j++){

			if (temp_strings[j].empty()){
				if (j < TI->arity){
					msg = "Missing value for key column ";
					msg += TI->colnames[j];
					msg += " at row ";
					msg += row_id_str;
					msg += " in range/sheet ";
					msg += table_name;
					logger.log(msg, LOG_ERROR);
					report_error = false;
					return DB_Error;
				}
				else{
					db->sval[0] = TI->Missing;
				}
			}
			else{
				if (is_string[j]){
					db->sval[0] = &temp_strings[j][0u];
				}
				else{
					//dont trust excel, always try to convert the value
					t = strtod(temp_strings[j].c_str(), &se);
					if (!*se) {/* valid number */
						db->sval[0] = 0;
						db->dval[0] = t;
						//~ std::cout << "assigning numeric: " << db->dval[0] << std::endl;
					}
					else{
						db->sval[0] = &temp_strings[j][0u];
						//~ std::cout << "assigning string: " << db->sval[0] << std::endl;
					}
				}
			}
			db++;
		}

		db = TI->cols;
		if ((*TI->AddRows)(TI, db, 1)){
			msg = "Error with AddRows";
			logger.log(msg, LOG_WARNING);
			report_error = false;
			return DB_Error;
		}

		row_child = row_child.next_sibling();

	}

	std::clock_t end_time = get_time();
	double total_time = clock_to_seconds(start_time, end_time);

	msg = std::string("Parse data done in ") +  numeric_to_fixed(total_time, CPUTIMES_NDIGITS) + std::string(" seconds");
	logger.log(msg, LOG_INFO);

	return 0;
};



void
ExcelManager::get_sstrings_map(){

	for (int i=0; i< shared_strings.size(); i++){
		sstrings_map[shared_strings[i]] = i;
	}

};


int
ExcelWriteManager::manage_data(){

	std::string msg;

	msg = "Manage data...";
	logger.log(msg, LOG_INFO);

	int result = 0;

	result = get_sheet_from_zip();

	if (result){
		msg = "Could not extract sheet " + excel_iner_file;
		logger.log(msg, LOG_ERROR);
		return 1;
	}

	excel_file = data_sheet.substr(data_sheet.find_last_of("/") + 1); 
	join_path(temp_folder, excel_file, final_path);

	std::string sheet_final_path = final_path;

	pugi::xml_document doc;
	pugi::xml_node node;
	pugi::xml_parse_result pugi_result;
	pugi::xml_node_iterator it;
	pugi::xml_node row_child;
	pugi::xml_node excel_cell;
	const char* row_attr = "r";

	pugi_result = doc.load_file(final_path.c_str());


	if (!pugi_result){
		msg = "Could not load sheet";
		logger.log(msg, LOG_ERROR);
		return 1;
	}

	node = doc.child("worksheet").child("sheetData");

	// get table dimensions
	int first_row = 1;
	int last_row = EXCEL_MAX_ROWS;
	std::string first_col = std::string("A");
	std::string last_col = EXCEL_MAX_COLS;

	if (has_range){
		first_row = range_first_row;
		first_col = range_first_col; 
		last_col = range_last_col; 
	}

	if (has_range){
		if (range_first_row != range_last_row){
			last_row = range_last_row;
		}
		else{
			last_row = get_last_row_in_table(node, first_row, first_col);
			//~ last_row = first_row + TI->nrows;
		}
	}
	else{
		result = get_table_top_left_coords(node, first_row, first_col);

		if (result){
			return 1;
		}

		result = get_last_column_in_table(node, first_row, first_col, last_col);

		if (first_col == "A" && last_col == EXCEL_MAX_COLS){
			last_col = number_to_cell_reference(TI->arity + TI->ncols);
		}

		last_row = get_last_row_in_table(node, first_row, first_col);
		//~ last_row = first_row + TI->nrows;
	}

	log_table_coords(first_col, last_col, first_row, last_row);

	// map shared strings for fast access and get the number of existing strings, since we may add
	// more strings later and need to update the file in excel
	get_sstrings_map();
	int n_sstrings = shared_strings.size();

	if (inout == "OUT"){

		if (write == "drop"){

			// At this point we have an estimate of the table dimensions.
			// However, due to the dynamic nature of data, this estimate may not be correct.
			// We compare dimensions and increase/reduce the number of rows accordingly

			int n_xl_rows = last_row - first_row; // still including the header so we dont need the +1
			int n_diff_rows = abs(TI->nrows - n_xl_rows);

			if (n_xl_rows > TI->nrows){
				// need to shrink
				delete_range_values(node, last_row - n_diff_rows + 1, last_row, first_col, last_col);
				log_last_row_change(last_row, last_row - n_diff_rows);
				last_row -= n_diff_rows;
			}
			else if (n_xl_rows < TI->nrows){
				// increase number of rows
				log_last_row_change(last_row, last_row + n_diff_rows);

				last_row += n_diff_rows;
			}

			result = check_columns(node, first_row, first_col, last_col);

			if (result == -2){ // no header declared
				result = write_all_data_out(node, first_row, last_row, first_col, last_col);
			}
			else if (result != -1){ // incomplete header
				log_missing_column(result);
				return 1;
			}
			else{
				first_row += 1; // advance header
				result = write_data_out(node, first_row, last_row, first_col, last_col);
			}
		}
		else if (write == "append"){

			result = check_columns(node, first_row, first_col, last_col);

			if (result == -2){ // append to non existing table ???
				msg = "Cannot append to non existing table.";
				logger.log(msg, LOG_ERROR);
				return 1;
			}
			else if (result != -1){
				log_missing_column(result);
				return 1;
			}
			else{
				first_row = last_row + 1;
				last_row = first_row + TI->nrows - 1;
				result = write_data_out(node, first_row, last_row, first_col, last_col);
			}
		}
	}
	else if (inout == "INOUT"){

		result = check_columns(node, first_row, first_col, last_col);

		if (result == -2){
			// the table is defined by a named range or sheet name but is empty
			result = write_all_data_out(node, first_row, last_row, first_col, last_col);
		}
		else if (result != -1){
			log_missing_column(result);
			return 1;
		}
		else{
			first_row += 1;
			result = write_data_inout(node, first_row, last_row, first_col, last_col);
		}
	}
	else{
		// unsuported write flag
		msg = "unsuported write flag ";
		msg += write;
		logger.log(msg, LOG_ERROR);
		return 1;
	}

	if (result){
		msg = "Could not write data";
		logger.log(msg, LOG_ERROR);
		return 1;
	}

	// vector with the names of the files we need to update in the original spreadsheet
	std::vector<std::string> changed_files;


	if (updateRange){
		changed_files.push_back("xl/workbook.xml");
	}



	// update sheet xml
	excel_iner_file = "xl/" + data_sheet;
	excel_file = data_sheet.substr(data_sheet.find("/") + 1); 
	join_path(temp_folder, excel_file, final_path);

	doc.save_file(final_path.c_str());

	changed_files.push_back(excel_iner_file);

	// update shared strings xml
	if (shared_strings.size() > n_sstrings){

		excel_iner_file = "xl/sharedStrings.xml";
		changed_files.push_back(excel_iner_file);

		result = update_shared_strings(n_sstrings);
		if (result){
			msg = "Could not update shared strings";
			logger.log(msg, LOG_ERROR);
			return 1;
		}
	}

	std::string xl_copy_path = temp_folder;

#if defined _WIN32 || defined _WIN64
	xl_copy_path += "\\";
#else
	xl_copy_path += "/";
#endif

	xl_copy_path += "xlcopy.tmp";

	result = copy_uchanged_files(excel_path, xl_copy_path, changed_files);

	// add changed files to zip
	excel_iner_file = "xl/" + data_sheet;
	excel_file = data_sheet.substr(data_sheet.find("/") + 1); 
	join_path(temp_folder, excel_file, final_path);

	result = myzip(xl_copy_path, excel_iner_file, final_path);
	if (result){
		msg = "Could not add sheet";
		logger.log(msg, LOG_ERROR);
		return 1;
	}

	if (shared_strings.size() > n_sstrings){

		excel_iner_file = "xl/sharedStrings.xml";
		excel_file = "sharedStrings.xml";

		join_path(temp_folder, excel_file, final_path);

		result = myzip(xl_copy_path, excel_iner_file, final_path);
		if (result){
			msg = "Could not add shared strings";
			logger.log(msg, LOG_ERROR);
			return 1;
		}
	}

	if (updateRange){
		update_workbook(xl_copy_path);
	}


	// remove initial file
	result = remove(excel_path.c_str());

	// replace it by modified copy
	// cannot use rename due to issue with files in different partitions
	my_copy_file(xl_copy_path, excel_path);

	// remove
	result = remove(xl_copy_path.c_str());

	msg = "Manage data done!";
	logger.log(msg, LOG_INFO);

	return 0;
};



int check_rows(pugi::xml_node node, int first_row, int last_row){

	int has_all_rows = 1;
	pugi::xml_node child_node;

	for (int i = first_row; i <= last_row; i++){

		child_node = get_excel_row(node, i);
		if (!child_node){

			pugi::xml_node row_child = node.append_child("row");
			row_child.append_attribute("r") = i;
			has_all_rows = 0;
		}
	}

	return has_all_rows; 
};



int
ExcelWriteManager::write_data_out(
	pugi::xml_node node,
	int first_row,
	int last_row,
	std::string &first_col,
	std::string &last_col
){
	std::string msg;

	msg = "Write data out...";
	logger.log(msg, LOG_INFO);

	std::clock_t start_time = get_time();

	// map rows and cells for faster access
	std::map<std::string, pugi::xml_node> row_map;
	std::map<std::string, pugi::xml_node> cell_map;
	get_maps(node, row_map, cell_map, logger);

	// check that all required cells exist
	check_table_cells(
		node,
		row_map,
		cell_map,
		first_row,
		last_row,
		first_col,
		last_col,
		logger
	);

	// write data
	const int ampl_ncols = TI->arity + TI->ncols;
	DbCol *db;

	int xl_row = first_row;
	for (int i = 0; i < TI->nrows; i++){

		db = TI->cols;

		for (int j = 0; j < ampl_ncols; j++){

			std::string cell_col = ampl_to_excel_cols[j];
			std::string cell_row = numeric_to_string(xl_row);
			std::string cell_reference = cell_col + cell_row;
			pugi::xml_node write_cell = cell_map[cell_reference];
			set_cell_value(db, i, write_cell);
			db++;
		}
		xl_row += 1;
	}

	if (tableType == TABLE_RANGE){
		check_range_update(xl_row - 1);
	}

	std::clock_t end_time = get_time();
	double total_time = clock_to_seconds(start_time, end_time);

	msg = std::string("Write data out done in ") +  numeric_to_fixed(total_time, CPUTIMES_NDIGITS) + std::string(" seconds");
	logger.log(msg, LOG_INFO);

	return 0;
};

void
ExcelWriteManager::check_range_update(int last_row){

	if (last_row != range_last_row){
		range_last_row = last_row;
		updateRange = true;
		std::string msg = "named range update required";
		logger.log(msg, LOG_DEBUG);
	}
};


int
ExcelWriteManager::write_all_data_out(
	pugi::xml_node node,
	int first_row,
	int last_row,
	std::string &first_col,
	std::string &last_col
){
	std::string msg;

	msg = "Write data out...";
	logger.log(msg, LOG_INFO);

	std::clock_t start_time = get_time();

	pugi::xml_node excel_row;
	pugi::xml_node excel_cell;
	pugi::xml_node excel_val;
	pugi::xml_node dnode;

	std::stringstream strs;
	std::string row_id_str;

	// map rows and cells for faster access
	std::map<std::string, pugi::xml_node> row_map;
	std::map<std::string, pugi::xml_node> cell_map;
	get_maps(node, row_map, cell_map, logger);

	// check that all required cells exist
	check_table_cells(
		node,
		row_map,
		cell_map,
		first_row,
		last_row,
		first_col,
		last_col,
		logger
	);

	write_header(node, first_row, first_col, cell_map);

	first_row += 1; // advance header

	// write data
	const int ampl_ncols = TI->arity + TI->ncols;
	DbCol *db;
	std::string iter_col;

	int trow = 0;
	for (int i = first_row; i <= last_row; i++){

		db = TI->cols;
		iter_col = first_col;

		for (int j = 0; j < ampl_ncols; j++){

			std::string cell_col = iter_col;
			std::string cell_row = numeric_to_string(i);
			std::string cell_reference = cell_col + cell_row;
			pugi::xml_node write_cell = cell_map[cell_reference];
			set_cell_value(db, trow, write_cell);
			db++;
			ecm.next(iter_col);
		}
		trow += 1;
	}

	if (tableType == TABLE_RANGE){
		check_range_update(trow - 1);
	}

	std::clock_t end_time = get_time();
	double total_time = clock_to_seconds(start_time, end_time);

	msg = std::string("Write data out done in ") +  numeric_to_fixed(total_time, CPUTIMES_NDIGITS) + std::string(" seconds");
	logger.log(msg, LOG_INFO);


	return 0;


	/*
	pugi::xml_node excel_row;
	pugi::xml_node excel_cell;
	pugi::xml_node excel_val;
	pugi::xml_node dnode;

	const char* row_attr = "r";
	std::stringstream strs;
	std::string row_id_str;
	std::string iter_col = first_col;

	const int ampl_ncols = TI->arity + TI->ncols;
	DbCol *db;

	int trow = 0;
	excel_row = get_excel_row(node, first_row);

	for (int i = first_row; i <= last_row; i++){

		strs.str(std::string()); // clear stringstream
		strs << i;
		row_id_str = strs.str();

		if (excel_row.attribute(row_attr).value() != row_id_str){
			excel_row = get_excel_row(node, i);
		}

		db = TI->cols;
		iter_col = first_col;

		// we have the row now we need to check if it has the column nodes
		for (int j = 0; j < ampl_ncols; j++){

			excel_cell = get_xl_cell(excel_row, i, iter_col);
			set_cell_value(db, trow, excel_cell);

			db++;
			ecm.next(iter_col);
		}
		trow += 1;
		excel_row = excel_row.next_sibling();

	}
	return 0;
	*/
};


int
ExcelWriteManager::get_excel_keys(pugi::xml_node excel_row, int row){

	std::string msg;

	for (int i = 0; i < excel_keys.size(); i++){
		excel_keys[i].clear();
	}

	int nkeys = TI->arity;

	for (int i = 0; i < nkeys; i++){

		std::string scol = ampl_to_excel_cols[i];
		pugi::xml_node excel_cell = get_xl_cell(excel_row, row, scol);

		if (!excel_cell){
			// could not find key
			msg = "Could not get spreadsheet keys from row";
			logger.log(msg, LOG_ERROR);
			return 1;
		}

		std::string value = excel_cell.child("v").child_value();

		if (excel_cell.attribute("t").value() == std::string("s")){
			value = shared_strings[std::atoi(value.c_str())];
		}
		else{
			// if the value is numeric its string representation might be different from amplxl
			// default(scientific) so we turn it to number and back to string for it to have the
			// same representation in the keys of a map.
			// Otherwise we could interpret same numbers as different and add unwanted
			// entries to inout tables

			char* se;
			double t;

			t = strtod(value.c_str(), &se);
			if (!*se) {/* valid number */
				value = numeric_to_scientific(t);
				// add prefix to avoid clash with strings with the same numeric value
				value = "ampl-numeric-" + value;
			}
			else{
				// could not convert number to numeric value
				msg = "Could not convert " + value + " to numeric";
				logger.log(msg, LOG_ERROR);
				return 1;
			}

		}



		//~ std::cout << "string value: " << value << std::endl;

		if (value.length() > 0){
			excel_keys[i] = value;
		}
		else{
			// no value found
			msg = "Could not get spreadsheet keys";
			logger.log(msg, LOG_ERROR);
			return 1;
		}
	}

	//~ if (verbose == 73){
		//~ printf("excel_keys = [");
		//~ for (int i = 0; i < nkeys; i++){
			//~ printf("%s, ", excel_keys[i].c_str());
		//~ }
		//~ printf("]\n");
	//~ }


	//~ print_vector(excel_keys);

	return 0;
};


int
ExcelWriteManager::get_ampl_keys(int line){

	DbCol *db;

	for (int i = 0; i < ampl_keys.size(); i++){
		ampl_keys[i].clear();
	}

	int nkeys = TI->arity;
	db = TI->cols;

	for (int i = 0; i < nkeys; i++){

		if (db->sval && db->sval[line]){
			ampl_keys[i] = std::string(db->sval[line]);
		}
		else{
			std::string value = numeric_to_scientific(db->dval[line]);
			// add prefix to avoid clash with strings with the same numeric value
			value = "ampl-numeric-" + value;
			ampl_keys[i] = value;
		}
		db++;
	}
	return 0;
};


int
ExcelWriteManager::write_data_inout(

	pugi::xml_node node,
	int first_row,
	int last_row,
	std::string &first_col,
	std::string &last_col
){
	std::string msg;

	msg = "Write data inout...";
	logger.log(msg, LOG_INFO);

	std::clock_t start_time = get_time();

	pugi::xml_node excel_row;
	pugi::xml_node excel_cell;
	pugi::xml_node excel_val;
	pugi::xml_node dnode;

	const char* row_attr = "r";
	std::stringstream strs;
	std::string row_id_str;

	const int ampl_ncols = TI->arity + TI->ncols;

	int nkeys = TI->arity;

	excel_keys.resize(nkeys);
	ampl_keys.resize(nkeys);

	// map xl rows according to arity keys and get last row of xl table
	// mapping only stops when we cannot find keys (assume blank row)
	std::map<std::vector<std::string>, pugi::xml_node> xl_key_map;
	int xl_table_last_row = first_row;
	excel_row = get_excel_row(node, first_row);
	pugi::xml_node pg_table_last_row = excel_row;

	//~ std::cout << "get excel keys" << std::endl;

	for (int i = first_row; i <= EXCEL_MAX_ROWS; i++){

		row_id_str = numeric_to_string(i);

		if (excel_row.attribute(row_attr).value() != row_id_str){
			excel_row = get_excel_row(node, i);
		}

		int res = get_excel_keys(excel_row, i);

		if (res){
			// we could not find all the keys, assume end of table
			break;
		}

		//~ print_vector(excel_keys);

		xl_key_map[excel_keys] = excel_row;
		xl_table_last_row = i;
		pg_table_last_row = excel_row;

		excel_row = excel_row.next_sibling();
	}

	//~ std::cout << std::endl;

	// iterate AMPL table and write data to spreadsheet
	for (int i = 0; i < TI->nrows; i++){

		get_ampl_keys(i);

		// get the corresponding row in xl table
		pugi::xml_node row_to_write;

		std::map<std::vector<std::string>, pugi::xml_node>::iterator it = xl_key_map.find(ampl_keys);
		if (it == xl_key_map.end()){

			//~ std::cout << "could not find keys" << std::endl;
			//~ print_vector(ampl_keys);
			//~ return 1;

			// row is not mapped, append to table
			xl_table_last_row += 1;

			// check row already exists
			row_to_write = get_excel_row(node, xl_table_last_row);

			if (!row_to_write){
				row_to_write = node.insert_child_after("row", pg_table_last_row);
				row_to_write.append_attribute("r") = numeric_to_string(xl_table_last_row).c_str();
				pg_table_last_row = row_to_write;
			}

			// write cells of arity columns as the new/append row does not have it
			write_arity_cells(row_to_write, xl_table_last_row, i);
		}
		else{
			row_to_write = it->second;
		}

		// write info
		copy_info(row_to_write, atoi(row_to_write.attribute("r").value()), i);
	}

	if (tableType == TABLE_RANGE){
		check_range_update(xl_table_last_row);
	}

	std::clock_t end_time = get_time();
	double total_time = clock_to_seconds(start_time, end_time);

	msg = std::string("Write data inout done in ") +  numeric_to_fixed(total_time, CPUTIMES_NDIGITS) + std::string(" seconds");
	logger.log(msg, LOG_INFO);

	return 0;
};


int
ExcelWriteManager::get_ampl_row(){

	for (int i = 0; i < TI->nrows; i++){

		get_ampl_keys(i);

		if (excel_keys == ampl_keys){
			return i;
		}
	}
	return -1;
};


int
ExcelWriteManager::copy_info(pugi::xml_node excel_row, int row, int ampl_row){

	DbCol *db;

	db = &TI->cols[TI->arity];

	for (int i = TI->arity; i < TI->arity + TI->ncols; i++){

		std::string scol = ampl_to_excel_cols[i];
		pugi::xml_node excel_cell = get_xl_cell(excel_row, row, scol);

		if (!excel_cell){
			excel_cell = row_insert_cell(excel_row, row, scol);
		}

		set_cell_value(db, ampl_row, excel_cell);

		db++;
	}
	return 0;
};

void
ExcelWriteManager::write_arity_cells(pugi::xml_node row_node, int xl_row, int db_row){

	DbCol *db = TI->cols;

	for (int i = 0; i < TI->arity; i++){

		std::string xl_col = ampl_to_excel_cols[i];
		pugi::xml_node xl_cell = get_xl_cell(row_node, xl_row, xl_col);

		if (!xl_cell){
			xl_cell = row_insert_cell(row_node, xl_row, xl_col);
		}

		set_cell_value(db, db_row, xl_cell);
		db++;
	}

};




void inspect_values(AmplExports *ae, TableInfo *TI){

	printf("<Inspect values>\n");

	DbCol *db;
	const int ampl_ncols = TI->arity + TI->ncols;

	for (int j=0; j<TI->nrows; j++){
		db = TI->cols;
		for (int i=0; i<ampl_ncols; i++){

			if (db->sval && db->sval[j]){
				printf("%s", db->sval[j]);
			}
			else{
				printf("%.5f", db->dval[j]);
			}
			printf("\t");
			db++;
		}
		printf("\n");
	}
	printf("<Inspect values done>\n");
};





pugi::xml_node
get_excel_row(pugi::xml_node parent, int row){

	const char* row_attr = "r";
	std::stringstream strs;
	strs << row;
	std::string row_id_str = strs.str();
	char* row_id = &row_id_str[0u];

	return parent.find_child_by_attribute(row_attr, row_id);
};

pugi::xml_node
get_xl_cell(pugi::xml_node parent, int row, std::string &col){

	const char* row_attr = "r";
	std::stringstream strs;
	strs << row;
	std::string row_id_str = strs.str();

	std::string cell_id_str = col + row_id_str;
	char* cell_id = &cell_id_str[0u];

	return parent.find_child_by_attribute(row_attr, cell_id);
};



int
ExcelManager::check_shared_strings(const std::string & s){

	std::map<std::string,int>::iterator it = sstrings_map.find(s);
	if (it == sstrings_map.end()){
		// need to add string
		int pos = shared_strings.size();
		shared_strings.push_back(s);
		sstrings_map[s] = pos;
		return pos;
	}
	else{
		return it->second;
	}
};


int
ExcelWriteManager::update_shared_strings(int init_size){

	int res = 0;

	// reopen shared strings
	//~ excel_iner_file = "xl/sharedStrings.xml";
	//~ res = myunzip(&excel_path[0u], &excel_iner_file[0u], &temp_folder[0u]);

	//~ if (res){
		// error extracting shared strings
		//~ return 1;
	//~ }

	// load shared strings file
	excel_file = "sharedStrings.xml";
	join_path(temp_folder, excel_file, final_path);

	pugi::xml_document doc;
	pugi::xml_node node;
	pugi::xml_parse_result result;
	pugi::xml_node si_node;
	pugi::xml_node t_node;

	result = doc.load_file(final_path.c_str());

	if (!result){
		return 1;
	}

	node = doc.child("sst");

	for (int i = init_size; i < shared_strings.size(); i++){

		si_node = node.append_child("si");
		t_node = si_node.append_child("t");
		t_node.append_child(pugi::node_pcdata).set_value(shared_strings[i].c_str());
	}

	// update sheet xml
	doc.save_file(final_path.c_str());

	// replace inside zip
	//~ res = myzip(&excel_path[0u], &excel_iner_file[0u], &final_path[0u]);

	return 0;
};

int
ExcelManager::create_temp_folder(){

#ifdef _WIN32
	// get windows temporary folder, example from
	// https://docs.microsoft.com/en-us/windows/win32/fileio/creating-and-using-a-temporary-file

	DWORD res = 0;
	TCHAR temp_path[MAX_PATH];

	res = GetTempPath(MAX_PATH, temp_path);

	if (res > MAX_PATH || (res == 0)){
		return 1;
	}

	// template for unique filename
	std::string tplt = "ampltemp-";

	tplt += get_current_date2();
	tplt += "-";

	DWORD gtc = GetTickCount();
	DWORD gcpi = GetCurrentProcessId();

	std::stringstream ss;
	ss << gtc;

	tplt += ss.str();
	tplt += "-";

	ss.str(std::string());
	ss.clear();
	ss << gcpi;

	tplt += ss.str();

	std::string full_path = temp_path + std::string("\\") + tplt;

	res = CreateDirectory(full_path.c_str(), NULL);
	Sleep(10); // to avoid potential name colisions

	if (res == 0){
		return 1;
	}

	temp_folder = full_path;

#else
	char tp[] = "/tmp/ampltempXXXXXX";
	char *dir_name = mkdtemp(tp);


	if (dir_name == NULL){
		return 1;
	}

	temp_folder = std::string(dir_name);
	
#endif

	return 0;
};

int
ExcelManager::clean_temp_folder(){

#ifdef _WIN32

	WIN32_FIND_DATA ffd;
	HANDLE hFind = INVALID_HANDLE_VALUE;

	std::string help_path = temp_folder;
	help_path += "\\*";

	hFind = FindFirstFile(&help_path[0], &ffd);

	do {
		std::string mystr = temp_folder;
		mystr += "\\";
		mystr += std::string(ffd.cFileName);
		int res_del = DeleteFile(&mystr[0]);

		if (verbose > 1){
			if (res_del == 0){
				//~ std::cout << "amplxl: could not delete temporary file: " << GetLastError() << std::endl;
				printf("amplxl: could not delete temporary file: error %d", GetLastError());
			}
		}

	} while (FindNextFile(hFind, &ffd));

	int res_rem = RemoveDirectory(&temp_folder[0u]);

	if (verbose > 1){
		if (res_rem == 0){
			//~ std::cout << "amplxl: could not remove temporary folder: " << GetLastError() << std::endl;
			printf("amplxl: could not remove temporary folder:: error %d", GetLastError());
		}
	}

#else
	std::string test_string = std::string("/tmp/ampltemp");
	if (temp_folder.substr(0, test_string.size()) == test_string){

		std::string my_com = std::string("exec rm -r ") + temp_folder;
		int res_rem = system(my_com.c_str());

	if (verbose > 1){
		if (res_rem != 0){
			printf("amplxl: could not remove temporary folder.\n");
		}
	}
	}
#endif

	return 0;
};








void
ExcelWriteManager::set_cell_value(
	DbCol *db,
	int db_row,
	pugi::xml_node xl_cell
){
	// xl cell has a value child of type "v"
	pugi::xml_node xl_val = xl_cell.child("v");
	if(!xl_val){
		xl_val = xl_cell.append_child("v");
	}

	std::string temp_str;
	bool is_str = false;

	// check if value to write is string or numeric 
	if (db->sval && db->sval[db_row]){

		int sstring_pos = check_shared_strings(std::string(db->sval[db_row]));
		temp_str = numeric_to_string(sstring_pos);
		is_str = true;
	}
	else{

		//~ bool is_int = double_is_int(db->dval[db_row]);

		//~ if (is_int){
			// write in scientific
			temp_str = numeric_to_scientific(db->dval[db_row]);
		//~ }
		//~ else{
			//~ // write in decimal
			//~ temp_str = numeric_to_fixed(db->dval[db_row], 17);
		//~ }
	}

	// check cell data type ("t" attribute)
	// if we are writing a string we change it to a shared string
	if (is_str){
		pugi::xml_attribute attr = xl_cell.attribute("t");
		if (attr){
			attr.set_value("s");
		}
		else{
			xl_cell.append_attribute("t") = "s";
		}
	}
	// change to number
	else{
		pugi::xml_attribute attr = xl_cell.attribute("t");
		if (attr){
			attr.set_value("n");
		}
		else{
			xl_cell.append_attribute("t") = "n";
		}
	}

	// write info in data node
	pugi::xml_node data_node = xl_val.first_child();
	if (!data_node){
		xl_val.append_child(pugi::node_pcdata).set_value(temp_str.c_str());
	}
	else{
		data_node.set_value(temp_str.c_str());
	}
};



void
ExcelManager::set_cell_string_value(
	pugi::xml_node xl_cell,
	const std::string wstr,
	int row,
	const std::string col

){
	int pos = check_shared_strings(wstr);
	std::string shared_string_num = numeric_to_string(pos);
	std::string cell_adress = col + numeric_to_string(row);

	pugi::xml_node xl_val = xl_cell.child("v");
	if(!xl_val){
		xl_val = xl_cell.append_child("v");
	}

	pugi::xml_attribute attr = xl_cell.attribute("t");
	if (attr){
		attr.set_value("s");
	}
	else{
		xl_cell.append_attribute("t") = "s";
	}

	pugi::xml_node dnode = xl_val.first_child();
	if (!dnode){
		xl_val.append_child(pugi::node_pcdata).set_value(shared_string_num.c_str());
	}
	else{
		dnode.set_value(shared_string_num.c_str());
	}
};










#ifdef _WIN32
void mymkstemp(std::string& tmpl, int pos){

	const char alphanum[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

	// get a semi decent seed
	unsigned int seed = GetTickCount() + GetCurrentProcessId();
	srand (seed);
	Sleep(10); // otherwise it can be too fast and overlap

	for (int i = pos; i < tmpl.size(); i++){
		tmpl[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
	}
};
#endif



void unquote_string(std::string &str){

	size_t n;

	n = str.size();

	if (str[0] == '\'' && str[n-1] == '\''){
		str = str.substr(1, n-2);
	}

	n = str.size();

	if (str[0] == '"' && str[n-1] == '"'){
		str = str.substr(1, n-2);
	}
};


int
ExcelWriteManager::delete_data(pugi::xml_node parent){

	int include_header = 0;
	if (write == std::string("drop")){
		include_header = 1;
	}

	if (has_range){
		if (range_first_row != range_last_row){
			delete_range(parent, include_header);
		}
		else{
			delete_header_range(parent, include_header);
		}
	}
	else{
		delete_sheet(parent, include_header);
	}
	return 0;
};

int
ExcelWriteManager::delete_range(pugi::xml_node parent, int include_header){

	if (verbose > 1){
		printf("Deleting range...\n");

		//~ std::cout << "range fc:" << range_first_col << std::endl;
		//~ std::cout << "range lc:" << range_last_col << std::endl;

		//~ std::cout << "range fr:" << range_first_row << std::endl;
		//~ std::cout << "range lr:" << range_last_row << std::endl;

	}

	int shift = 1;
	if (include_header == 1){
		shift = 0;
	}

	pugi::xml_node excel_row;
	pugi::xml_node excel_cell;

	const char* row_attr = "r";

	std::stringstream strs;
	std::string row_id_str;
	std::string iter_col;
	std::string cell_adress;

	excel_row = get_excel_row(parent, range_first_row + shift);

	// iterate rows
	for (int i = range_first_row + 1; i <= range_last_row; i++){

		strs.str(std::string()); // clear stringstream
		strs << i;
		row_id_str = strs.str();

		if (excel_row.attribute(row_attr).value() != row_id_str){
			excel_row = get_excel_row(parent, i);
		}

		if (excel_row){

			// iterate columns and delete cell (if found)
			iter_col = range_first_col;
			std::string cell_adress = iter_col + row_id_str;
			excel_cell = excel_row.find_child_by_attribute(row_attr, cell_adress.c_str());


			while (1){
				std::string cell_adress = iter_col + row_id_str;

				// get the cell element
				
				if (excel_cell.attribute(row_attr).value() != cell_adress){
					excel_cell = excel_row.find_child_by_attribute(row_attr, cell_adress.c_str());
				}

				if (excel_cell){

					pugi::xml_node excel_val = excel_cell.child("v");

					if (excel_val){
						excel_cell.remove_child(excel_val);
					}
				}

				if (iter_col == range_last_col){
					break;
				}

				excel_cell = excel_cell.next_sibling();
				ecm.next(iter_col);
			}
		}
		excel_row = excel_row.next_sibling();
	}

	if (verbose > 1){
		printf("Delete range done.");
	}
	return 0;
};


int
ExcelWriteManager::delete_header_range(pugi::xml_node parent, int include_header){

	int shift = 1;
	if (include_header == 1){
		shift = 0;
	}

	// find first empty row in header defined table
	const char* row_attr = "r";
	std::stringstream strs;
	std::string row_id;

	int iter_row = range_first_row + shift;
	int last_row = iter_row;

	strs.str(std::string()); // clear stringstream
	strs << iter_row;
	row_id = strs.str();

	pugi::xml_node row_child = parent.find_child_by_attribute(row_attr, row_id.c_str());

	while (1){

		strs.str(std::string()); // clear stringstream
		strs << iter_row;
		row_id = strs.str();

		if (row_child.attribute(row_attr).value() != row_id){
			row_child = parent.find_child_by_attribute(row_attr, row_id.c_str());
		}

		bool has_content = false;

		std::string iter_col = range_first_col;

		while (1){

			// concatenate iter_col and i to get the excel cell information
			std::string cell_adress = iter_col + row_id;

			// get the element
			pugi::xml_node excel_cell = row_child.find_child_by_attribute(row_attr, cell_adress.c_str());

			if (excel_cell){

				std::string value = excel_cell.child("v").child_value();

				if (excel_cell.attribute("t").value() == std::string("s")){
					value = shared_strings[std::atoi(value.c_str())];
				}
				else if (excel_cell.attribute("t").value() == std::string("inlineStr")){
					value = excel_cell.child("is").child("t").child_value();
				}

				if (value.length() > 0){
					has_content = true;
				}
			}

			if (iter_col == range_last_col){
				break;
			}

			ecm.next(iter_col);
		}

		if (!has_content){
			break;
		}

		last_row = iter_row;
		row_child = row_child.next_sibling();
		iter_row += 1;
	}
	range_last_row = last_row;
	delete_range(parent, include_header);
	return 0;
};


int
ExcelWriteManager::delete_sheet(pugi::xml_node parent, int include_header){

	const char* row_attr = "r";
	std::stringstream strs;
	std::string row_id;
	pugi::xml_node temp_node;

	// save header node
	if (include_header == 0){

		strs.str(std::string());
		strs << 1; // header must be first row
		row_id = strs.str();

		pugi::xml_node child = parent.first_child();

		while (child){
			pugi::xml_node next_child = child.next_sibling();
			if (child.attribute(row_attr).value() != row_id){
				parent.remove_child(child);
			}
			child = next_child;
		}

	}
	else{

		// delete everything
		pugi::xml_node child = parent.first_child();

		while (child){
			pugi::xml_node next_child = child.next_sibling();
			parent.remove_child(child);
			child = next_child;
		}
	}
	return 0;
};


int
ExcelWriteManager::write_header(
	pugi::xml_node parent,
	int first_row,
	std::string & first_col,
	std::map<std::string, pugi::xml_node> & cell_map
){

	std::string row_id = numeric_to_string(first_row);
	std::string iter_col = first_col;

	for (int i = 0; i < TI->arity + TI->ncols; i++){

		std::string wstr = std::string(TI->colnames[i]);
		int pos = check_shared_strings(wstr);

		std::string shared_string_num = numeric_to_string(pos);

		std::string cell_adress = iter_col + row_id;

		pugi::xml_node excel_cell = cell_map[cell_adress];

		pugi::xml_node excel_val = excel_cell.child("v");
		if(!excel_val){
			excel_val = excel_cell.append_child("v");
		}

		pugi::xml_attribute attr = excel_cell.attribute("t");
		if (attr){
			attr.set_value("s");
		}
		else{
			excel_cell.append_attribute("t") = "s";
		}

		pugi::xml_node dnode = excel_val.first_child();
		if (!dnode){
			excel_val.append_child(pugi::node_pcdata).set_value(shared_string_num.c_str());
		}
		else{
			dnode.set_value(shared_string_num.c_str());
		}

		ecm.next(iter_col);

	}

	/*
	// get the header row
	const char* row_attr = "r";

	std::stringstream strs;
	strs << first_row;
	std::string row_id = strs.str();

	pugi::xml_node excel_row = parent.find_child_by_attribute(row_attr, row_id.c_str());

	std::string iter_col = first_col;

	for (int i = 0; i < TI->arity + TI->ncols; i++){

		std::string wstr = std::string(TI->colnames[i]);
		int pos = check_shared_strings(wstr);

		strs.str(std::string());
		strs << pos;

		std::string cell_adress = iter_col + row_id;

		// get the cell element
		pugi::xml_node excel_cell = excel_row.find_child_by_attribute(row_attr, cell_adress.c_str());

		if (!excel_cell){
			excel_cell = excel_row.append_child("c");
			excel_cell.append_attribute("r") = cell_adress.c_str();
		}

		pugi::xml_node excel_val = excel_cell.child("v");
		if(!excel_val){
			excel_val = excel_cell.append_child("v");
		}

		pugi::xml_attribute attr = excel_cell.attribute("t");
		if (attr){
			attr.set_value("s");
		}
		else{
			excel_cell.append_attribute("t") = "s";
		}

		pugi::xml_node dnode = excel_val.first_child();
		if (!dnode){
			excel_val.append_child(pugi::node_pcdata).set_value(&strs.str()[0u]);
		}
		else{
			dnode.set_value(&strs.str()[0u]);
		}

		ecm.next(iter_col);
	}

	range_last_col = iter_col;
	*/
	return 0;
};




int
ExcelWriteManager::write_header_2D(
	pugi::xml_node parent,
	int first_row,
	std::string & first_col,
	std::map<std::string, pugi::xml_node> & cell_map,
	std::map<std::string, std::string> & xl_col_map
){

	std::string row_id = numeric_to_string(first_row);
	std::string iter_col = first_col;
	std::string ampl_header;

	// write the first arity - 1 column names
	for (int i = 0; i < TI->arity - 1; i++){

		ampl_header = TI->colnames[i];
		std::string cell_adress = iter_col + row_id;
		pugi::xml_node xl_cell = cell_map[cell_adress];
		set_cell_string_value(xl_cell, ampl_header, first_row, iter_col);
		xl_col_map[ampl_header] = iter_col;
		ecm.next(iter_col);
	}

	// write the elements of the last arity in the header
	DbCol* db = &TI->cols[TI->arity - 1];
	for (int i = 0; i < TI->nrows; i++){

		std::string cell_adress = iter_col + row_id;
		pugi::xml_node xl_cell = cell_map[cell_adress];

		if (db->sval && db->sval[i]){
			ampl_header = db->sval[i];
		}
		else{
			ampl_header = "ampl-numeric-" + numeric_to_scientific(db->dval[i]);
		}
		// skip repetitions
		if (xl_col_map.find(ampl_header) == xl_col_map.end()){
			xl_col_map[ampl_header] = iter_col;
			set_cell_value(db, i, xl_cell);
			ecm.next(iter_col);
		}

	}


	return 0;
};





















void
get_maps(
	pugi::xml_node parent,
	std::map<std::string, pugi::xml_node> & row_map,
	std::map<std::string, pugi::xml_node> & cell_map,
	Logger & logger
){
	std::string msg;
	msg = "Get maps...";
	logger.log(msg, LOG_DEBUG);

	std::clock_t start_time = get_time();

	pugi::xml_node row_node = parent.first_child();

	while(row_node){

		std::string row_ref = row_node.attribute("r").value();
		row_map[row_ref] = row_node;

		pugi::xml_node cell_node = row_node.first_child();

		while(cell_node){

			std::string cell_ref = cell_node.attribute("r").value();
			cell_map[cell_ref] = cell_node;
			cell_node = cell_node.next_sibling();
		}
		row_node = row_node.next_sibling();
	}

	std::clock_t end_time = get_time();
	double total_time = clock_to_seconds(start_time, end_time);

	msg = std::string("Get maps done in ") +  numeric_to_fixed(total_time, CPUTIMES_NDIGITS) + std::string(" seconds");
	logger.log(msg, LOG_DEBUG);
};


int
check_table_cells(
	pugi::xml_node parent,
	std::map<std::string, pugi::xml_node> & row_map,
	std::map<std::string, pugi::xml_node> & cell_map,
	int first_row,
	int last_row,
	std::string & first_col,
	std::string & last_col,
	Logger & logger
){
	std::string msg;
	msg = "Check table cells...";
	logger.log(msg, LOG_DEBUG);

	std::clock_t start_time = get_time();


	// auxiliary vector with the strings that define the columns in the spreadsheet representation 
	// of the table, e.g., ["AA", "AB", "AC"]
	std::vector<std::string> col_range;
	fill_range(col_range, first_col, last_col);

	// check if the first row of the table already exists
	pugi::xml_node anchor = row_map[numeric_to_string(first_row)];

	if (!anchor){
		// row does not exist, get the first existing row in the sheet
		pugi::xml_node init_row = parent.first_child();

		if (!init_row){
			// xl sheet has no rows, the first row of the table will be the first one 
			anchor = parent.append_child("row");
		}
		else{
			int init_row_num = std::atoi(init_row.attribute("r").value());

			if (init_row_num > first_row){
				// our new row will be the first
				anchor = parent.prepend_child("row");
			}
			else{
				// we iterate untill we get the position of the new row
				bool found = false;
				pugi::xml_node iter_row = init_row;
				while (iter_row){
					pugi::xml_node next_row = iter_row.next_sibling();
					int next_row_num = std::atoi(next_row.attribute("r").value());

					if (next_row_num > first_row){
						found = true;
						break;
					}
					iter_row = next_row;
				}

				if (found){
					// we got the position for the insertion
					anchor = parent.insert_child_after("row", iter_row);
				}
				else{
					// insert at the end
					anchor = parent.append_child("row");
				}
			}
		}
		std::string row_num = numeric_to_string(first_row);
		anchor.append_attribute("r") = row_num.c_str();
		row_map[row_num] = anchor;
	}
	// garantee that the row has all the required cells
	add_missing_cells(anchor, first_row, col_range, cell_map, logger);

	// now that we have the first row we know that the following rows are contiguous
	// so we just iterate and add rows as needed

	for (int i = first_row + 1; i <= last_row; i++){

		std::string next_row_num_str = numeric_to_string(i);
		pugi::xml_node next_row = anchor.next_sibling();

		if (next_row_num_str == next_row.attribute("r").value()){
			//we have a match, just advance
			anchor = next_row;
			add_missing_cells(anchor, i, col_range, cell_map, logger);
		}
		else{
			// no match, add the new cell
			pugi::xml_node child = parent.insert_child_after("row", anchor);
			anchor = child;
			anchor.append_attribute("r") = next_row_num_str.c_str();
			row_map[next_row_num_str] = anchor;
			add_range_cells(anchor, i, col_range, cell_map);
		}
	}

	std::clock_t end_time = get_time();
	double total_time = clock_to_seconds(start_time, end_time);

	msg = std::string("Check table cells done in ") +  numeric_to_fixed(total_time, CPUTIMES_NDIGITS) + std::string(" seconds");
	logger.log(msg, LOG_DEBUG);
	return 0;
};


void
add_missing_cells(
	pugi::xml_node row,
	int row_num,
	std::vector<std::string> & col_range,
	std::map<std::string, pugi::xml_node> & cell_map,
	Logger & logger
){
	std::string msg;
	msg = "Add missing cells...";
	logger.log(msg, LOG_DEBUG);

	std::clock_t start_time = get_time();

	// get the first cell (anchor) of the column range we are checking
	std::string cell_ref = col_range[0] + numeric_to_string(row_num);
	pugi::xml_node anchor = cell_map[cell_ref];

	std::string temp_str;

	if (!anchor){
		// if anchor does not exist we get the first child in the row
		pugi::xml_node iter_cell = row.first_child();

		if (!iter_cell){
			// if the first child does not exist our new cell will be the first cell in the row
			anchor = row.append_child("c");
		}
		else{
			// check if already existing first child in the row is before or after our new cell
			int new_cell_col_num = cell_reference_to_number(cell_ref);
			temp_str = iter_cell.attribute("r").value();
			int curr_cell_col_num = cell_reference_to_number(temp_str);

			if (curr_cell_col_num > new_cell_col_num){
				// the existing first cell in the row is after the first cell of the range
				// we insert our new cell in the first positionof the row
				anchor = row.prepend_child("c");
			}
			else{
				// we iterate existing cells until we find the position to insert the new cell
				bool found = false;
				while(iter_cell){
					pugi::xml_node next_cell = iter_cell.next_sibling();
					temp_str = next_cell.attribute("r").value();
					int next_cell_col_num = cell_reference_to_number(temp_str);

					if (next_cell_col_num > new_cell_col_num){
						found = true;
						break;
					}
					iter_cell = next_cell;
				}
				if (found){
					// new cell is inserted after iter_cell
					anchor = row.insert_child_after("c", iter_cell);
				}
				else{
					// all existing cells are before the new one
					// we insert the new cell at the end of the row
					anchor = row.append_child("c");
				}
			}
		}
		// update new cell attributes
		anchor.append_attribute("r") = cell_ref.c_str();
		// we assume the cell has attribute t=s, this may be overriden later
		anchor.append_attribute("t") = "s";
		cell_map[cell_ref] = anchor;
	}

	// now that we have the first cell we know that the following ones are contiguous
	// so we just iterate and add cells as required
	for (int i = 1; i < col_range.size(); i++){

		std::string new_cell_ref = col_range[i] + numeric_to_string(row_num);
		pugi::xml_node next_cell = anchor.next_sibling();

		if (new_cell_ref == next_cell.attribute("r").value()){
			//we have a match, just advance
			anchor = next_cell;
		}
		else{
			// no match, add the new cell
			pugi::xml_node child = row.insert_child_after("c", anchor);
			anchor = child;
			anchor.append_attribute("r") = new_cell_ref.c_str();
			// we assume the cell has attribute t=s, this may be overriden later
			anchor.append_attribute("t") = "s";
			cell_map[new_cell_ref] = anchor;
		}
	}

	std::clock_t end_time = get_time();
	double total_time = clock_to_seconds(start_time, end_time);

	msg = std::string("Add missing cells done in ") +  numeric_to_fixed(total_time, CPUTIMES_NDIGITS) + std::string(" seconds");
	logger.log(msg, LOG_DEBUG);
};


int
cell_reference_to_number(std::string & s){

	int r = 0;
	for (int i = 0; i < s.length(); i ++) {
		r = r * 26 + s[i] - 64;
	}
	return r;
};

std::string
number_to_cell_reference(int n){

	std::string r = "";
	while (n > 0) {
		r = (char)(65 + (n - 1) % 26) + r;
		n = (n - 1) / 26;
	}
	return r;
};


double
clock_to_seconds(std::clock_t start_time, std::clock_t end_time){
	return double(end_time - start_time) / CLOCKS_PER_SEC;
};


std::clock_t get_time(){
	return std::clock();
};

void
fill_range(std::vector<std::string> & col_range, std::string & first_col, std::string & last_col){

	std::string iter_col = first_col;

	ExcelColumnManager ecm;

	while (true){
		col_range.push_back(iter_col);

		if (iter_col == last_col){break;}

		ecm.next(iter_col);
	}

};

void
add_range_cells(pugi::xml_node row, int row_num, std::vector<std::string> & col_range, std::map<std::string, pugi::xml_node> & cell_map){

	std::string row_num_str = numeric_to_string(row_num);

	for (int i = 0; i < col_range.size(); i++){

		std::string cell_ref = col_range[i] + row_num_str;

		pugi::xml_node child = row.append_child("c");
		child.append_attribute("r") = cell_ref.c_str();
		// we assume the cell has attribute t=s, this may be overriden later
		child.append_attribute("t") = "s";
		cell_map[cell_ref] = child;
	}

};




pugi::xml_node
row_insert_cell(
	pugi::xml_node row_node,
	int row_num,
	std::string & cell_col
){
	std::string cell_ref = cell_col + numeric_to_string(row_num);
	pugi::xml_node cell_iter_node = row_node.first_child();
	pugi::xml_node cell_node;

	std::string tmp_str;

	// if row is empty, add cell to row
	if (!cell_iter_node){
		cell_node = row_node.append_child("c");
		cell_add_basic_attrs(cell_node, cell_ref);
		return cell_node;
	}

	int cell_node_num = cell_reference_to_number(cell_ref);
	tmp_str = cell_iter_node.attribute("r").value();
	int cell_iter_num = cell_reference_to_number(tmp_str);

	// if first cell in row is after our cell add cell at the beggining of the row
	if (cell_iter_num > cell_node_num){
		cell_node = row_node.prepend_child("c");
		cell_add_basic_attrs(cell_node, cell_ref);
		return cell_node;
	}

	// iterate cells to find place of insertion
	pugi::xml_node prev_node;

	while (cell_iter_node){

		tmp_str = cell_iter_node.attribute("r").value();
		cell_iter_num = cell_reference_to_number(tmp_str);

		if (cell_iter_num > cell_node_num){
			break;
		}
		prev_node = cell_iter_node;
		cell_iter_node = cell_iter_node.next_sibling();
	}
	cell_node = row_node.insert_child_after("c", prev_node);
	cell_add_basic_attrs(cell_node, cell_ref);
	return cell_node;
};

void
cell_add_basic_attrs(
	pugi::xml_node node,
	std::string & cell_ref
){
	node.append_attribute("r") = cell_ref.c_str();
	node.append_attribute("t") = "s";
};


int
ExcelReadManager::manage_data2D(){

	std::string msg;
	msg = "Manage data 2D...";
	logger.log(msg, LOG_DEBUG);

	int result = 0;

	result = get_sheet_from_zip();

	if (result){
		msg = "Could not extract sheet " + excel_iner_file;
		logger.log(msg, LOG_ERROR);
		return 1;
	}

	// open xml
	excel_file = data_sheet.substr(data_sheet.find_last_of("/") + 1); 
	join_path(temp_folder, excel_file, final_path);

	pugi::xml_document doc;
	pugi::xml_node node;
	pugi::xml_parse_result pugi_result;

	pugi_result = doc.load_file(final_path.c_str());

	if (!pugi_result){
		msg = "Could not load sheet";
		logger.log(msg, LOG_ERROR);
		return 1;
	}

	node = doc.child("worksheet").child("sheetData");

	// get table dimensions
	int first_row = 1;
	int last_row = EXCEL_MAX_ROWS;
	std::string first_col = std::string("A");
	std::string last_col = EXCEL_MAX_COLS;

	if (has_range){
		first_row = range_first_row;
		first_col = range_first_col; 
		last_col = range_last_col; 
	}

	if (has_range){
		if (range_first_row != range_last_row){
			last_row = range_last_row;
		}
		else{
			last_row = get_last_row_in_table(node, first_row, first_col);
		}
	}
	else{
		result = get_table_top_left_coords(node, first_row, first_col);
		if (result){
			msg = "Could not get table top left coords";
			logger.log(msg, LOG_ERROR);
			return 1;
		}

		result = get_last_column_in_table(node, first_row, first_col, last_col);
		if (result){
			msg = "Could not get last column in table";
			logger.log(msg, LOG_ERROR);
			return 1;
		}
		last_row = get_last_row_in_table(node, first_row, first_col);
	}

	log_table_coords(first_col, last_col, first_row, last_row);

	result = parse_data2D(node, first_row, last_row, first_col, last_col);

	if (result){
		return 1;
	}

	msg = "Manage data 2D done!";
	logger.log(msg, LOG_DEBUG);

	return 0;
};


void ExcelManager::get_cell_val(pugi::xml_node node, std::string & val){

		val = "";
		val = node.child("v").child_value();

		if (node.attribute("t").value() == std::string("s")){
			val = shared_strings[std::atoi(val.c_str())];
		}
		else if (node.attribute("t").value() == std::string("inlineStr")){
			val = node.child("is").child("t").child_value();
		}
};

void
ExcelManager::set_dbcol_val(std::string & val, DbCol * db, int is_string){

	char* se;
	double t;

	if (val.empty()){
		db->sval[0] = TI->Missing;
	}
	else{
		if (is_string){
			db->sval[0] = &val[0u];
		}
		else{
			// try to convert the value to numeric
			t = strtod(val.c_str(), &se);
			if (!*se) {/* valid number */
				db->sval[0] = 0;
				db->dval[0] = t;
				//~ std::cout << "assigning numeric: " << db->dval[0] << std::endl;
			}
			else{
				db->sval[0] = &val[0u];
				//~ std::cout << "assigning string: " << db->sval[0] << std::endl;
			}
		}
	}
};


bool
ExcelManager::check_is_number(const std::string & val){

	char* se;
	double t;

	// try to convert the value to numeric
	t = strtod(val.c_str(), &se);
	if (!*se) {/* valid number */
		return true;
	}
	return false;
};


int
ExcelManager::get_last_column_in_table(
	const pugi::xml_node node,
	const int first_row,
	const std::string & first_col,
	std::string & last_col
){
	if (verbose > 2){printf("amplxl: get_last_column_in_table...\n");}

	std::string row_id = numeric_to_string(first_row);
	pugi::xml_node iter_row = node.find_child_by_attribute(row_attr, row_id.c_str());

	if (!iter_row){
		if (verbose == 73){
			printf("get_last_column_in_table: cannot get first row\n");
		}
		return 0;
	}

	std::string iter_col = first_col;
	std::string cell_ref = iter_col + row_id;
	pugi::xml_node cell = iter_row.find_child_by_attribute(row_attr, cell_ref.c_str());
	std::string value;

	// iterate columns
	while (true){

		cell_ref = iter_col + row_id;

		if (cell.attribute(row_attr).value() != row_id){
			cell = iter_row.find_child_by_attribute(row_attr, cell_ref.c_str());
		}

		if (!cell){
			// no more columns to parse
			return 0;
		}

		value = cell.child("v").child_value();

		if (cell.attribute("t").value() == std::string("inlineStr")){
			value = cell.child("is").child("t").child_value();
		}

		if(value.empty()){
			// cell may have only formating
			return 0;
		}

		last_col = iter_col;
		cell = cell.next_sibling();
		ecm.next(iter_col);

	}
	if (verbose > 2){printf("amplxl: get_last_column_in_table done!\n");}

	return 0;
};

int
ExcelManager::get_last_row_in_table(
	const pugi::xml_node node,
	const int first_row,
	const std::string & first_col
){
	int iter_row_num = first_row + 1; // first row only has headers
	int last_row = -1;

	std::string row_id = numeric_to_string(iter_row_num);
	pugi::xml_node iter_row = node.find_child_by_attribute(row_attr, row_id.c_str());

	std::string value;
	std::string cell_ref;
	pugi::xml_node cell;

	while(true){

		row_id = numeric_to_string(iter_row_num);

		if (iter_row.attribute(row_attr).value() != row_id){
			iter_row = node.find_child_by_attribute(row_attr, row_id.c_str());
		}

		if (!iter_row){break;}

		cell_ref = first_col + row_id;
		cell = iter_row.find_child_by_attribute(row_attr, cell_ref.c_str());

		if (!cell){break;}

		value = cell.child("v").child_value();

		if (cell.attribute("t").value() == std::string("inlineStr")){
			value = cell.child("is").child("t").child_value();
		}


		if(value.empty()){break;}

		last_row = iter_row_num;
		iter_row_num += 1;

		iter_row = iter_row.next_sibling();
	}

	if (last_row == -1){
		last_row = first_row + TI->nrows;
	}

	return last_row;
};







int
ExcelWriteManager::write_data_out_2D(
	pugi::xml_node node,
	int first_row,
	int last_row,
	std::string &first_col,
	std::string &last_col
){

	if (verbose > 0){
		printf("amplxl: write_data_2D...\n");
	}
	std::clock_t start_time = get_time();

	std::string msg;

	pugi::xml_node excel_row;
	pugi::xml_node excel_cell;
	pugi::xml_node excel_val;
	pugi::xml_node dnode;

	// parse header info
	std::map<std::string, std::string> xl_col_map;
	parse_header(first_col, last_col, first_row, node, xl_col_map);

	std::string h_set;
	int h_set_pos = -1;
	bool has_header = true;

	if (xl_col_map.size() == 0){

		// no header provided
		h_set_pos = TI->arity - 1;
		h_set = TI->colnames[h_set_pos];
		has_header = false;
	}
	else{
		// one of the columns will not appear in xl_col_map
		// this column elements will appear in the header

		std::string ampl_col_name;
		std::string ampl_col_value;
		bool found = false;

		for (int i = 0; i < TI->arity; i++){

			ampl_col_name = TI->colnames[i];

			if (xl_col_map.find(ampl_col_name) == xl_col_map.end()){

				if (found){
					// more than one column not mapped
				}

				found = true;
				h_set = ampl_col_name;
				h_set_pos = i;
			}
		}
	}

	if (h_set_pos == -1){
		// could not find hset
	}

	msg = "hset: " + h_set + ", " + numeric_to_string(h_set_pos);
	logger.log(msg, LOG_DEBUG);


	// only now we can deduce the number of rows of the table
	std::map<std::vector<std::string>, int> key_set;
	int n_recalc_rows = count_2D_rows(key_set, h_set_pos); // warning this is doing more than counting the rows
	last_row = first_row + n_recalc_rows;

	if (!has_header){
		// need to recalculate last column

		// count number of unique elements in h_set
		int card_h_set = 0;
		std::map<std::string, int> h_set_items;

		for (int i=0; i<TI->nrows; i++){

			std::string temp_item;

			if (TI->cols[h_set_pos].sval && TI->cols[h_set_pos].sval[i]){
				temp_item = TI->cols[h_set_pos].sval[i];
			}
			else{
				temp_item = "ampl-numeric-" + numeric_to_string(TI->cols[h_set_pos].dval[i]);
			}

			if (!temp_item.empty()){
				if (h_set_items.find(temp_item) == h_set_items.end()){
					h_set_items[temp_item] = card_h_set;
					card_h_set += 1;
				}
			}
		}

		int n_recalc_cols = TI->arity - 1 + card_h_set;
		int first_col_num = cell_reference_to_number(first_col);
		int last_col_num = first_col_num + n_recalc_cols - 1;
		last_col = number_to_cell_reference(last_col_num);

		//~ msg = "card_h_set: " + numeric_to_string(card_h_set);
		//~ logger.log(msg, LOG_DEBUG);
		//~ msg = "n_recalc_cols: " + numeric_to_string(n_recalc_cols);
		//~ logger.log(msg, LOG_DEBUG);
		//~ msg = "first_col_num: " + numeric_to_string(first_col_num);
		//~ logger.log(msg, LOG_DEBUG);
		//~ msg = "last_col_num: " + numeric_to_string(last_col_num);
		//~ logger.log(msg, LOG_DEBUG);
		//~ msg = "first_col: " + numeric_to_string(first_col);
		//~ logger.log(msg, LOG_DEBUG);
		//~ msg = "last_col: " + numeric_to_string(last_col);
		//~ logger.log(msg, LOG_DEBUG);
	}

	log_table_coords(first_col, last_col, first_row, last_row);

	// map rows and cells for faster access
	std::map<std::string, pugi::xml_node> row_map;
	std::map<std::string, pugi::xml_node> cell_map;
	get_maps(node, row_map, cell_map, logger);

	// check that all required cells exist
	check_table_cells(
		node,
		row_map,
		cell_map,
		first_row,
		last_row,
		first_col,
		last_col,
		logger
	);

	// write default header if none was given
	if (!has_header){
		write_header_2D(node, first_row, first_col, cell_map, xl_col_map);
	}


	if (verbose == 73){
		// check map
		std::map<std::string, std::string>::iterator it;

		for (it = xl_col_map.begin(); it != xl_col_map.end(); it++){

			std::cout << it->first  // string (key)
				<< ':'
				<< it->second   // string's value 
				<< std::endl ;
		}
	}


	// write data
	const int ampl_ncols = TI->arity + TI->ncols;
	DbCol *db;
	DbCol *auxdb;
	std::string xl_col_name;

	int ampl_row = 0;
	int xl_row = first_row + 1;

	std::vector<std::string> ampl_keys(TI->arity);
	std::string ampl_col_name;

	//~ while (1){
	for (int k = 0; k < TI->nrows; k++){

		//~ std::cout << "k: " << k << std::endl;

		for (int i = 0; i < TI->arity; i++){

			if (i == h_set_pos){continue;}

			if (TI->cols[i].sval && TI->cols[i].sval[ampl_row]){
				ampl_col_name = TI->cols[i].sval[ampl_row];
			}
			else{
				ampl_col_name = "ampl-numeric-" + numeric_to_string(TI->cols[i].dval[ampl_row]);
			}
			ampl_keys[i] = ampl_col_name;
		}

		//~ print_vector(ampl_keys);

		db = TI->cols;

		for (int i = 0; i < ampl_ncols; i++){

			//~ std::cout << "i: " << i << std::endl;

			if (i == h_set_pos){
				continue;
			}
			else if (i == ampl_ncols - 1){

				if (TI->cols[h_set_pos].sval && TI->cols[h_set_pos].sval[ampl_row]){
					xl_col_name = TI->cols[h_set_pos].sval[ampl_row];
				}
				else{
					xl_col_name = "ampl-numeric-" + numeric_to_string(TI->cols[h_set_pos].dval[ampl_row]);
				}
				auxdb = &TI->cols[ampl_ncols - 1];
			}
			else{
				xl_col_name = TI->colnames[i];
				//~ cell_col = xl_col_map[xl_col_name];
				auxdb = &TI->cols[i];
			}

			std::string cell_col = xl_col_map[xl_col_name];

			//~ std::cout << "xl_col_name: " << xl_col_name << std::endl;
			//~ std::cout << "cell_col: " << cell_col << std::endl;

			//~ std::string cell_col = ampl_to_excel_cols[j];
			std::string cell_row = numeric_to_string(first_row + 1 + key_set[ampl_keys]);
			std::string cell_reference = cell_col + cell_row;

			//~ std::cout << "cell_ref: " << cell_reference << std::endl;

			pugi::xml_node write_cell = cell_map[cell_reference];

			if (!write_cell){
				msg = "Could not find cell " + cell_reference;
				logger.log(msg, LOG_DEBUG);
				return 1;
			}

			set_cell_value(auxdb, ampl_row, write_cell);

			//~ std::cout << "write done" << std::endl;

			db++;
		}

		ampl_row += 1;
	}

	std::clock_t end_time = get_time();
	double total_time = clock_to_seconds(start_time, end_time);

	if (verbose > 0){
		printf("amplxl: write_data_2D done in %.3f s.\n", total_time);
	}

	return 0;
};




int
ExcelManager::get_table_top_left_coords(pugi::xml_node node, int & first_row, std::string & first_col){

	std::string msg;

	pugi::xml_node iter_row;
	std::string row_id;
	bool row_found = false;

	// get first row in xl
	for (int i = 1; i < EXCEL_MAX_ROWS; i++){

		row_id = numeric_to_string(i);
		iter_row = node.find_child_by_attribute(row_attr, row_id.c_str());

		if (iter_row){
			row_found = true;
			first_row = i;
			break;
		}
	}

	if (!row_found){
		msg = "get_table_top_left_coords : Could not find rows in sheet";
		logger.log(msg, LOG_DEBUG);
		return 0;
	}

	// get first column
	pugi::xml_node iter_col;
	std::string cell_ref;
	bool col_found = false;

	std::string test_col = "A";

	while (test_col != EXCEL_MAX_COLS){

		cell_ref = test_col + numeric_to_string(first_row);
		iter_col = iter_row.find_child_by_attribute(row_attr, cell_ref.c_str());

		if (iter_col){
			col_found = true;
			first_col = test_col;
			break;
		}
		ecm.next(test_col);
	}

	if (!col_found){
		msg = "Could not find columns in sheet";
		logger.log(msg, LOG_ERROR);
		return 1;
	}

	msg = "Get_table_top_left_coords: ", first_col + ", " + numeric_to_string(first_row);
	logger.log(msg, LOG_DEBUG);

	return 0;
};


int
write_indexing_sets(
	const pugi::xml_node node,
	const int first_row,
	const int last_row,
	const std::string & first_col,
	const std::string & last_col,
	std::map<std::string, std::string> & first_set_map,
	std::map<std::string, std::string> & second_set_map
){



	return 0;
};




int
ExcelManager::parse_header(

	const std::string & first_col,
	std::string & last_col,
	int first_row,
	pugi::xml_node node,
	std::map<std::string, std::string> & xl_col_map

){
	std::string iter_col;
	std::string cell_adress;
	std::string xl_col_name;

	pugi::xml_node xl_cell;

	//~ int nempty = 0; // number of empty columns parsed
	//~ const int max_empty = 100; // maximum number of empty columns allowed
	//~ bool found = false;

	// get first row
	std::string row_id = numeric_to_string(first_row);
	pugi::xml_node xl_row = node.find_child_by_attribute(row_attr, row_id.c_str());

	iter_col = first_col;
	while(1){

		bool is_numeric = true;
		cell_adress = iter_col + row_id;
		xl_cell = xl_row.find_child_by_attribute(row_attr, cell_adress.c_str());
		xl_col_name = xl_cell.child("v").child_value();

		if (xl_cell.attribute("t").value() == std::string("s")){
			xl_col_name = shared_strings[std::atoi(xl_col_name.c_str())];
			is_numeric = false;
		}
		else if (xl_cell.attribute("t").value() == std::string("inlineStr")){
			xl_col_name = xl_cell.child("is").child("t").child_value();
			is_numeric = false;
		}

		// check if the value is actualy a number to avoid issues with text cells other than shared
		// strings or inline strings
		if (!check_is_number(xl_col_name)){
			is_numeric = false;
		}

		if (!xl_col_name.empty()){
			last_col = iter_col;
			if (is_numeric){
				xl_col_name = "ampl-numeric-" + xl_col_name;
			}

			if (xl_col_map.find(xl_col_name) != xl_col_map.end()){
				std::string msg = "Column \'" + xl_col_name;
				msg += "\' at cell \'";
				msg += iter_col;
				msg += row_id;
				msg += "\' already defined at cell \'";
				msg += xl_col_map[xl_col_name];
				msg += row_id;
				msg += "\'";
				logger.log(msg, LOG_ERROR);
				return 1;
			}

			xl_col_map[xl_col_name] = iter_col;

			if (verbose >= 3){
				std::string msg = "Found column \'";
				msg += xl_col_name;
				msg += "\'";
				logger.log(msg, LOG_DEBUG);
			}
		}
		else{
			//~ nempty += 1;
			break;
		}

		if (has_range){
			if (iter_col == range_last_col){
				break;
			}
		}

		if (iter_col == EXCEL_MAX_COLS){
			if (verbose > 2){
				printf("Last column reached, search done.\n");
			}
			break;
		}

		ecm.next(iter_col);
	}
	return 0;
};




int
ExcelWriteManager::manage_data2D(){

	std::string msg;

	msg = "Manage data 2D...";
	logger.log(msg, LOG_INFO);

	int result = 0;

	result = get_sheet_from_zip();

	if (result){
		msg = "Could not extract sheet " + excel_iner_file;
		logger.log(msg, LOG_ERROR);
		return 1;
	}

	excel_file = data_sheet.substr(data_sheet.find_last_of("/") + 1); 
	join_path(temp_folder, excel_file, final_path);

	std::string sheet_final_path = final_path;

	pugi::xml_document doc;
	pugi::xml_node node;
	pugi::xml_parse_result pugi_result;
	pugi::xml_node_iterator it;
	pugi::xml_node row_child;
	pugi::xml_node excel_cell;
	const char* row_attr = "r";

	pugi_result = doc.load_file(final_path.c_str());

	if (!pugi_result){
		// could not load xml
		msg = "Could not load sheet";
		logger.log(msg, LOG_ERROR);
		return 1;
	}

	node = doc.child("worksheet").child("sheetData");

	// get table dimensions
	int first_row = 1;
	int last_row = EXCEL_MAX_ROWS;
	std::string first_col = std::string("A");
	std::string last_col = EXCEL_MAX_COLS;

	if (has_range){
		first_row = range_first_row;
		first_col = range_first_col; 
		last_col = range_last_col; 
	}

	if (has_range){
		if (range_first_row != range_last_row){
			last_row = range_last_row;
		}
		else{
			last_row = get_last_row_in_table(node, first_row, first_col);
			//~ last_row = first_row + TI->nrows;
		}
	}
	else{
		result = get_table_top_left_coords(node, first_row, first_col);

		if (result){
			return 1;
		}

		result = get_last_column_in_table(node, first_row, first_col, last_col);

		if (result){
			return 1;
		}

		last_row = get_last_row_in_table(node, first_row, first_col);
		//~ last_row = first_row + TI->nrows;

		if (last_row == -1){
			return 1;
		}
	}

	// map shared strings for fast access and get the number of existing strings, since we may add
	// more strings later and need to update the file in excel
	get_sstrings_map();
	int n_sstrings = shared_strings.size();

	if (inout == "OUT"){

		if (write == "drop"){

			result = write_data_out_2D(node, first_row, last_row, first_col, last_col);
		}
		else if (write == "append"){
			msg = "Mode append not available for 2D tables.";
			logger.log(msg, LOG_ERROR);
			return 1;
		}
	}
	else if (inout == "INOUT"){
		if (write == "drop"){

			result = write_data_inout_2D(node, first_row, last_row, first_col, last_col);
		}
		else if (write == "append"){
			msg = "Mode append not available for 2D tables.";
			logger.log(msg, LOG_ERROR);
			return 1;
		}
	}
	else{
		msg = "unsuported write flag";
		logger.log(msg, LOG_ERROR);
		return 1;
	}

	if (result){
		msg = "Could not write 2D data";
		logger.log(msg, LOG_ERROR);
		return 1;
	}

	// vector with the names of the files we need to update in the original spreadsheet
	std::vector<std::string> changed_files;

	// update sheet xml
	excel_iner_file = "xl/" + data_sheet;
	excel_file = data_sheet.substr(data_sheet.find("/") + 1); 
	join_path(temp_folder, excel_file, final_path);

	//~ if (verbose == 73){

		//~ std::cout << "excel_path: " << excel_path << std::endl;
		//~ std::cout << "excel_iner_file: " << excel_iner_file << std::endl;
		//~ std::cout << "final_path: " << final_path << std::endl;
	//~ }

	doc.save_file(final_path.c_str());

	changed_files.push_back(excel_iner_file);

	//~ result = myzip(&excel_path[0u], &excel_iner_file[0u], &final_path[0u]);
	//~ if (result){
		//~ cannot_update_sheet();
		//~ return 1;
	//~ }


	// update shared strings xml

	if (shared_strings.size() > n_sstrings){

		excel_iner_file = "xl/sharedStrings.xml";
		changed_files.push_back(excel_iner_file);

		result = update_shared_strings(n_sstrings);
		if (result){
			msg = "Could not update shared strings";
			logger.log(msg, LOG_ERROR);
			return 1;
		}
	}

	std::string xl_copy_path = temp_folder;

#if defined _WIN32 || defined _WIN64
	xl_copy_path += "\\";
#else
	xl_copy_path += "/";
#endif

	xl_copy_path += "xlcopy.tmp";

	result = copy_uchanged_files(excel_path, xl_copy_path, changed_files);

	// add changed files to zip
	excel_iner_file = "xl/" + data_sheet;
	excel_file = data_sheet.substr(data_sheet.find("/") + 1); 
	join_path(temp_folder, excel_file, final_path);

	result = myzip(xl_copy_path, excel_iner_file, final_path);
	if (result){
		msg = "Could not add sheet to oxml";
		logger.log(msg, LOG_ERROR);
		return 1;
	}

	if (shared_strings.size() > n_sstrings){

		excel_iner_file = "xl/sharedStrings.xml";
		excel_file = "sharedStrings.xml";

		join_path(temp_folder, excel_file, final_path);

		result = myzip(xl_copy_path, excel_iner_file, final_path);
		if (result){
			msg = "Could not add shared strings to oxml";
			logger.log(msg, LOG_ERROR);
			return 1;
		}
	}

	// remove initial file
	result = remove(excel_path.c_str());

	// replace it by modified copy
	// cannot use rename due to issue with files in different partitions
	my_copy_file(xl_copy_path, excel_path);

	// remove
	result = remove(xl_copy_path.c_str());

	msg = "Manage data 2D done!";
	logger.log(msg, LOG_INFO);

	return 0;
};



int ExcelWriteManager::count_2D_rows(std::map<std::vector<std::string>, int> & key_set, int pos){

	int nrows = 0;
	std::vector<std::string> ampl_keys(TI->arity);
	std::string ampl_col_value;
	bool same_row = true;

	for (int i = 0; i < TI->nrows; i++){

		for (int j = 0; j < TI->arity; j++){

			if (j == pos){continue;}

			if (TI->cols[j].sval && TI->cols[j].sval[i]){
				ampl_col_value = TI->cols[j].sval[i];
			}
			else{
				ampl_col_value = "ampl-numeric-" + numeric_to_string(TI->cols[j].dval[i]);
			}
			ampl_keys[j] = ampl_col_value;
		}

		if (key_set.find(ampl_keys) == key_set.end()){

			key_set[ampl_keys] = nrows;
			nrows += 1;
		}
	}

	//~ // check map
	//~ std::map<std::vector<std::string>, int>::iterator it;

	//~ for (it = key_set.begin(); it != key_set.end(); it++){

		//~ print_vector(it->first);

		//~ std::cout <<  it->second   // string's value 
			//~ << std::endl ;
	//~ }
	return nrows;
};




void
ExcelManager::
set_default_2D_col_map(
	const std::string & first_col,
	const std::string & last_col,
	std::map<std::string, std::string> & col_map
){
	std::string iter_col = first_col;
	int key_row_pos = TI->arity - 1;

	// map AMPL columns
	for (int i = 0; i < key_row_pos; i++){
		col_map[TI->colnames[i]] = iter_col;
		ecm.next(iter_col);
	}

	std::string ampl_val;

	// map key row
	for (int i = 0; i < TI->nrows; i++){

		if (TI->cols[key_row_pos].sval && TI->cols[key_row_pos].sval[i]){
			ampl_val = TI->cols[key_row_pos].sval[i];
		}
		else{
			ampl_val = numeric_to_string(TI->cols[key_row_pos].dval[i]);
		}

		// check if element is already mapped
		if (col_map.find(ampl_val) == col_map.end()){
			col_map[ampl_val] = iter_col;
			ecm.next(iter_col);
		}
	}
};


void
ExcelManager::
set_default_col_map(
	const std::string & first_col,
	const std::string & last_col,
	std::map<std::string, std::string> & col_map
){
	std::string iter_col = first_col;

	// map AMPL columns
	for (int i = 0; i < TI->arity + TI->ncols; i++){
		col_map[TI->colnames[i]] = iter_col;
		ecm.next(iter_col);
	}
};




int
ExcelManager::parse_header_2D_reader(

	const std::string & first_col,
	const std::string & last_col,
	int first_row,
	pugi::xml_node node,
	std::map<std::string, std::string> & xl_col_map,
	std::vector<std::string> & header,
	std::vector<int> & is_header_string

){
	std::string msg;
	msg = "parse_header_2D_reader...";
	logger.log(msg, LOG_DEBUG);

	std::string iter_col;
	std::string cell_adress;
	std::string xl_col_name;

	pugi::xml_node xl_cell;

	bool found = false;

	// get first row
	std::string row_id = numeric_to_string(first_row);
	pugi::xml_node xl_row = node.find_child_by_attribute(row_attr, row_id.c_str());

	iter_col = first_col;
	while(1){

		cell_adress = iter_col + row_id;
		xl_cell = xl_row.find_child_by_attribute(row_attr, cell_adress.c_str());

		int is_string = 0;
		xl_col_name = xl_cell.child("v").child_value();

		if (xl_cell.attribute("t").value() == std::string("s")){
			xl_col_name = shared_strings[std::atoi(xl_col_name.c_str())];
			is_string = 1;
		}
		else if (xl_cell.attribute("t").value() == std::string("inlineStr")){
			xl_col_name = xl_cell.child("is").child("t").child_value();
			is_string = 1;
		}

		if (!xl_col_name.empty()){
			xl_col_map[xl_col_name] = iter_col;
			header.push_back(xl_col_name);

			msg = "Found column header '" + xl_col_name + "'";
			logger.log(msg, LOG_DEBUG);
		}
		else if (tableType != TABLE_SHEET){
			msg = "Missing value in header of 2D table at cell ";
			msg += cell_adress;
			msg += " of range ";
			msg += table_name;
			logger.log(msg, LOG_ERROR);
			report_error = false;
			return 1;
		}
		else{
			break;
		}

		is_header_string.push_back(is_string);

		if (iter_col == last_col){
			msg = "Last column reached, search done";
			logger.log(msg, LOG_DEBUG);
			break;
		}
		ecm.next(iter_col);
	}

	msg = "parse_header_2D_reader done!";
	logger.log(msg, LOG_DEBUG);

	return 0;
};


int
ExcelManager::parse_data2D(
	pugi::xml_node node,
	int first_row,
	int last_row,
	std::string & first_col,
	std::string & last_col
){
	std::string msg;

	msg = "Parse data 2D...";
	logger.log(msg, LOG_INFO);

	std::clock_t start_time = get_time();

	// get information from table header
	std::map<std::string, std::string> xl_col_map;
	std::vector<std::string> header;
	std::vector<int> is_header_string;
	int res = parse_header_2D_reader(first_col, last_col, first_row, node, xl_col_map, header, is_header_string);

	if (res && !report_error){
		return 1;
	}
	else if (res){
		// no header found?
		msg = "Cannot find header in 2D table";
		logger.log(msg, LOG_ERROR);
		return 1;
	}

	if (verbose == 73){
		printf("header: ");
		print_vector(header);
	}

	// identify key row in 2D table
	bool found = false;
	int h_set_pos = -1;
	std::string h_set;
	std::string ampl_col_name;

	for (int i = 0; i < TI->arity; i++){

		ampl_col_name = TI->colnames[i];

		if (xl_col_map.find(ampl_col_name) == xl_col_map.end()){

			if (found){
				// more than 1 column not mapped
				msg = "Found more than one candidate for key row in 2D table. At least "
					+ h_set + " and " + ampl_col_name + " are not key columns.";
				logger.log(msg, LOG_ERROR);
				return 1;
			}
			h_set_pos = i;
			h_set = ampl_col_name;
			found = true;
		}
	}

	if (h_set_pos == -1){
		// cannot find h_set
		msg = "Cannot find key row in 2D table.";
		logger.log(msg, LOG_ERROR);
		return 1;
	}

	/*
	* xl_to_ampl_cols is a vector that for each element in header indicates the
	* the corresponding column in the ampl table or -1 if the column is not present.
	* If the element is not present in ampl colnames than it is an element from key row in the
	* 2D table. 
	* arity_cols: contains the indexes in xl_to_ampl_cols that correspond to arity cols
	* value_cols: contains the indexes in xl_to_ampl_cols that do not correspond to arity cols
	* so they will correpond to values when we read the table
	*/
	std::vector<int> xl_to_ampl_cols(header.size(), -1);
	std::vector<int> arity_cols;
	std::vector<int> value_cols; 

	for (int i = 0; i < TI->arity; i++){
		for (int j = 0; j < header.size(); j++){
			if (header[j] == TI->colnames[i]){
				xl_to_ampl_cols[j] = i;
				arity_cols.push_back(j);
			}
		}
	}

	for (int i = 0; i < xl_to_ampl_cols.size(); i++){
		if (xl_to_ampl_cols[i] == -1){
			value_cols.push_back(i);
		}
	}

	if (verbose == 73){
		printf("xl_to_ampl_cols: ");
		print_vector(xl_to_ampl_cols);
		printf("arity cols: ");
		print_vector(arity_cols);
		printf("value cols: ");
		print_vector(value_cols);
	}

	// structure to store info of the row we are reading
	std::vector<std::string> xl_row_info;
	xl_row_info.resize(header.size());

	// check if value was read from numeric or string
	std::vector<int> is_string;
	is_string.resize(header.size());

	// iterate rows
	std::string cell_ref;
	std::string iter_col;
	std::string cell_value;
	pugi::xml_node iter_cell;

	// get the first row with data from the table
	std::string row_id = numeric_to_string(first_row + 1);
	pugi::xml_node iter_row = node.find_child_by_attribute(row_attr, row_id.c_str());

	for (int i = first_row + 1; i <= last_row; i++){

		row_id = numeric_to_string(i);

		// check if rows are in sequence
		if (iter_row.attribute(row_attr).value() != row_id){
			iter_row = node.find_child_by_attribute(row_attr, row_id.c_str());
		}

		if (!iter_row){
			// could not find row ?!, should never enter here
			msg = "Cannot find row " + row_id + " in table.";
			logger.log(msg, LOG_ERROR);
		return 1;
		}

		// iterate columns of row
		iter_col = first_col;
		cell_ref = iter_col + row_id;
		iter_cell = iter_row.find_child_by_attribute(row_attr, cell_ref.c_str());

		// delete info in row
		for (int j = 0; j < xl_row_info.size(); j++){
			xl_row_info[j].clear();
			is_string[j] = 0;
		}

		// populate row with data from spreadsheet
		int col = 0;
		while (true){

			cell_ref = iter_col + row_id;
			if (iter_cell.attribute(row_attr).value() != cell_ref){
				iter_cell = iter_row.find_child_by_attribute(row_attr, cell_ref.c_str());
			}

			int is_string_cell = 0;
			if (!iter_cell){
				// could not find cell in defined range ?!
				cell_value = "";
				if (iter_col == first_col){
					msg = "Missing value for key column at cell ";
					msg += cell_ref;
					msg += " in range/sheet ";
					msg += table_name;
					logger.log(msg, LOG_ERROR);
					return DB_Error;
				}
			}
			else{
				//~ get_cell_val(iter_cell, cell_value);
				cell_value = "";
				cell_value = iter_cell.child("v").child_value();

				if (iter_cell.attribute("t").value() == std::string("s")){
					cell_value = shared_strings[std::atoi(cell_value.c_str())];
					is_string_cell = 1;
				}
				else if (iter_cell.attribute("t").value() == std::string("inlineStr")){
					cell_value = iter_cell.child("is").child("t").child_value();
					is_string_cell = 1;
				}
			}

			xl_row_info[col] = cell_value;
			is_string[col] = is_string_cell;

			if (iter_col == last_col){
				break;
			}
			ecm.next(iter_col);
			iter_cell = iter_cell.next_sibling();
			col += 1;
		}

		if (verbose == 73){
			printf("xl: ");
			print_vector(xl_row_info);
		}

		// pass data to ampl

		// fill arity cols values
		int pos = 0;
		for (int j = 0; j < arity_cols.size(); j++){
			pos = arity_cols[j];
			set_dbcol_val(xl_row_info[pos], &TI->cols[xl_to_ampl_cols[pos]], is_string[pos]);
		}

		// for each element in value_cols we add a row to the ampl table
		for (int j = 0; j < value_cols.size(); j++){

			pos = value_cols[j];

			if (xl_row_info[pos].empty()){
				continue;
			}

			set_dbcol_val(header[pos], &TI->cols[h_set_pos], is_header_string[pos]);
			set_dbcol_val(xl_row_info[pos], &TI->cols[TI->arity], is_string[pos]);

			if (verbose == 73){
				std::cout << "header: " << header[pos] << std::endl;
				std::cout << "val: " << xl_row_info[pos] << std::endl;
			}

			DbCol *db = TI->cols;
			if ((*TI->AddRows)(TI, db, 1)){
				msg = "Error with AddRows";
				logger.log(msg, LOG_DEBUG);
				return DB_Error;
			}
		}
		iter_row = iter_row.next_sibling();
	}

	std::clock_t end_time = get_time();
	double total_time = clock_to_seconds(start_time, end_time);

	msg = std::string("Parse data 2D done in ") +  numeric_to_fixed(total_time, CPUTIMES_NDIGITS) + std::string(" seconds");
	logger.log(msg, LOG_INFO);

	return 0;
};


int
ExcelWriteManager::write_data_inout_2D(
	pugi::xml_node node,
	int first_row,
	int last_row,
	std::string &first_col,
	std::string &last_col
){

	if (verbose > 0){
		printf("amplxl: write_data_INOUT_2D...\n");
	}
	std::clock_t start_time = get_time();

	std::string msg;

	pugi::xml_node excel_row;
	pugi::xml_node excel_cell;
	pugi::xml_node excel_val;
	pugi::xml_node dnode;

	// parse header info
	std::map<std::string, std::string> xl_col_map;
	parse_header(first_col, last_col, first_row, node, xl_col_map);

	std::string h_set;
	int h_set_pos = -1;

	if (xl_col_map.size() == 0){
		// no header provided write OUT table
		return write_data_out_2D(node, first_row, last_row, first_col, last_col);
	}
	// one of the columns will not appear in xl_col_map
	// this column elements will appear in the header

	std::string ampl_col_name;
	std::string ampl_col_value;
	bool found = false;

	for (int i = 0; i < TI->arity; i++){

		ampl_col_name = TI->colnames[i];

		if (xl_col_map.find(ampl_col_name) == xl_col_map.end()){

			if (found){
				// more than one column not mapped
			}

			found = true;
			h_set = ampl_col_name;
			h_set_pos = i;
		}
	}

	if (h_set_pos == -1){
		// could not find hset
	}

	msg = "hset: " + h_set + ", " + numeric_to_string(h_set_pos);
	logger.log(msg, LOG_DEBUG);

	ampl_to_excel_cols.resize(TI->arity + TI->ncols);
	for (int i = 0; i < TI->arity + TI->ncols; i++){

		if (i == h_set_pos){continue;}
		ampl_to_excel_cols[i] = xl_col_map[TI->colnames[i]];
	}

	int nkeys = TI->arity;

	// map rows and cells for faster access
	std::map<std::string, pugi::xml_node> row_map;
	std::map<std::string, pugi::xml_node> cell_map;
	get_maps(node, row_map, cell_map, logger);

	// check that all required cells exist
	check_table_cells(
		node,
		row_map,
		cell_map,
		first_row,
		last_row,
		first_col,
		last_col,
		logger
	);

	excel_keys.resize(nkeys);
	ampl_keys.resize(nkeys);

	// map xl rows according to arity keys and get last row of xl table
	// mapping only stops when we cannot find keys (assume blank row)
	std::map<std::vector<std::string>, pugi::xml_node> xl_key_map;
	std::map<std::vector<std::string>, int> xl_row_map;
	int xl_table_last_row = first_row;
	excel_row = get_excel_row(node, first_row);
	pugi::xml_node pg_table_last_row = excel_row;

	//~ std::cout << "get excel keys" << std::endl;

	std::string row_id_str;

	for (int i = first_row + 1; i <= EXCEL_MAX_ROWS; i++){

		row_id_str = numeric_to_string(i);

		if (excel_row.attribute(row_attr).value() != row_id_str){
			excel_row = get_excel_row(node, i);
		}

		int res = get_excel_keys_2D(excel_row, i, h_set_pos);

		if (res){
			// we could not find all the keys, assume end of table
			break;
		}

		std::cout << "xl_keys" << std::endl;
		print_vector(excel_keys);

		xl_key_map[excel_keys] = excel_row;
		xl_row_map[excel_keys] = i;
		xl_table_last_row = i;
		pg_table_last_row = excel_row;

		excel_row = excel_row.next_sibling();
	}


	// iterate AMPL table and write data to spreadsheet
	for (int i = 0; i < TI->nrows; i++){

		get_ampl_keys_2D(i, h_set_pos);

		std::cout << "ampl_keys" << std::endl;
		print_vector(ampl_keys);

		// get the corresponding row in xl table
		pugi::xml_node row_to_write;

		std::map<std::vector<std::string>, pugi::xml_node>::iterator it = xl_key_map.find(ampl_keys);
		if (it == xl_key_map.end()){

			std::cout << "could not find keys" << std::endl;
			print_vector(ampl_keys);
			return 1;

			// row is not mapped, append to table
			xl_table_last_row += 1;

			// check row already exists
			row_to_write = get_excel_row(node, xl_table_last_row);

			if (!row_to_write){
				row_to_write = node.insert_child_after("row", pg_table_last_row);
				row_to_write.append_attribute("r") = numeric_to_string(xl_table_last_row).c_str();
				pg_table_last_row = row_to_write;
			}

			// write cells of arity columns as the new/append row does not have it
			write_arity_cells(row_to_write, xl_table_last_row, i);
		}
		else{
			DbCol* db = &TI->cols[h_set_pos];

			std::string col_to_write;
			if (db->sval && db->sval[i]){
				col_to_write = db->sval[i];
			}
			else{
				col_to_write = numeric_to_string(db->dval[i]);
			}

			std::string row_ref = numeric_to_string(xl_row_map[ampl_keys]);
			std::string col_ref = xl_col_map[col_to_write];
			std::string cell_ref = col_ref + row_ref;

			msg = "cell to write inout: " + cell_ref;
			logger.log(msg, LOG_DEBUG);

			db = &TI->cols[TI->arity]; // the column with the value to write is the last one in the table
			pugi::xml_node xl_cell = cell_map[cell_ref];
			set_cell_value(db, i, xl_cell);
		}

		// write info
		copy_info(row_to_write, atoi(row_to_write.attribute("r").value()), i);
	}

	std::clock_t end_time = get_time();
	double total_time = clock_to_seconds(start_time, end_time);

	msg = std::string("Write data inout done in ") +  numeric_to_fixed(total_time, CPUTIMES_NDIGITS) + std::string(" seconds");
	logger.log(msg, LOG_INFO);

	return 0;
};



int
ExcelWriteManager::get_excel_keys_2D(
	pugi::xml_node excel_row,
	int row,
	int h_set_pos
){

	std::string msg;

	for (int i = 0; i < excel_keys.size(); i++){
		excel_keys[i].clear();
	}

	int nkeys = TI->arity;

	int pos = 0;
	for (int i = 0; i < nkeys; i++){

		// skip column that will be used as header row
		if (i == h_set_pos){
			continue;
		}

		std::string scol = ampl_to_excel_cols[i];
		pugi::xml_node excel_cell = get_xl_cell(excel_row, row, scol);

		if (!excel_cell){
			// could not find key
			msg = "Could not get spreadsheet keys from row";
			logger.log(msg, LOG_ERROR);
			return 1;
		}

		std::string value = excel_cell.child("v").child_value();

		if (excel_cell.attribute("t").value() == std::string("s")){
			value = shared_strings[std::atoi(value.c_str())];
		}
		else{
			// if the value is numeric its string representation might be different from amplxl
			// default(scientific) so we turn it to number and back to string for it to have the
			// same representation in the keys of a map.
			// Otherwise we could interpret same numbers as different and add unwanted
			// entries to inout tables

			char* se;
			double t;

			t = strtod(value.c_str(), &se);
			if (!*se) {/* valid number */
				value = numeric_to_scientific(t);
				// add prefix to avoid clash with strings with the same numeric value
				value = "ampl-numeric-" + value;
			}
			else{
				// could not convert number to numeric value
				msg = "Could not convert " + value + " to numeric";
				logger.log(msg, LOG_ERROR);
				return 1;
			}

		}



		//~ std::cout << "string value: " << value << std::endl;

		if (value.length() > 0){
			excel_keys[pos] = value;
			pos += 1;
		}
		else{
			// no value found
			msg = "Could not get spreadsheet keys";
			logger.log(msg, LOG_ERROR);
			return 1;
		}
	}

	//~ if (verbose == 73){
		//~ printf("excel_keys = [");
		//~ for (int i = 0; i < nkeys; i++){
			//~ printf("%s, ", excel_keys[i].c_str());
		//~ }
		//~ printf("]\n");
	//~ }


	//~ print_vector(excel_keys);

	return 0;



};


int
ExcelWriteManager::get_ampl_keys_2D(int line, int h_set){

	DbCol *db;

	for (int i = 0; i < ampl_keys.size(); i++){
		ampl_keys[i].clear();
	}

	int nkeys = TI->arity;
	db = TI->cols;

	int pos = 0;
	for (int i = 0; i < nkeys; i++){

		if (i == h_set){
			db++;
			continue;
		}

		if (db->sval && db->sval[line]){
			ampl_keys[pos] = std::string(db->sval[line]);
		}
		else{
			std::string value = numeric_to_scientific(db->dval[line]);
			// add prefix to avoid clash with strings with the same numeric value
			value = "ampl-numeric-" + value;
			ampl_keys[pos] = value;
		}
		db++;
		pos += 1;
	}
	return 0;
};


void
ExcelWriteManager::delete_range_values(
	pugi::xml_node parent,
	int first_row,
	int last_row,
	const std::string & first_col,
	const std::string & last_col
){
	std::string msg;

	msg = "Delete range values...";
	logger.log(msg, LOG_INFO);
	std::clock_t start_time = get_time();

	pugi::xml_node xl_row;
	pugi::xml_node xl_cell;
	std::string row_id;
	std::string iter_col;
	std::string cell_adress;

	//~ row_id = numeric_to_string(first_row);
	xl_row = get_excel_row(parent, first_row);

	// iterate rows
	for (int i = first_row; i <= last_row; i++){

		row_id = numeric_to_string(i);

		if (xl_row.attribute(row_attr).value() != row_id){
			xl_row = get_excel_row(parent, i);
		}

		if (xl_row){

			// iterate columns and delete cell (if found)
			iter_col = first_col;
			cell_adress = iter_col + row_id;
			xl_cell = xl_row.find_child_by_attribute(row_attr, cell_adress.c_str());

			while (1){
				cell_adress = iter_col + row_id;

				// get the cell element
				if (xl_cell.attribute(row_attr).value() != cell_adress){
					xl_cell = xl_row.find_child_by_attribute(row_attr, cell_adress.c_str());
				}

				if (xl_cell){

					pugi::xml_node xl_val = xl_cell.child("v");

					if (xl_val){
						xl_cell.remove_child(xl_val);
					}
				}

				if (iter_col == last_col){
					break;
				}

				xl_cell = xl_cell.next_sibling();
				ecm.next(iter_col);
			}
		}
		xl_row = xl_row.next_sibling();
	}

	std::clock_t end_time = get_time();
	double total_time = clock_to_seconds(start_time, end_time);

	msg = std::string("Delete range values done in ") +  numeric_to_fixed(total_time, CPUTIMES_NDIGITS) + std::string(" s.");
	logger.log(msg, LOG_INFO);
};


int
ExcelWriteManager::update_workbook(std::string & xl_copy_path
){

	std::string new_range;
	get_new_range(new_range);

	if (verbose > 0){
		std::cout << "new range: " << new_range << std::endl;
	}

	int result = 0;

	// extract workbook
	excel_iner_file = "xl/workbook.xml";
	result = myunzip(excel_path, excel_iner_file, temp_folder);

	if (result){
		// error extracting workbook
		//~ cannot_extract_workbook();
		return 1;
	}

	// get info from workbook
	excel_file = "workbook.xml";
	join_path(temp_folder, excel_file, final_path);

	pugi::xml_document doc;
	pugi::xml_node node;
	pugi::xml_parse_result presult;
	pugi::xml_node_iterator it;

	presult = doc.load_file(&final_path[0u]);

	if (!presult){
		//~ cannot_open_workbook();
		return 1;
	}

	// replace named range
	node = doc.child("workbook").child("definedNames");

	for (it = node.begin(); it != node.end(); ++it){

		if (it->attribute("name").value() == table_name){
			//~ excel_range = it->child_value();
			pugi::xml_text my_text = it->text();
			my_text.set(&new_range[0u]);
		}
	}


	if (verbose == 73){

		std::cout << "excel_path: " << excel_path << std::endl;
		std::cout << "excel_iner_file: " << excel_iner_file << std::endl;
		std::cout << "final_path: " << final_path << std::endl;
	}


	// update sheet xml
	doc.save_file(&final_path[0u]);

	// replace inside zip
	result = myzip(xl_copy_path, excel_iner_file, final_path);

	return 0;
};



int
ExcelWriteManager::get_new_range(std::string & new_range){

	std::string msg;

	new_range = range_sheet;
	new_range += "!$";
	new_range += range_first_col;
	new_range += "$";
	new_range += numeric_to_string(range_first_row);
	new_range += ":$";
	new_range += range_last_col;
	new_range += "$";

	if (tableType == TABLE_RANGE){
		new_range += numeric_to_string(range_last_row);
	}
	else if (tableType == TABLE_HEADER){
		new_range += numeric_to_string(range_first_row);
	}
	else{
		msg = "undefined table type for named range";
		logger.log(msg, LOG_ERROR);
	}

	msg = "new named range: " + new_range;
	logger.log(msg, LOG_DEBUG);

	return 0;
};


bool
ExcelWriteManager::validate_table_utf8_compatible(){

	unsigned char* temp_string;

	int ampl_ncols = TI->arity + TI->ncols;
	DbCol *db;

	// validate header
	for (int j = 0; j < ampl_ncols; j++){

		temp_string = reinterpret_cast<unsigned char *>(TI->colnames[j]);

		if (utf8_check(temp_string)){

			std::string msg = "Could not write invalid utf-8 column name \'";
			msg += TI->colnames[j];
			msg += "\' to spreadsheet";
			logger.log(msg, LOG_ERROR);
			return false;
		}
	}

	// validate data
	for (int i = 0; i < TI->nrows; i++){

		db = TI->cols;

		for (int j = 0; j < ampl_ncols; j++){

			if (db->sval && db->sval[i]){

				temp_string = reinterpret_cast<unsigned char *>(db->sval[i]);

				if (utf8_check(temp_string)){

					std::string msg = "Could not write invalid utf-8 table value \'";
					msg += db->sval[i];
					msg += "\' to spreadsheet";
					logger.log(msg, LOG_ERROR);
					return false;
				}
			}
			db++;
		}
	}
	return true;
}



