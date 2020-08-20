#include "handler.hpp"


static int
Read_Basic(AmplExports *ae, TableInfo *TI){

	int res = DBE_Done;
	Handler cn;

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
	Handler cn;
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


Handler::Handler(){
	// provide default values for atributes
};


void
Handler::read_in(){

	log_msg = "<read_in>";
	logger.log(log_msg, LOG_DEBUG);
};


void
Handler::write_out(){

	log_msg = "<write_out>";
	logger.log(log_msg, LOG_DEBUG);
};


void
Handler::write_inout(){

	log_msg = "<write_inout>";
	logger.log(log_msg, LOG_DEBUG);
};


void
Handler::generate_table(){

	log_msg = "<generate_table>";
	logger.log(log_msg, LOG_DEBUG);
};


void
Handler::register_handler_names(){

	log_msg = "<register_handler_names>";
	logger.log(log_msg, LOG_DEBUG);
};


void
Handler::register_handler_extensions(){

	log_msg = "<register_handler_extensions>";
	logger.log(log_msg, LOG_DEBUG);
};




