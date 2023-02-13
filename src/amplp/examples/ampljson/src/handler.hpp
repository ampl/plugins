#pragma once

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/ostreamwrapper.h>

#include "amplp.hpp"

using namespace amplp;

static std::string name = "ampljson";
static std::string version = "alpha - 0.0.0";

static std::string doc = name + "\n" + name + "-" + version + "\n" +
"Don't change the above line.\n"
"Add a description of the table handler and its parameters.\n"
"This documentation will be accessed in AMPL with the command.\n"
"    print _handler_desc[\"name\"];\n"
;

class Handler:
public TableConnector{

	// add aditional attributes and methods as needed

	private:

	bool raw = false; // write minimal information
	bool append = false; // append table to existing json (find tables keyword)

	// keys for the json we are going to generate
	std::string ktables = "tables"; // optional
	std::string kname = "name";
	std::string kcolnames = "colnames";
	std::string layout = "rows";
	std::string krow = "row"; //optional

	// override functions
	void register_handler_names(){
		log_msg = "<register_handler_names>";
		logger.log(log_msg, LOG_DEBUG);
		handler_names ={name, name + ".dll"};
	};
	void register_handler_extensions();
	void register_handler_args();
	void register_handler_kargs();
	void generate_table();
	void validate_arguments();
	void read_in();
	void write_out();
	void write_inout();

	void read_in_bones(rapidjson::Document & doc);
	void read_in_meat(rapidjson::Document & doc);

	void write_out_append();
	void write_out_bones();
	void write_out_meat();

	void data_get_rows(rapidjson::Value& data, rapidjson::Document::AllocatorType& allocator);
	void data_get_columns(rapidjson::Value& data, rapidjson::Document::AllocatorType& allocator);

	void push_table(rapidjson::Document& d);
	void add_table_info(rapidjson::Value& table, rapidjson::Document::AllocatorType& allocator);

	void read_table_info(rapidjson::Value& jobj);

	rapidjson::Document read_json_doc();
	void write_json_doc(rapidjson::Document& d);


	void test_merge();

	public:

	Handler(AmplExports *ae, TableInfo *TI) : TableConnector(ae, TI){
		handler_version = name + " - " + version;
	};
};

// add aditional functions as needed
void wrap_string(rapidjson::Value& v, std::string& s, rapidjson::Document::AllocatorType& a);

