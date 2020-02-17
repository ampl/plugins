#include "logger.hpp"

Logger::Logger(){
	level = 0;
	path = "";
};

void
Logger::add_info(AmplExports *ae, TableInfo *TI){

	this->ae = ae;
	this->TI = TI;
};

void
Logger::set_level(int level){

	this->level = level;
};


void
Logger::log(std::string & msg, int code){

	messages.push_back(msg);
	codes.push_back(code);

	// pass error to AMPL
	if (code == 0){
		sprintf(TI->Errmsg = (char*)TM(strlen(msg.c_str())), "%s", msg.c_str());
	}
	// print message acording to level
	else if (code <= level){
		printf("%s\n", msg.c_str());
	}
};
