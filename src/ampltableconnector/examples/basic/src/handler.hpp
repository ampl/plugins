#pragma once

#include "ampltableconnector.hpp"

using namespace amplt;

static std::string name = "basichandler";
static std::string version = "version";

static std::string doc = name + "\n" + name + "-" + version + "\n" +
"Don't change the above line.\n"
"Add a description of the table handler and its parameters.\n"
"This documentation will be accessed in AMPL with the command.\n"
"    print _handler_desc[\"name\"];\n"
;

class Handler:
public Connector{

	// add aditional attributes and methods as needed

	private:

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

	public:

	Handler(AmplExports *ae, TableInfo *TI) : Connector(ae, TI){
		handler_version = name + " - " + version;
	};
};

// add aditional functions as needed
