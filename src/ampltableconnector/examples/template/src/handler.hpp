#pragma once

#include "ampltableconnector.hpp"

using namespace ampl;

class Handler:
public Connector{

	public:

	Handler();

	// add aditional attributes as needed

	// override functions
	void read_in();
	void write_out();
	void write_inout();
	void register_handler_names();
	void register_handler_extensions();
	void generate_table();

	// add aditional methods as needed
};

// add aditional functions as needed
