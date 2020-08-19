#include "handler.hpp"


static int
Read_Basic(AmplExports *ae, TableInfo *TI){

	int res = DBE_Done;
	BasicHandler cn;

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
Write_Basic(AmplExports *ae, TableInfo *TI){

	int res = DBE_Done;
	BasicHandler cn;
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
	static char info[] = "handler name\n"
		"Write table handler description and help\n";

	// Inform AMPL about the handlers
	ae->Add_table_handler(Read_Basic, Write_Basic, info, 0, 0);
};


void
BasicHandler::read_in(){

	log_msg = "<read_in>";
	logger.log(log_msg, LOG_DEBUG);

	// In this method you will get the data from your external representation of the table and pass
	// it to AMPL.

	// A small example follows. Consider a table with 10 rows and 3 columns (keys, string_values and
	// numeric_values) with data contained in vectors. The table is iterated row by row and each row
	// is individually passed to AMPL. Use this code with the file ../tests/test_in.run 

	int n_data_rows = 10;

	std::vector<std::string> keys;
	std::vector<std::string> string_values;
	std::vector<double> numeric_values;

	keys.reserve(n_data_rows);
	string_values.reserve(n_data_rows);
	numeric_values.reserve(n_data_rows);

	// generate some data
	for (int i = 0; i < n_data_rows; i++){
		keys.push_back("k" + numeric_to_string(i + 1));
		string_values.push_back("val" + numeric_to_string(i + 1));
		numeric_values.push_back(2 * (i + 1));
	}

	// iterate rows of data
	for (int i = 0; i < n_data_rows; i++){

		// set value for the appropriate column number
		set_col_val(keys[i], 0);
		set_col_val(string_values[i], 1);
		set_col_val(numeric_values[i], 2);

		// pass row to AMPL
		add_row();
	}
};


void
BasicHandler::write_out(){

	log_msg = "<write_out>";
	logger.log(log_msg, LOG_DEBUG);

	// This method should overwrite the external representation of the table with the data in AMPL's
	// table.
	// The following code is an example on how to get the data from AMPL, printing the column names
	// and the data in AMPL's representation of the table.

	// iterate the column names
	for (int j = 0; j < ncols(); j++){
		ampl_printf("%s", get_col_name(j));
		if (j < ncols() - 1){
			ampl_printf("\t");
		}
	}
	ampl_printf("\n");

	// iterate rows and columns printing data
	for (int i = 0; i < nrows(); i++){
		for (int j = 0; j < ncols(); j++){

			if (is_char_val(i, j)){
				// string value
				ampl_printf("%s", get_char_val(i, j));
			}
			else{
				// numeric value
				ampl_printf("%g", get_numeric_val(i, j));
			}

			if (j < ncols() - 1){
				ampl_printf("\t");
			}
		}
		ampl_printf("\n");
	}
};


void
BasicHandler::write_inout(){

	log_msg = "<write_inout>";
	logger.log(log_msg, LOG_DEBUG);

	// Unlike write_out() this method should update the external representation of the table with
	// the data in AMPL's table.
	// For an example on how to get the data from AMPL see write_out() .

	// A detailed description of table handlers management is available at
	// https://ampl.com/netlib/ampl/tables/index.html

	// implement the method as needed and remove the following error
	log_msg = "write_inout() not implemented";
	logger.log(log_msg, LOG_ERROR);
	throw DBE_Error;
};


void
BasicHandler::generate_table(){

	log_msg = "<generate_table>";
	logger.log(log_msg, LOG_DEBUG);

	// Implement a method to generate a file (or something else) for the developed handler with
	// name filepath.
};


void
BasicHandler::register_handler_names(){

	log_msg = "<register_handler_names>";
	logger.log(log_msg, LOG_DEBUG);

	handler_names.push_back("basichandler");
	handler_names.push_back("basichandler.dll");
};


void
BasicHandler::register_handler_extensions(){

	log_msg = "<register_handler_extensions>";
	logger.log(log_msg, LOG_DEBUG);

	handler_extensions.push_back("bas");
};




