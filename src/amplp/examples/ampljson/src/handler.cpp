#include "handler.hpp"


static int
Read_ampljson(AmplExports *ae, TableInfo *TI){

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
Write_ampljson(AmplExports *ae, TableInfo *TI){

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
	add_table_handler(ae, Read_ampljson, Write_ampljson, const_cast<char *>(doc.c_str()), 0, 0);
};

// Adapt the functions bellow to meet your requirements

void
Handler::read_in(){

	log_msg = "<read_in>";
	logger.log(log_msg, LOG_DEBUG);

	rapidjson::Document doc = read_json_doc();

	if (raw){
		read_in_bones(doc);
	}
	else{
		read_in_meat(doc);
	}
};

rapidjson::Document
Handler::read_json_doc(){
	std::ifstream ifs(filepath.c_str()); // stream to read the data from the json file

	if (!ifs){
		log_msg = "Could not open " + filepath + " to read data.";
		logger.log(log_msg, LOG_ERROR);
		throw DBE_Error;
	}

	rapidjson::IStreamWrapper isw { ifs };

	rapidjson::Document doc {};
	doc.ParseStream( isw );

	rapidjson::StringBuffer buffer {};
	rapidjson::Writer<rapidjson::StringBuffer> writer { buffer };
	doc.Accept( writer );

	if ( doc.HasParseError() )
	{
		std::cout << "Error  : " << doc.GetParseError()  << '\n'
				<< "Offset : " << doc.GetErrorOffset() << '\n';
		throw DBE_Error;;
	}
	const std::string jsonStr { buffer.GetString() };
	//std::cout << jsonStr << std::endl;

	return doc;
};

void
Handler::write_json_doc(rapidjson::Document& doc){

	// 3. Stringify the DOM
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	doc.Accept(writer);

	// Output {"project":"rapidjson","stars":11}
	//~ std::cout << buffer.GetString() << std::endl;
	std::string json (buffer.GetString(), buffer.GetSize());


	std::ofstream ofs (filepath);
	ofs << json;

	if (!ofs.good()){
		log_msg = "Could not write json to " + filepath;
		logger.log(log_msg, LOG_ERROR);
		throw DBE_Error;
	}

};





void
Handler::read_in_bones(rapidjson::Document & doc){

	log_msg = "<read_in_bones>";
	logger.log(log_msg, LOG_DEBUG);

};


void
Handler::read_in_meat(rapidjson::Document & doc){

	log_msg = "<read_in_meat>";
	logger.log(log_msg, LOG_DEBUG);

	//bool is_array = false;
	//rapidjson::Value table(rapidjson::kObjectType);

	// the json might be an array of entities(objects)
	if (doc.IsArray()){

		// there's probably a better way to get the table object
		bool found = false;

		for (rapidjson::SizeType i = 0; i < doc.Size(); i++){

			if(doc[i].IsObject()){
				
				if (doc[i].HasMember("id") && doc[i]["id"].GetString() == table_name){

					read_table_info(doc[i]);
					found = true;
					break;
				}
			}
		}
		if (!found){
			log_msg = "Could not find table " + table_name + " in file " + filepath;
			logger.log(log_msg, LOG_ERROR);
			throw DBE_Error;
		}
	}
	// or a single entity(object)
	else if (doc.IsObject()){
		read_table_info(doc);
	}
	else{
		log_msg = "Unsuported json root object " + doc.GetType() ;
		logger.log(log_msg, LOG_ERROR);
		throw DBE_Error;
	}
};


void
Handler::write_out(){

	log_msg = "<write_out>";
	logger.log(log_msg, LOG_DEBUG);

	if (append){
		write_out_append();
		return;
	}

	if (raw){
		write_out_bones();
	}
	else{
		write_out_meat();
	}
};

void
Handler::test_merge(){

	rapidjson::Document olddoc = read_json_doc();

	rapidjson::Document newdoc;
    newdoc.SetArray();

	rapidjson::Value clone(olddoc, newdoc.GetAllocator());
	newdoc.PushBack(clone, newdoc.GetAllocator());

	// 3. Stringify the DOM
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	newdoc.Accept(writer);

	// Output {"project":"rapidjson","stars":11}
	//~ std::cout << buffer.GetString() << std::endl;
	std::string json (buffer.GetString(), buffer.GetSize());


	std::ofstream ofs (filepath);
	ofs << json;

	if (!ofs.good()){
		log_msg = "Could not write json to " + filepath;
		logger.log(log_msg, LOG_ERROR);
		throw DBE_Error;
	}




}

void
Handler::write_out_append(){

	log_msg = "<write_out_append>";
	logger.log(log_msg, LOG_DEBUG);

	rapidjson::Document olddoc = read_json_doc();
	if (olddoc.IsArray()){
		push_table(olddoc);
		write_json_doc(olddoc);
	}
	else if (olddoc.IsObject()){
		rapidjson::Document d;
		d.SetArray();
		rapidjson::Value clone(olddoc, d.GetAllocator());
		d.PushBack(clone, d.GetAllocator());
		push_table(d);
		write_json_doc(d);
	}
	else{
		rapidjson::Document d;
		d.SetArray();
		push_table(d);
		write_json_doc(d);
	}
};

void
Handler::write_out_meat(){

	log_msg = "<write_out_meat>";
	logger.log(log_msg, LOG_DEBUG);

	std::clock_t c_start = std::clock();

	rapidjson::Document d;
	d.SetObject();
	rapidjson::Document::AllocatorType& allocator = d.GetAllocator();
	add_table_info(d, allocator);
	write_json_doc(d);

	std::clock_t c_end = std::clock();
	double time_elapsed = 1000.0 * (c_end-c_start) / CLOCKS_PER_SEC;
	log_msg = "write_out_meat done in: " + numeric_to_fixed(time_elapsed / 1000, 3);
	logger.log(log_msg, LOG_DEBUG);

};


void
Handler::write_out_bones(){

	std::clock_t c_start = std::clock();

	log_msg = "<write_out_bones>";
	logger.log(log_msg, LOG_DEBUG);

	rapidjson::Document d;
	d.SetArray();

	rapidjson::Document::AllocatorType& allocator = d.GetAllocator();

	rapidjson::Value val(rapidjson::kObjectType);

	rapidjson::Value columns(rapidjson::kArrayType);

	for (size_t j = 0; j < ncols(); j++){

		std::string tmp_str = get_col_name(j);
		val.SetString(tmp_str.c_str(), static_cast<rapidjson::SizeType>(tmp_str.length()), allocator);
		columns.PushBack(val, allocator);
	}

	d.PushBack(columns, allocator);

	// write data iterating by rows and columns
	for (size_t i = 0; i < nrows(); i++){

		rapidjson::Value row(rapidjson::kArrayType);

		for (size_t j = 0; j < ncols(); j++){

			// check if element is a string
			if (is_char_val(i, j)){
				// if value is missing don't write anything
				if (is_missing(i, j)){}
				else{
					std::string tmp_str = get_char_val(i, j);
					val.SetString(tmp_str.c_str(), static_cast<rapidjson::SizeType>(tmp_str.length()), allocator);
					row.PushBack(val, allocator);
				}
			}
			// otherwise element is numeric
			else{
				row.PushBack(get_numeric_val(i, j), allocator);
			}
		}
		d.PushBack(row, allocator);
	}

	// 3. Stringify the DOM
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	d.Accept(writer);

	// Output {"project":"rapidjson","stars":11}
	//~ std::cout << buffer.GetString() << std::endl;
	std::string json (buffer.GetString(), buffer.GetSize());


	std::ofstream ofs (filepath);
	ofs << json;
	if (!ofs.good()){
		log_msg = "Could not write json to " + filepath;
		logger.log(log_msg, LOG_ERROR);
		throw DBE_Error;
	}


	std::clock_t c_end = std::clock();
	double time_elapsed = 1000.0 * (c_end-c_start) / CLOCKS_PER_SEC;
	log_msg = "write_out_bones done in: " + numeric_to_fixed(time_elapsed / 1000, 3);
	logger.log(log_msg, LOG_DEBUG);
};


void
Handler::write_inout(){

	log_msg = "<write_inout>";
	logger.log(log_msg, LOG_DEBUG);

	log_msg = "ampljson: INOUT not supported.";
	logger.log(log_msg, LOG_ERROR);
	throw DBE_Error;
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

	handler_extensions = {"json"};
};


void
Handler::register_handler_args(){

	log_msg = "<register_handler_args>";
	logger.log(log_msg, LOG_DEBUG);

	allowed_args = {"raw", "append"};
};


void
Handler::register_handler_kargs(){

	log_msg = "<register_handler_kargs>";
	logger.log(log_msg, LOG_DEBUG);

	allowed_kargs ={"layout"};
};


void
Handler::validate_arguments(){

	log_msg = "<validate_arguments>";
	logger.log(log_msg, LOG_DEBUG);

	for (const auto& elem: user_args){
		if (elem == "raw"){
			raw = true;
		}
		else if (elem == "append"){
			append = true;
		}
	}


	for(const auto it: user_kargs){

		std::string key = it.first;
		if (key == "layout"){
			if (it.second == "rows"){layout = it.second;}
			else if (it.second == "columns"){layout = it.second;}
			else{
				log_msg = "Unsuported layout option " + it.second;
				logger.log(log_msg, LOG_ERROR);
				throw DBE_Error;
			}
		}
	}
};

void
Handler::data_get_rows(rapidjson::Value& data, rapidjson::Document::AllocatorType& allocator){
	
	rapidjson::Value val(rapidjson::kObjectType);

	// write data iterating by rows and columns
	for (size_t i = 0; i < nrows(); i++){

		rapidjson::Value row(rapidjson::kArrayType);

		for (size_t j = 0; j < ncols(); j++){

			// check if element is a string
			if (is_char_val(i, j)){
				// if value is missing don't write anything
				if (is_missing(i, j)){}
				else{
					std::string tmp_str = get_char_val(i, j);
					val.SetString(tmp_str.c_str(), static_cast<rapidjson::SizeType>(tmp_str.length()), allocator);
					row.PushBack(val, allocator);
				}
			}
			// otherwise element is numeric
			else{
				row.PushBack(get_numeric_val(i, j), allocator);
			}
		}
		data.PushBack(row, allocator);
	}
};

void
Handler::data_get_columns(rapidjson::Value& data, rapidjson::Document::AllocatorType& allocator){
	
	rapidjson::Value val(rapidjson::kObjectType);

	// write data iterating columns and rows
	for (size_t j = 0; j < ncols(); j++){

		rapidjson::Value row(rapidjson::kArrayType);

		for (size_t i = 0; i < nrows(); i++){

			// check if element is a string
			if (is_char_val(i, j)){
				// if value is missing don't write anything
				if (is_missing(i, j)){}
				else{
					std::string tmp_str = get_char_val(i, j);
					val.SetString(tmp_str.c_str(), static_cast<rapidjson::SizeType>(tmp_str.length()), allocator);
					row.PushBack(val, allocator);
				}
			}
			// otherwise element is numeric
			else{
				row.PushBack(get_numeric_val(i, j), allocator);
			}
		}
		data.PushBack(row, allocator);
	}
};

void
Handler::add_table_info(rapidjson::Value& table, rapidjson::Document::AllocatorType& allocator){

	rapidjson::Value jkey(rapidjson::kObjectType);
	rapidjson::Value jval(rapidjson::kObjectType);

	// table type
	std::string skey = "type";
	std::string sval = "table";

	wrap_string(jkey, skey, allocator);
	wrap_string(jval, sval, allocator);

	table.AddMember(jkey, jval, allocator);

	// name of the table
	skey = "id";

	wrap_string(jkey, skey, allocator);
	wrap_string(jval, table_name, allocator);

	table.AddMember(jkey, jval, allocator);

	// column names
	skey = "colnames";
	wrap_string(jkey, skey, allocator);

	rapidjson::Value columns(rapidjson::kArrayType);

	for (size_t j = 0; j < ncols(); j++){
		std::string tmp_str = get_col_name(j);
		wrap_string(jval, tmp_str, allocator);
		columns.PushBack(jval, allocator);
	}
	table.AddMember(jkey, columns, allocator);

	// layout
	skey = "layout";

	wrap_string(jkey, skey, allocator);
	wrap_string(jval, layout, allocator);

	table.AddMember(jkey, jval, allocator);

	// data
	skey = "data";
	wrap_string(jkey, skey, allocator);

	rapidjson::Value data(rapidjson::kArrayType);
	if (layout == "rows"){
		data_get_rows(data, allocator);
	}
	else if (layout == "columns"){
		data_get_columns(data, allocator);
	}
	else{
		//should never get here
	}
	table.AddMember(jkey, data, allocator);
};

void
Handler::push_table(rapidjson::Document& d){

	rapidjson::Document::AllocatorType& allocator = d.GetAllocator();

	rapidjson::Value table(rapidjson::kObjectType);
	add_table_info(table, allocator);
	d.PushBack(table, allocator);
};


void
//Handler::read_table_info(const rapidjson::GenericObject& jobj){
Handler::read_table_info(rapidjson::Value& jobj){
	//std::cout << doc.HasMember(kcolnames.c_str()) << std::endl;
	//std::cout << doc[kcolnames.c_str()].GetString() << std::endl;

	/*
	if (jobj.HasMember("type")){
		//int aux = doc[kcolnames.c_str()].GetType();
		std::cout << "type : " << jobj["type"].GetString() << std::endl;
	}

	if (jobj.HasMember("id")){
		//int aux = doc[kcolnames.c_str()].GetType();
		std::cout << "id : " << jobj["id"].GetString() << std::endl;
	}
	*/

	std::vector<std::string> json_header;

	if (jobj.HasMember("colnames")){
		//std::cout << "colnames : " << jobj["colnames"].GetType() << std::endl;

		rapidjson::Value info(rapidjson::kArrayType);

		info = jobj["colnames"].GetArray();
		const rapidjson::Value& a = jobj["colnames"];

		for (rapidjson::SizeType i = 0; i < info.Size(); i++){
			if(info[i].IsString()){
				//std::cout << info[i].GetString() << std::endl;
				json_header.push_back(info[i].GetString());
			}
			else if(info[i].IsNumber()){
				//std::cout << info[i].GetDouble() << std::endl;
			}
		}
	}

	std::vector<int> perm;
	for(int i=0; i<json_header.size(); i++){
		perm.push_back(i);
	}

	if (jobj.HasMember("layout")){
		layout = jobj["layout"].GetString();
	}

	if (layout == "rows"){
		const rapidjson::Value& allrows = jobj["data"];
		for (rapidjson::SizeType i = 0; i < allrows.Size(); i++){
			const rapidjson::Value& row = allrows[i];
			for (rapidjson::SizeType j = 0; j < row.Size(); j++){
				if(row[j].IsNull()){
					set_col_missing_val(perm[j]);
				}
				else if(row[j].IsString()){
					std::cout << row[j].GetString() << std::endl;
					set_col_val(row[j].GetString(), perm[j]);
				}
				else if(row[j].IsNumber()){
					std::cout << row[j].GetDouble() << std::endl;
					set_col_val(row[j].GetDouble(), perm[j]);
				}	
			}
			add_row();
		}
	}
	else if (layout == "columns"){
		const rapidjson::Value& allcolumns = jobj["data"];

		int json_nrows = allcolumns[0].Size();

		for(int i = 0; i < json_nrows; i++){
			for (int j = 0; j < allcolumns.Size(); j++){

				if(allcolumns[j][i].IsNull()){
					set_col_missing_val(perm[j]);
				}
				else if(allcolumns[j][i].IsString()){
					//std::cout << row[j].GetString() << std::endl;
					set_col_val(allcolumns[j][i].GetString(), perm[j]);
				}
				else if(allcolumns[j][i].IsNumber()){
					//std::cout << row[j].GetDouble() << std::endl;
					set_col_val(allcolumns[j][i].GetDouble(), perm[j]);
				}	
			}
			add_row();
		}
	}
};

void wrap_string(rapidjson::Value& v, std::string& s, rapidjson::Document::AllocatorType& a){
	v.SetString(s.c_str(), static_cast<rapidjson::SizeType>(s.length()), a);
};




