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
	"one or two strings (an optional 'ampl_xl' and the file name,\n"
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
	write = std::string("delete");
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
	return 0;
};


int
ExcelManager::prepare(){

	excel_path = get_excel_path(TI);

	if (excel_path.empty()){

		cannot_find_file();
		return 1;
	}

	table_name = TI->tname;

	std::string temp_string;
	std::string option_string;
	int has_alias = 0;
	int n = 0;

	// first string holds table handler name, second the file name
	// we need to parse remaining ones

	// first parse verbose only
	for (int i = 2; i < TI->nstrings; i++){

		temp_string = TI->strings[i];

		if (temp_string.substr(0, 8) == std::string("verbose=")){

			option_string = temp_string.substr(8, temp_string.size() - 8);
			std::istringstream iss(option_string);
			iss >> verbose;
		}
	}

	if (verbose > 0){
		printf("amplxl:\n");
		printf("\thandler: %s\n", TI->strings[0]);
		printf("\tfile: %s\n", TI->strings[1]);
		printf("\tverbose: %d\n", verbose);
	}

	// parse remaining args
	for (int i = 2; i < TI->nstrings; i++){

		temp_string = TI->strings[i];

		if (temp_string.substr(0, 8) == "verbose="){
			continue;
		}
		else if (temp_string.substr(0, 6) == std::string("write=")){

			option_string = temp_string.substr(6, temp_string.size() - 6);

			if (option_string == std::string("delete")){
				write = std::string("delete");
			}
			else if (option_string == std::string("drop")){
				write = std::string("drop");
			}
			else if (verbose > 0){
				printf("\tignoring write option: %s\n", TI->strings[i]);
			}
		}
		else{

			if (has_alias == 0){
				table_name = TI->strings[i];
				has_alias = 1;
				if (verbose > 0){
					printf("\tusing alias: %s\n", TI->strings[i]);
				}
			}
			else{
				if (verbose > 0){
					printf("\tignoring option: %s\n", TI->strings[i]);
				}
			}
		}
	}

	if (verbose > 0){
		printf("\twrite option: %s\n", &write[0u]);
		if (TI->flags == 1){
			printf("\tstatus: IN\n");
		}
		else if (TI->flags == 2){
			printf("\tstatus: OUT\n");
		}
		else if (TI->flags == 3){
			printf("\tstatus: INOUT\n");
		}
	}

	return 0;
};

int
ExcelManager::manage_workbook(){

	if (verbose > 1){
		printf("amplxl: manage workbook...\n");
	}

	int result = 0;

	// extract workbook
	excel_iner_file = "xl/workbook.xml";
	result = myunzip(&excel_path[0u], &excel_iner_file[0u], &temp_folder[0u]);

	if (result){
		// error extracting workbook
		cannot_extract_workbook();
		return 1;
	}

	// get info from workbook
	excel_file = "workbook.xml";
	join_path(temp_folder, excel_file, final_path);
	result = parse_workbook();

	if (result){
		// error parsing workbook
		return 1;
	}

	if (excel_range.empty()){
		// assume the name of the sheet equals the name of the table
		range_sheet = table_name;
	}
	else{
		has_range = true;
		result = parse_excel_range();

		if (result){
			return 1;
		}

	}

	//~ sheet_rel = sheet_rel_map[range_sheet];

	std::map<std::string,std::string>::iterator it = sheet_rel_map.find(range_sheet);
	if (it == sheet_rel_map.end()){
		// cannot find table
		cannot_find_table();
		return 1;
	}
	else{
		sheet_rel = it->second;
	}

	return 0;
};

int
ExcelManager::manage_relations(){

	int result = 0;

	// extract relations
	excel_iner_file = "xl/_rels/workbook.xml.rels";
	result = myunzip(&excel_path[0u], &excel_iner_file[0u], &temp_folder[0u]);

	if (result){
		// error extracting relations
		return 1;
	}

	// parse excel relations to get the actual name of the sheet
	excel_file = "workbook.xml.rels";
	join_path(temp_folder, excel_file, final_path);
	result = get_excel_sheet(final_path);

	if (result){
		// error parsing relations
		return 1;
	}

	return 0;
};

int
ExcelManager::manage_shared_strings(){

	int result = 0;

	// extract shared strings
	excel_iner_file = "xl/sharedStrings.xml";
	result = myunzip(&excel_path[0u], &excel_iner_file[0u], &temp_folder[0u]);

	if (result){
		cannot_extract_ss();
		return 1;
	}

	// load shared strings
	excel_file = "sharedStrings.xml";
	join_path(temp_folder, excel_file, final_path);
	result = get_shared_strings();

	if (result){
		// error parsing shared strings
		cannot_open_ss();
		return 1;
	}

	if (verbose == 73){
		printf("Shared strings:\n");
		for (int i=0; i< shared_strings.size(); i++){
			printf("\t%d, %s\n", i, &shared_strings[i][0u]);
		}
		printf("Shared strings done.\n");
	}

	return 0;
};

int
ExcelReadManager::manage_data(){

	if (verbose > 1){
		printf("amplxl: manage data...\n");
	}

	int result = 0;

	// extract sheet with data
	excel_iner_file = "xl/" + data_sheet;
	result = myunzip(&excel_path[0u], &excel_iner_file[0u], &temp_folder[0u]);

	if (result){
		// error extracting data
		cannot_extract_sheet();
		return 1;
	}

	excel_file = data_sheet.substr(data_sheet.find("/") + 1); 
	join_path(temp_folder, excel_file, final_path);

	pugi::xml_document doc;
	pugi::xml_node node;
	pugi::xml_parse_result pugi_result;
	pugi::xml_node_iterator it;
	pugi::xml_node row_child;
	pugi::xml_node excel_cell;
	const char* row_attr = "r";

	pugi_result = doc.load_file(&final_path[0u]);

	if (!pugi_result){
		cannot_open_sheet();
		return 1;
	}

	node = doc.child("worksheet").child("sheetData");

	int first_row = 1;
	int last_row = EXCEL_MAX_ROWS;
	std::string first_col = std::string("A");
	std::string last_col = EXCEL_MAX_COLS;

	if (has_range){
		first_row = range_first_row;
		first_col = range_first_col; 
		last_col = range_last_col; 
	}

	result = check_columns(node, first_row, first_col, last_col);

	if (result != -1){
		// error with column result
		cannot_find_column(result);
		return 1;
	}

	break_mode = true;
	if (has_range){
		if (range_first_row != range_last_row){
			break_mode = false;
			last_row = range_last_row;
		}
	}

	first_row += 1;
	result = parse_data(node, first_row, last_row, first_col, last_col);

	if (result){
		return 1;
	}

	return 0;
};

int
ExcelReadManager::run(){

	int result = 0;

	if (verbose == 73){
		inspect_ti(ae, TI);
	}

	result = prepare();
	if (result){
		return DB_Error;
	}

	result = create_temp_folder();
	if (result){
		return DB_Error;
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

	result = manage_data();
	if (result){
		return DB_Error;
	}

	result = clean_temp_folder();
	if (result){
		return DB_Error;
	}

	if (verbose > 1){
		printf("amplxl: all done!\n");
	}
	return DB_Done;
};

int
ExcelWriteManager::run(){

	int result = 0;

	if (verbose == 73){
		inspect_ti(ae, TI);
		inspect_values(ae, TI);
	}

	result = prepare();
	if (result){
		return DB_Error;
	}

	result = create_temp_folder();
	if (result){
		return DB_Error;
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

	result = manage_data();
	if (result){
		return DB_Error;
	}

	result = clean_temp_folder();
	if (result){
		return DB_Error;
	}

	if (verbose > 1){
		printf("amplxl: all done!\n");
	}
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

	pugi::xml_document doc;
	pugi::xml_node node;
	pugi::xml_parse_result result;
	pugi::xml_node_iterator it;

	result = doc.load_file(&final_path[0u]);

	if (!result){
		cannot_open_workbook();
		return 1;
	}

	// get the excel range of the given name
	node = doc.child("workbook").child("definedNames");

	for (it = node.begin(); it != node.end(); ++it){

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
	}
	return 0;
};

int
ExcelManager::parse_excel_range(){

	char* to_split = &excel_range[0u];
	char* pch;

	pch = strtok(to_split, "!$:;");

	while(pch != NULL){
		split.push_back(pch);
		//~ std::cout << "pch: " << pch << std::endl;
		pch = strtok(NULL, "!$:;");
	}


	if (split.size() != 5){
		// range should have 5 elements
		//Error could not parse range
		cannot_parse_range();
		return 1;
	}

	try{
		range_sheet = std::string(split[0]);
		range_first_col = std::string(split[1]);
		range_last_col = std::string(split[3]);
		range_first_row = atoi(split[2]);
		range_last_row = atoi(split[4]);
	}
	catch(int e){
		// could not convert one of the elements
		cannot_parse_range();
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
	const std::string &last_col
){
	const int ampl_ncols = TI->arity + TI->ncols;
	ampl_to_excel_cols.resize(ampl_ncols);

	const char* row_attr = "r";

	std::stringstream strs;
	strs << first_row;
	std::string row_id_str = strs.str();
	char* row_id = &row_id_str[0u];

	pugi::xml_node row_child = node.find_child_by_attribute(row_attr, row_id);
	pugi::xml_node excel_cell;

	std::string ampl_col_name;
	std::string excel_col_name;
	std::string cell_adress;
	std::string iter_col;
	bool found = false;


	std::map<std::string, std::string> excel_col_map;

	int nempty = 0; // number of empty columns parsed
	const int max_empty = 100; // maximum number of empty columns allowed


	iter_col = first_col;
	while(1){

		cell_adress = iter_col + row_id_str;
		excel_cell = row_child.find_child_by_attribute(row_attr, &cell_adress[0u]);
		excel_col_name = excel_cell.first_child().child_value();

		if (excel_cell.attribute("t").value() == std::string("s")){
			excel_col_name = shared_strings[std::atoi(excel_col_name.c_str())];
		}
		else if (excel_cell.attribute("t").value() == std::string("inlineStr")){
			excel_col_name = excel_cell.first_child().first_child().child_value();
		}

		if (!excel_col_name.empty()){
			excel_col_map[excel_col_name] = iter_col;
			nempty = 0;

			if (verbose == 73){
				printf("Found column %s\n", &excel_col_name[0u]);
			}

		}
		else{
			nempty += 1;
		}

		if (nempty == max_empty){
			if (verbose > 1){
				printf("cannot find columns, search done.\n");
			}
			break;
		}

		if (iter_col == range_last_col){
			break;
		}

		ecm.next(iter_col);
	}

	for (int i = 0; i < ampl_ncols; i++){

		ampl_col_name = TI->colnames[i];

		std::map<std::string,std::string>::iterator it = excel_col_map.find(ampl_col_name);

		if (it != excel_col_map.end()){
			ampl_to_excel_cols[i] = it->second;
		}
		else{
			return i;
		}
	}
	return -1;
};


int
ExcelManager::get_excel_sheet(std::string &path){

	pugi::xml_document doc;
	pugi::xml_node node;
	pugi::xml_parse_result result;
	pugi::xml_node_iterator it;

	result = doc.load_file(&path[0u]);

	if (!result){
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

	pugi::xml_document doc;
	pugi::xml_node node;
	pugi::xml_parse_result result;
	pugi::xml_node_iterator it;

	result = doc.load_file(&final_path[0u]);

	if (!result){
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
	printf("flags: %s\n", TI->flags);
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
	int ampl_ncols = TI->ncols + TI->arity;
	DbCol *db;

	std::string iter_col = first_col;
	std::vector<std::string> temp_strings(ampl_ncols);

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
		}

		bool has_content = false;

		for(int j = 0; j < ampl_ncols; j++){

			iter_col = ampl_to_excel_cols[j];

			// concatenate iter_col and i to get the excel cell information
			std::string cell_adress = iter_col + row_id_str;

			// get the element
			excel_cell = row_child.find_child_by_attribute(row_attr, &cell_adress[0u]);

			if (excel_cell){

				// check if the cell holds a string or a numeric value
				// if the node has a "t" attribute with value "s" it's child node holds the position of a shared string
				// if the attribute is not present the child holds a numeric value

				std::string value = excel_cell.first_child().child_value();

				if (excel_cell.attribute("t").value() == std::string("s")){
					value = shared_strings[std::atoi(value.c_str())];
				}
				else if (excel_cell.attribute("t").value() == std::string("inlineStr")){
					value = excel_cell.first_child().first_child().child_value();
				}

				if (value.length() > 0){
					temp_strings[j] = value;
					has_content = true;
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
				db->sval[0] = TI->Missing;
			}
			else{
				//dont trust excel, always try to convert the value
				t = strtod(&temp_strings[j][0u], &se);
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
			db++;
		}

		db = TI->cols;
		if ((*TI->AddRows)(TI, db, 1)){
			return DB_Error;
		}

		row_child = row_child.next_sibling();

	}

	return 0;
};



void
ExcelWriteManager::get_sstrings_map(){

	for (int i=0; i< shared_strings.size(); i++){
		sstrings_map[shared_strings[i]] = i;
	}

};


int
ExcelWriteManager::manage_data(){

	if (verbose > 1){
		printf("amplxl: manage data...\n");
	}

	int result = 0;

	// extract sheet with data
	excel_iner_file = "xl/" + data_sheet;
	result = myunzip(&excel_path[0u], &excel_iner_file[0u], &temp_folder[0u]);

	if (result){
		// error extracting data
		cannot_extract_sheet();
		return 1;
	}

	excel_file = data_sheet.substr(data_sheet.find("/") + 1); 
	join_path(temp_folder, excel_file, final_path);

	std::string sheet_final_path = final_path;

	pugi::xml_document doc;
	pugi::xml_node node;
	pugi::xml_parse_result pugi_result;
	pugi::xml_node_iterator it;
	pugi::xml_node row_child;
	pugi::xml_node excel_cell;
	const char* row_attr = "r";

	pugi_result = doc.load_file(&final_path[0u]);


	if (!pugi_result){
		// could not load xml
		cannot_open_sheet();
		return 1;
	}

	node = doc.child("worksheet").child("sheetData");

	int first_row = 1;
	int last_row = EXCEL_MAX_ROWS;
	std::string first_col = std::string("A");
	std::string last_col = EXCEL_MAX_COLS;


	if (has_range){
		first_row = range_first_row;
		first_col = range_first_col; 
		last_col = range_last_col;
	}
	last_row = first_row + TI->nrows;

	// map shared strings for fast access and get the number of existing strings, since we may add
	// more strings later and need to update the file in excel
	get_sstrings_map();
	int n_sstrings = shared_strings.size();

	if (TI->flags == 2){

		if (write == std::string("delete")){

			result = check_columns(node, first_row, first_col, last_col);

			if (result != -1){
				cannot_find_column(result);
				return 1;
			}

			first_row += 1;

			if (!result){
				// we needed to add some rows
			}

			delete_data(node);

			result = check_rows(node, first_row, last_row);

			range_last_row = last_row;

			result = write_data_out(node, first_row, last_row, first_col, last_col);

		}
		else if (write == std::string("drop")){

			// delete all info, including header
			delete_data(node);

			result = check_rows(node, first_row, last_row);

			write_header(node, first_row, first_col);

			first_row += 1;
			result = write_all_data_out(node, first_row, last_row, first_col, last_col);
		}
	}
	else if (TI->flags == 3){

		result = check_columns(node, first_row, first_col, last_col);

		if (result != -1){
			cannot_find_column(result);
			return 1;
		}

		first_row += 1;

		result = write_data_inout(node, first_row, last_row, first_col, last_col);
	}
	else{
		// unsuported write flag
		unsuported_flag();
		return 1;
	}

	if (result){
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

	doc.save_file(&final_path[0u]);

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
			cannot_update_ss();
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

	result = myzip(&xl_copy_path[0u], &excel_iner_file[0u], &final_path[0u]);
	if (result){
		cannot_update_sheet();
		return 1;
	}

	if (shared_strings.size() > n_sstrings){

		excel_iner_file = "xl/sharedStrings.xml";
		excel_file = "sharedStrings.xml";

		join_path(temp_folder, excel_file, final_path);

		result = myzip(&xl_copy_path[0u], &excel_iner_file[0u], &final_path[0u]);
		if (result){
			cannot_update_sheet();
			return 1;
		}
	}

	remove(&excel_path[0u]);
	if (rename(&xl_copy_path[0u], &excel_path[0u]) != 0)
	{
		// cannot swap files
		return 1;
	}

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
	pugi::xml_node excel_row;
	pugi::xml_node excel_cell;
	pugi::xml_node excel_val;
	pugi::xml_node dnode;

	const char* row_attr = "r";
	std::stringstream strs;
	std::string row_id_str;

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

		// we have the row now we need to check if it has the column nodes
		for (int j = 0; j < ampl_ncols; j++){

			std::string scol = ampl_to_excel_cols[j];
			excel_cell = get_excel_cell(excel_row, i, scol);

			set_cell_value(db, excel_row, excel_cell, scol, i, trow);

			db++;
		}
		trow += 1;
		excel_row = excel_row.next_sibling();
	}
	return 0;
};


int
ExcelWriteManager::write_all_data_out(
	pugi::xml_node node,
	int first_row,
	int last_row,
	std::string &first_col,
	std::string &last_col
){

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

			excel_cell = get_excel_cell(excel_row, i, iter_col);
			set_cell_value(db, excel_row, excel_cell, iter_col, i, trow);

			db++;
			ecm.next(iter_col);
		}
		trow += 1;
		excel_row = excel_row.next_sibling();

	}
	return 0;
};


int
ExcelWriteManager::get_excel_keys(pugi::xml_node excel_row, int row){

	for (int i = 0; i < excel_keys.size(); i++){
		excel_keys[i].clear();
	}

	int nkeys = TI->arity;

	for (int i = 0; i < nkeys; i++){

		std::string scol = ampl_to_excel_cols[i];
		pugi::xml_node excel_cell = get_excel_cell(excel_row, row, scol);

		if (!excel_cell){
			// could not find key
			cannot_find_keys();
			return 1;
		}

		std::string value = excel_cell.first_child().child_value();

		//~ std::cout << "string value: " << value << std::endl;

		if (excel_cell.attribute("t").value() == std::string("s")){
			value = shared_strings[std::atoi(value.c_str())];
		}

		if (value.length() > 0){
			excel_keys[i] = value;
		}
		else{
			// no value found
			cannot_find_keys();
			return 1;
		}
	}

	if (verbose == 73){
		printf("excel_keys = [");
		for (int i = 0; i < nkeys; i++){
			printf("%s, ", &excel_keys[i][0u]);
		}
		printf("]\n");
	}

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

		if (db->sval){
			ampl_keys[i] = std::string(db->sval[line]);
		}
		else{
			std::stringstream strs;
			strs << db->dval[line];
			ampl_keys[i] = strs.str();
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

	if (verbose == 73){
		printf("ExcelWriteManager::write_data_inout\n");
		printf("\tfirst row: %d\n", first_row);
		printf("\tlast row: %d\n", last_row);
		printf("\tfirst col: %s\n", &first_col[0u]);
		printf("\tlast col: %s\n", &last_col[0u]);
	}

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

	excel_row = get_excel_row(node, first_row);

	for (int i = first_row; i <= last_row; i++){

		strs.str(std::string()); // clear stringstream
		strs << i;
		row_id_str = strs.str();

		if (verbose == 73){
			printf("cell id: %s\n", &row_id_str[0u]);
		}

		if (excel_row.attribute(row_attr).value() != row_id_str){
			excel_row = get_excel_row(node, i);
		}

		int res = get_excel_keys(excel_row, i);

		if (res){
			// we could not find all the keys
			return 1;
		}

		int ampl_row = get_ampl_row();

		if (ampl_row == -1){
			// we could not find an ampl row with the provided excel keys
			return 1;
		}

		copy_info(excel_row, i, ampl_row);
		excel_row = excel_row.next_sibling();
	}
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
		pugi::xml_node excel_cell = get_excel_cell(excel_row, row, scol);

		set_cell_value(db, excel_row, excel_cell, scol, row, ampl_row);

		db++;
	}
	return 0;
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
get_excel_cell(pugi::xml_node parent, int row, std::string &col){

	const char* row_attr = "r";
	std::stringstream strs;
	strs << row;
	std::string row_id_str = strs.str();

	std::string cell_id_str = col + row_id_str;
	char* cell_id = &cell_id_str[0u];

	return parent.find_child_by_attribute(row_attr, cell_id);
};



int
ExcelWriteManager::check_shared_strings(std::string s){

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

	result = doc.load_file(&final_path[0u]);

	if (!result){
		return 1;
	}

	node = doc.child("sst");

	for (int i = init_size; i < shared_strings.size(); i++){

		si_node = node.append_child("si");
		t_node = si_node.append_child("t");
		t_node.append_child(pugi::node_pcdata).set_value(&shared_strings[i][0u]);
	}

	// update sheet xml
	doc.save_file(&final_path[0u]);

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
		cannot_create_temp();
		return 1;
	}

	// template for unique filename
	std::string tplt = std::string("ampltemp-XXXXXXXXXXXX");
	// fill the Xs after the 9th character
	mymkstemp(tplt, 9);

	std::string full_path = temp_path + std::string("\\") + tplt;

	res = CreateDirectory(&full_path[0u], NULL);

	if (res == 0){
		cannot_create_temp();
		return 1;
	}

	temp_folder = full_path;

#else
	char tp[] = "/tmp/ampltempXXXXXX";
	char *dir_name = mkdtemp(tp);


	if (dir_name == NULL){
		cannot_create_temp();
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

		if (verbose > 0){
			if (res_del == 0){
				std::cout << "could not delete file: " << GetLastError() << std::endl;
			}
		}

	} while (FindNextFile(hFind, &ffd));

	int res_rem = RemoveDirectory(&temp_folder[0u]);

	if (verbose > 0){
		if (res_rem == 0){
			std::cout << "could not remove folder: " << GetLastError() << std::endl;
		}
	}

#else
	std::string test_string = std::string("/tmp/ampltemp");
	if (temp_folder.substr(0, test_string.size()) == test_string){

		std::string my_com = std::string("exec rm -r ") + temp_folder;
		int res_rem = system(my_com.c_str());

	if (verbose > 0){
		if (res_rem != 0){
			printf("Could not remove temporary folder.\n");
		}
	}
	}
#endif

	return 0;
};








void
ExcelWriteManager::set_cell_value(

	DbCol *db,
	pugi::xml_node excel_row,
	pugi::xml_node excel_cell,
	std::string &scol,
	int i,
	int trow
){

	pugi::xml_node excel_val;
	pugi::xml_node dnode;

	std::stringstream strs;
	std::string temp_str;


	if (!excel_cell){
		excel_cell = excel_row.append_child("c");
		strs.str(std::string());
		strs << i;
		std::string srow = strs.str();
		std::string scell = scol + srow;
		excel_cell.append_attribute("r") = &scell[0u];
	}

	// excel cell has a child of type "v"
	excel_val = excel_cell.first_child();
	if(!excel_val){
		excel_val = excel_cell.append_child("v");
	}

	bool is_str = false;

	// check value 
	if (db->sval){
		int sstring_pos = check_shared_strings(std::string(db->sval[trow]));
		strs.str(std::string());
		strs << sstring_pos;
		temp_str = strs.str();
		is_str = true;
	}
	else{
		strs.str(std::string());
		strs << db->dval[trow];
		temp_str = strs.str();
	}

	// check cell data type ("t" attribute)
	if (is_str){
		pugi::xml_attribute attr = excel_cell.attribute("t");
		if (attr){
			attr.set_value("s");
		}
		else{
			excel_cell.append_attribute("t") = "s";
		}
	}

	dnode = excel_val.first_child();
	if (!dnode){
		excel_val.append_child(pugi::node_pcdata).set_value(&temp_str[0u]);
	}
	else{
		dnode.set_value(&temp_str[0u]);
	}

};

void
ExcelManager::cannot_find_file()
{
	sprintf(TI->Errmsg = (char*)TM(33),
		"Cannot find .xlsx or .xlsm files.");
};
void
ExcelManager::cannot_create_temp()
{
	sprintf(TI->Errmsg = (char*)TM(31),
		"Cannot create temporary folder.");
};


void
ExcelManager::cannot_extract_workbook(){
	sprintf(TI->Errmsg = (char*)TM(24),
		"Cannot extract workbook.");
};


void
ExcelManager::cannot_open_workbook(){
	sprintf(TI->Errmsg = (char*)TM(21),
		"Cannot open workbook.");
};


void
ExcelManager::cannot_parse_range(){

	sprintf(TI->Errmsg = (char*)TM(strlen(&excel_range[0u]) + 26),
		"Cannot parse range \"%s\".", &excel_range[0u]);
};


void
ExcelManager::cannot_find_table(){
	sprintf(TI->Errmsg = (char*)TM(strlen(&table_name[0u]) + 25),
		"Cannot find table \"%s\".", &table_name[0u]);
};


void
ExcelManager::cannot_extract_ss(){
	sprintf(TI->Errmsg = (char*)TM(21),
		"Cannot extract shared strings.");
};

void
ExcelManager::cannot_open_ss(){
	sprintf(TI->Errmsg = (char*)TM(21),
		"Cannot open shared strings.");
};




void
ExcelManager::cannot_extract_sheet(){
	sprintf(TI->Errmsg = (char*)TM(21),
		"Cannot extract sheet.");

};

void
ExcelManager::cannot_open_sheet(){
	sprintf(TI->Errmsg = (char*)TM(21),
		"Cannot open sheet.");

};


void
ExcelManager::cannot_find_column(int col){

	sprintf(TI->Errmsg = (char*)TM(strlen(TI->colnames[col]) + 26),
		"Cannot find column \"%s\".", TI->colnames[col]);


};


void
ExcelManager::cannot_find_keys(){
	sprintf(TI->Errmsg = (char*)TM(17),
		"Cannot find keys.");

};


void
ExcelManager::cannot_update_ss(){
	sprintf(TI->Errmsg = (char*)TM(29),
		"Cannot update shared strings.");

};



void
ExcelManager::cannot_update_sheet(){
	sprintf(TI->Errmsg = (char*)TM(20),
		"Cannot update sheet.");
};



void
ExcelManager::unsuported_flag(){
	sprintf(TI->Errmsg = (char*)TM(19),
		"Cannot eval option.");
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
};

int
ExcelWriteManager::delete_range(pugi::xml_node parent, int include_header){

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
			while (1){
				std::string cell_adress = iter_col + row_id_str;

				// get the cell element
				excel_cell = excel_row.find_child_by_attribute(row_attr, &cell_adress[0u]);

				if (excel_cell){
					excel_row.remove_child(excel_cell);
				}

				if (iter_col == range_last_col){
					break;
				}

				ecm.next(iter_col);
			}
		}
		excel_row = excel_row.next_sibling();
	}
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

	pugi::xml_node row_child = parent.find_child_by_attribute(row_attr, &row_id[0u]);

	while (1){

		strs.str(std::string()); // clear stringstream
		strs << iter_row;
		row_id = strs.str();

		if (row_child.attribute(row_attr).value() != row_id){
			row_child = parent.find_child_by_attribute(row_attr, &row_id[0u]);
		}

		bool has_content = false;

		std::string iter_col = range_first_col;

		while (1){

			// concatenate iter_col and i to get the excel cell information
			std::string cell_adress = iter_col + row_id;

			// get the element
			pugi::xml_node excel_cell = row_child.find_child_by_attribute(row_attr, &cell_adress[0u]);

			if (excel_cell){

				std::string value = excel_cell.first_child().child_value();

				if (excel_cell.attribute("t").value() == std::string("s")){
					value = shared_strings[std::atoi(value.c_str())];
				}
				else if (excel_cell.attribute("t").value() == std::string("inlineStr")){
					value = excel_cell.first_child().first_child().child_value();
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
};


int
ExcelWriteManager::write_header(pugi::xml_node parent, int first_row, std::string & first_col){

	// get the header row
	const char* row_attr = "r";

	std::stringstream strs;
	strs << first_row;
	std::string row_id = strs.str();

	pugi::xml_node excel_row = parent.find_child_by_attribute(row_attr, &row_id[0u]);

	std::string iter_col = first_col;

	for (int i = 0; i < TI->arity + TI->ncols; i++){

		std::string wstr = std::string(TI->colnames[i]);
		int pos = check_shared_strings(wstr);

		strs.str(std::string());
		strs << pos;

		std::string cell_adress = iter_col + row_id;

		// get the cell element
		pugi::xml_node excel_cell = excel_row.find_child_by_attribute(row_attr, &cell_adress[0u]);

		if (!excel_cell){
			excel_cell = excel_row.append_child("c");
			excel_cell.append_attribute("r") = &cell_adress[0u];
		}

		pugi::xml_node excel_val = excel_cell.first_child();
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

};
