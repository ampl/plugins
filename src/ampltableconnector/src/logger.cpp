#include "logger.hpp"


Logger::Logger(){
	level = 0;
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
Logger::log(const std::string & msg, int code){

	messages.push_back(msg);
	codes.push_back(code);

	// pass error to AMPL
	if (code == 0){
		sprintf(TI->Errmsg = (char*)TM(msg.size() + 1), "%s", msg.c_str());
	}
	// print message acording to level
	else if (code <= level){
		if (code == LOG_WARNING){
			printf("WARNING: ");
		}
		else if (code == LOG_INFO){
			printf("INFO: ");
		}
		else{
			printf("DEBUG: ");
		}
		printf("%s\n", msg.c_str());
	}
};


void
Logger::print_log(){

	for (int i = 0; i < codes.size(); i++){
		if (codes[i] <= level){
			if (codes[i] == LOG_WARNING){
				printf("WARNING: ");
			}
			else if (codes[i] == LOG_INFO){
				printf("INFO: ");
			}
			else{
				printf("DEBUG: ");
			}
			printf("%s\n", messages[i].c_str());
		} 
	}
};
