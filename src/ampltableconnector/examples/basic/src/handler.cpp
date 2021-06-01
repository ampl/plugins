#include "handler.hpp"


static int
Read_Basic(AmplExports *ae, TableInfo *TI){

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
Write_Basic(AmplExports *ae, TableInfo *TI){

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
	add_table_handler(ae, Read_Basic, Write_Basic, const_cast<char *>(doc.c_str()), 0, 0);
};

// Adapt the functions bellow to meet your requirements

void
Handler::read_in(){

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

	// Before passing the data to AMPL you should always confirm the names and positions of the 
	// columns in AMPL. See write_out for an example on how to iterate the column names.

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
Handler::write_out(){

	log_msg = "<write_out>";
	logger.log(log_msg, LOG_DEBUG);

	// This method should replace the external representation of the table with the data in AMPL's
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
Handler::write_inout(){

	log_msg = "<write_inout>";
	logger.log(log_msg, LOG_DEBUG);

	// This method should update the external representation of the table with the data in AMPL's 
	// table. For an example on how to get the data from AMPL see write_out() .
};


void
Handler::generate_table(){

	log_msg = "<generate_table>";
	logger.log(log_msg, LOG_DEBUG);

	// Implement a method to generate a file (or something else) for the developed handler with
	// name filepath.
};


void
Handler::register_handler_extensions(){

	log_msg = "<register_handler_extensions>";
	logger.log(log_msg, LOG_DEBUG);

	// We assume that content for this handler has the extension ".bas". amplt will automatically
	// search for files in the table handler declaration and put it in the variable filepath. 
	handler_extensions = {"bas"};
};


void
Handler::register_handler_args(){

	log_msg = "<register_handler_args>";
	logger.log(log_msg, LOG_DEBUG);
};


void
Handler::register_handler_kargs(){

	log_msg = "<register_handler_kargs>";
	logger.log(log_msg, LOG_DEBUG);
};


void
Handler::validate_arguments(){

	log_msg = "<validate_arguments>";
	logger.log(log_msg, LOG_DEBUG);
};

