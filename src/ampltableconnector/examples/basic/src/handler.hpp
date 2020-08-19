#pragma once

#include "ampltableconnector.hpp"

class BasicHandler:
public Connector{

	public:

	// add attributes as needed

	// override functions
	void read_in();
	void write_out();
	void write_inout();
	void register_handler_names();
	void register_handler_extensions();
	void generate_table();

	// add functions as needed
};
