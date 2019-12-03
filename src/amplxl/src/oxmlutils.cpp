#include "oxmlutils.hpp"


int
update_date_created(std::string & current_date, std::string & temp_string){

	pugi::xml_document doc;
	pugi::xml_node node;
	pugi::xml_parse_result result;

	result = doc.load_file(temp_string.c_str());

	if (!result){
		return 1;
	}

	node = doc.child("cp:coreProperties").child("dcterms:created");
	node.first_child().set_value(current_date.c_str());
	doc.save_file(temp_string.c_str());

	return 0;
};


int
update_date_modified(std::string & current_date, std::string & temp_string){

	pugi::xml_document doc;
	pugi::xml_node node;
	pugi::xml_parse_result result;

	result = doc.load_file(temp_string.c_str());

	if (!result){
		return 1;
	}

	node = doc.child("cp:coreProperties").child("dcterms:modified");
	node.first_child().set_value(current_date.c_str());
	doc.save_file(temp_string.c_str());

	return 0;
};



std::string
get_current_date(){

	std::time_t now = std::time(0);
	std::tm* now_tm = std::gmtime(&now);
	char buf[100];
	std::strftime(buf, 100, "%Y-%m-%dT%H:%M:%SZ", now_tm);
	return std::string(buf);

};


std::string
get_current_date2(){

	std::time_t now = std::time(0);
	std::tm* now_tm = std::gmtime(&now);
	char buf[100];
	std::strftime(buf, 100, "%Y%m%d%H%M%S", now_tm);
	return std::string(buf);

};




void
join_path(const std::string & temp_folder, const std::string & excel_file, std::string & path){

	path = temp_folder;
#if defined _WIN32 || defined _WIN64
	path += "\\";
#else
	path += "/";
#endif
	path += excel_file;

};




int
build_oxml_file(std::string & oxml, std::string & temp_folder){

	std::vector<std::string> xml_content;

	xml_content.push_back(content_types_xml);
	xml_content.push_back(xl_rels_workbook_xml_rels);
	xml_content.push_back(xl_worksheets_sheet1_xml);
	xml_content.push_back(xl_styles_xml);
	xml_content.push_back(xl_workbook_xml);
	xml_content.push_back(rels_rels);
	xml_content.push_back(docProps_app_xml);
	xml_content.push_back(docProps_core_xml);

	std::vector<std::string> zip_orig(char_zip_orig, char_zip_orig + sizeof(char_zip_orig)/sizeof(char_zip_orig[0]));
	std::vector<std::string> zip_dest(char_zip_dest, char_zip_dest + sizeof(char_zip_dest)/sizeof(char_zip_dest[0]));

	std::ofstream out;
	std::string temp_string;

	// create xml files in temp folder
	for (int i = 0; i < xml_content.size(); i++){
		join_path(temp_folder, zip_orig[i], temp_string);
		reuse_ofstream(out, xml_content[i], temp_string);
	}

	// update date
	std::string current_date = get_current_date();
	join_path(temp_folder, "docProps_core_xml", temp_string);
	update_date_created(current_date, temp_string);

	// zip files
	int res = zip_xml_files(
		oxml,
		temp_folder,
		zip_orig,
		zip_dest
	);

	if (res){
		return 1;
	}

	// delete xml files in temp folder
	for (int i = 0; i < xml_content.size(); i++){
		join_path(temp_folder, zip_orig[i], temp_string);
		remove(temp_string.c_str());
	}

	return 0;
};


int
add_new_sheet_to_oxml(std::string & oxml, std::string & sheet_name, std::string & temp_folder){

	int res = 0;

	int new_sheet_number = get_last_sheet_number(oxml, temp_folder) + 1;

	if (new_sheet_number == -1){
		return 1;
	}

	res = add_sheet(oxml, new_sheet_number, temp_folder);

	if (res){
		return 1;
	}

	int new_rel_number = get_last_relation_number(oxml, temp_folder) + 1;

	if (new_rel_number == -1){
		return 1;
	}

	res = add_sheet_update_relations(oxml, new_sheet_number, new_rel_number, sheet_name, temp_folder);

	if (res){
		return 1;
	}

	return 0;
};


int
add_shared_strings_to_oxml(std::string & oxml, std::string & temp_folder){

	int res = 0;
	res = add_shared_strings_file(oxml, temp_folder);

	if (res){
		return 1;
	}

	int new_rel_number = get_last_relation_number(oxml, temp_folder) + 1;

	if (new_rel_number == -1){
		return 1;
	}

	res = add_shared_strings_update_relations(oxml, new_rel_number, temp_folder);

	if (res){
		return 1;
	}

	return 0;
}

int
add_shared_strings_update_relations(std::string & oxml, int new_rel_number, std::string & temp_folder){

	int res = 0;

	std::string new_file = oxml + "new.xlsx";

	std::vector<std::string> changed_files;
	std::vector<std::string> new_files;

	changed_files.push_back("[Content_Types].xml");
	changed_files.push_back("xl/_rels/workbook.xml.rels");


	res = copy_uchanged_files(
		oxml,
		new_file,
		changed_files
	);

	if (res){
		return 1;
	}

	std::string part_name = "/xl/sharedStrings.xml";
	std::string content_type = "application/vnd.openxmlformats-officedocument.spreadsheetml.sharedStrings+xml";

	res = update_content_types(oxml, part_name, content_type, temp_folder);
	res = add_shared_strings_update_xl_rels_workbook(oxml, new_rel_number, temp_folder);

	std::string ct = "[Content_Types].xml";
	std::string fpath;
	join_path(temp_folder, ct, fpath);
	int result = myzip(new_file, ct, fpath);

	remove(fpath.c_str());

	ct = "xl/_rels/workbook.xml.rels";
	join_path(temp_folder, "workbook.xml.rels", fpath);
	result = myzip(new_file, ct, fpath);

	remove(fpath.c_str());
	my_copy_file(new_file, oxml);
	remove(new_file.c_str());

	return 0;


};


int add_sheet_update_relations(std::string & oxml, int new_sheet_number, int new_rel_number, std::string & sheet_name, std::string & temp_folder){


	std::string new_file = oxml + "new.xlsx";

	std::vector<std::string> changed_files;
	std::vector<std::string> new_files;

	changed_files.push_back("[Content_Types].xml");
	changed_files.push_back("xl/workbook.xml");
	changed_files.push_back("xl/_rels/workbook.xml.rels");

	int t = copy_uchanged_files(
		oxml,
		new_file,
		changed_files
	);

	std::stringstream strs;
	strs << new_sheet_number;

	std::string part_name = "/xl/worksheets/sheet" + strs.str() + ".xml";
	std::string content_type = "application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml";

	update_content_types(oxml, part_name, content_type, temp_folder);
	add_sheet_update_xl_workbook(oxml, new_sheet_number, new_rel_number, sheet_name, temp_folder);
	add_sheet_update_xl_rels_workbook(oxml, new_sheet_number, new_rel_number, temp_folder);


	std::string ct = "[Content_Types].xml";
	std::string fpath;
	join_path(temp_folder, ct, fpath);
	int result = myzip(new_file, ct, fpath);

	remove(fpath.c_str());

	ct = "xl/workbook.xml"; 
	join_path(temp_folder, "workbook.xml", fpath);
	result = myzip(new_file, ct, fpath);

	remove(fpath.c_str());

	ct = "xl/_rels/workbook.xml.rels"; 

	join_path(temp_folder, "workbook.xml.rels", fpath);
	result = myzip(new_file, ct, fpath);

	remove(fpath.c_str());
	my_copy_file(new_file, oxml);
	remove(new_file.c_str());

	return 0;
};


int
update_content_types(std::string & oxml, std::string & part_name, std::string & content_type, std::string & temp_folder){

	int res = 0;

	std::string file_in_zip = "[Content_Types].xml";
	res = myunzip(oxml.c_str(), file_in_zip.c_str(), temp_folder.c_str());

	if (res){
		return 1;
	}

	std::string file_in_temp;
	join_path(temp_folder, file_in_zip, file_in_temp);

	pugi::xml_document doc;
	pugi::xml_node node;
	pugi::xml_parse_result result;

	result = doc.load_file(file_in_temp.c_str());

	if (!result){
		return 1;
	}

	node = doc.child("Types");
	node.append_child("Override");
	node.last_child().append_attribute("PartName") = part_name.c_str();
	node.last_child().append_attribute("ContentType") = content_type.c_str();

	doc.save_file(file_in_temp.c_str());

	return 0;
};



int
add_sheet_update_xl_workbook(std::string & oxml, int new_sheet_number, int new_rel_number, std::string & sheet_name, std::string & temp_folder){

	int res = 0;

	std::stringstream strs;
	strs << new_sheet_number;

	std::string file_in_zip = "xl/workbook.xml";
	res = myunzip(oxml.c_str(), file_in_zip.c_str(), temp_folder.c_str());

	if (res){
		return 1;
	}

	std::string file_in_temp;
	join_path(temp_folder, "workbook.xml", file_in_temp);

	pugi::xml_document doc;
	pugi::xml_node node;
	pugi::xml_parse_result result;
	pugi::xml_node_iterator it;

	result = doc.load_file(file_in_temp.c_str());

	if (!result){
		return 1;
	}

	node = doc.first_child().child("sheets");
	node.append_child("sheet");
	node.last_child().append_attribute("name") = sheet_name.c_str();

	std::string sheet_id = strs.str();
	node.last_child().append_attribute("sheetId") = sheet_id.c_str();
	node.last_child().append_attribute("state") = "visible";

	strs.str(std::string());
	strs << new_rel_number;

	std::string rId = "rId" + strs.str();
	node.last_child().append_attribute("r:id") = rId.c_str();

	doc.save_file(file_in_temp.c_str());

	return 0;
};


int
add_sheet_update_xl_rels_workbook(std::string & oxml, int new_sheet_number, int new_rel_number, std::string & temp_folder){

	int res = 0;

	std::stringstream strs;
	strs << new_sheet_number;

	std::string file_in_zip = "xl/_rels/workbook.xml.rels";
	res = myunzip(oxml.c_str(), file_in_zip.c_str(), temp_folder.c_str());

	if (res){
		return 1;
	}

	std::string file_in_temp;
	join_path(temp_folder, "workbook.xml.rels", file_in_temp);

	pugi::xml_document doc;
	pugi::xml_node node;
	pugi::xml_parse_result result;
	pugi::xml_node_iterator it;

	result = doc.load_file(file_in_temp.c_str());

	if (!result){
		return 1;
	}

	node = doc.child("Relationships");

	if (!node){
		return 1;
	}

	node.append_child("Relationship");

	strs.str(std::string());
	strs << new_rel_number;

	std::string rId = "rId" + strs.str();
	node.last_child().append_attribute("Id") = rId.c_str();

	node.last_child().append_attribute("Type") = "http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet";

	strs.str(std::string());
	strs << new_sheet_number;

	std::string target_value = "worksheets/sheet" + strs.str() + ".xml";

	node.last_child().append_attribute("Target") = target_value.c_str();

	doc.save_file(file_in_temp.c_str());

	return 0;
};


int
add_shared_strings_update_xl_rels_workbook(std::string & oxml, int new_rel_number, std::string & temp_folder){

	int res = 0;

	std::stringstream strs;

	std::string file_in_zip = "xl/_rels/workbook.xml.rels";
	res = myunzip(oxml.c_str(), file_in_zip.c_str(), temp_folder.c_str());

	if (res){
		return 1;
	}

	std::string file_in_temp;
	join_path(temp_folder, "workbook.xml.rels", file_in_temp);

	pugi::xml_document doc;
	pugi::xml_node node;
	pugi::xml_parse_result result;
	pugi::xml_node_iterator it;

	result = doc.load_file(file_in_temp.c_str());

	if (!result){
		return 1;
	}

	node = doc.child("Relationships");

	if (!node){
		return 1;
	}

	node.append_child("Relationship");

	strs.str(std::string());
	strs << new_rel_number;

	std::string rId = "rId" + strs.str();
	node.last_child().append_attribute("Id") = rId.c_str();
	node.last_child().append_attribute("Type") = "http://schemas.openxmlformats.org/officeDocument/2006/relationships/sharedStrings";
	std::string target_value = "sharedStrings.xml";
	node.last_child().append_attribute("Target") = target_value.c_str();

	doc.save_file(file_in_temp.c_str());
	return 0;
};


int
add_sheet(std::string & oxml, int new_sheet_number, std::string & temp_folder){

	int res = 0;

	std::stringstream strs;
	strs << new_sheet_number;

	std::string new_sheet_name = "sheet" + strs.str() + ".xml";
	std::string new_sheet_file;
	join_path(temp_folder, new_sheet_name, new_sheet_file);

	std::ofstream ofs(new_sheet_file.c_str());
	ofs << xl_worksheets_sheet2_xml;
	ofs.close();

	std::string zip_dest = "xl/worksheets/sheet" + strs.str() + ".xml";

	res = myzip(oxml, zip_dest, new_sheet_file);

	if (res){
		return 1;
	}

	remove(new_sheet_file.c_str());

	return 0;
};


int
add_shared_strings_file(std::string & oxml, std::string & temp_folder){

	std::string shared_strings_file;
	join_path(temp_folder, "sharedstrings.xml", shared_strings_file);

	std::ofstream ofs(shared_strings_file.c_str());
	ofs << shared_strings_tplt;
	ofs.close();

	std::string zip_dest = "xl/sharedStrings.xml";

	int res = myzip(oxml, zip_dest, shared_strings_file);

	if (res){
		return 1;
	}

	remove(shared_strings_file.c_str());

	return 0;
};



int
get_last_sheet_number(std::string & oxml, std::string & temp_folder){

	int res = 0;

	std::string file_in_zip = "[Content_Types].xml";
	res = myunzip(oxml.c_str(), file_in_zip.c_str(), temp_folder.c_str());

	if (res){
		return -1;
	}

	std::string file_in_temp;
	join_path(temp_folder, file_in_zip, file_in_temp);

	pugi::xml_document doc;
	pugi::xml_node node;
	pugi::xml_parse_result result;
	pugi::xml_node_iterator it;

	result = doc.load_file(file_in_temp.c_str());

	if (!result){
		return -1;
	}

	node = doc.child("Types");

	int max_sheet_val = 0;

	for (it = node.begin(); it != node.end(); ++it){

		std::string part_name_attr = it->attribute("PartName").value();
		std::string target_substr = "/xl/worksheets/sheet";

		if (part_name_attr.size() > target_substr.size()){

			if (part_name_attr.substr(0, target_substr.size()) == target_substr){

				int pos = part_name_attr.find_last_of(".");
				int sheet_number = std::atoi(part_name_attr.substr(target_substr.size(), pos).c_str());

				if (sheet_number > max_sheet_val){
					max_sheet_val = sheet_number;
				}

			}
		}
	}
	remove(file_in_temp.c_str());

	return max_sheet_val;
};


int
get_last_relation_number(std::string & oxml, std::string & temp_folder){

	std::string file_in_zip = "xl/_rels/workbook.xml.rels";
	int res = myunzip(oxml.c_str(), file_in_zip.c_str(), temp_folder.c_str());

	if (res){
		return -1;
	}

	std::string file_name = "workbook.xml.rels";
	std::string file_in_temp;
	join_path(temp_folder, file_name, file_in_temp);

	pugi::xml_document doc;
	pugi::xml_node node;
	pugi::xml_parse_result result;
	pugi::xml_node_iterator it;

	result = doc.load_file(file_in_temp.c_str());

	if (!result){
		return -1;
	}

	node = doc.child("Relationships");

	// number of chars till the number of id
	int nc = 3;

	int max_rel_val = 0;

	for (it = node.begin(); it != node.end(); ++it){

		std::string part_name_attr = it->attribute("Id").value();

		if (part_name_attr.size() > nc){

			int rel_number = std::atoi(part_name_attr.substr(nc).c_str());

			if (rel_number > max_rel_val){
				max_rel_val = rel_number;
			}
		}
	}
	remove(file_in_temp.c_str());

	return max_rel_val;
};


int reuse_ofstream(std::ofstream & ofs, std::string & orig, std::string & dest){
	ofs.clear();
	ofs.open(dest.c_str());
	ofs << orig;
	ofs.close();
	return 0;
};


int
zip_xml_files(
	std::string & zip_file,
	std::string & temp_path,
	std::vector<std::string> & zip_orig,
	std::vector<std::string> & zip_dest
){

	const char* tmp_name = zip_file.c_str();
	int res = 0;

# ifdef USEWIN32IOAPI
	zlib_filefunc64_def ffunc;
	fill_win32_filefunc64A(&ffunc);
	zipFile dzip = zipOpen2_64(tmp_name, APPEND_STATUS_CREATE, NULL, &ffunc);
# else
	zipFile dzip = zipOpen64(tmp_name, APPEND_STATUS_CREATE);
# endif

	if(dzip == NULL){
		// could not create zip file
		return 1;
	}

	for (int i = 0; i < zip_orig.size(); i++){
		std::string full_orig;
		join_path(temp_path, zip_orig[i], full_orig);
		int res = myzip(tmp_name, zip_dest[i], full_orig);

		if (res != 0){
			return 1;
		}
	}
	return 0;
};


int
has_shared_strings(std::string & oxml_file, std::string & temp_folder){

	int res = 0;

	std::string file_in_zip = "[Content_Types].xml";
	res = myunzip(oxml_file.c_str(), file_in_zip.c_str(), temp_folder.c_str());

	if (res){
		return -1;
	}

	std::string file_in_temp;
	join_path(temp_folder, file_in_zip, file_in_temp);

	pugi::xml_document doc;
	pugi::xml_node node;
	pugi::xml_parse_result result;
	pugi::xml_node_iterator it;

	result = doc.load_file(file_in_temp.c_str());

	if (!result){
		return -1;
	}

	int has_shared_strings = 0;

	node = doc.child("Types");
	node = node.first_child();

	while (node){

		if (std::string(node.attribute("PartName").value()) == "/xl/sharedStrings.xml"){

			has_shared_strings = 1;
			break;
		}
		node = node.next_sibling();
	};

	std::string fpath;
	join_path(temp_folder, file_in_zip, fpath);
	remove(fpath.c_str());

	return has_shared_strings;
};

bool
check_file_exists(const std::string & filename){
  std::ifstream ifile(filename.c_str());
  return static_cast<bool>(ifile);
};


void
my_copy_file(const std::string & source_path, const std::string & dest_path){
	std::ifstream source(source_path.c_str(), std::ios::binary);
	std::ofstream dest(dest_path.c_str(), std::ios::binary);
	dest << source.rdbuf();
};




