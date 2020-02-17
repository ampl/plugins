#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <cstring>

// headers from AMPL
#include "funcadd.h"
#include "arith.h"	/* for Arith_Kind_ASL and Long */

// auxiliary macro to allocate memory in AMPLs internal structures
#define TM(len) (*ae->Tempmem)(TI->TMI,len)

enum LOG_LEVELS{

	LOG_ERROR = 0,
	LOG_WARNING = 1,
	LOG_INFO = 2,
	LOG_DEBUG = 3,
};

class Logger
{
	public:

	AmplExports *ae; //for AMPL printf
	TableInfo *TI; // for AMPL error message
	int level; // level to print info
	std::vector<std::string> messages; 
	std::vector<int> codes;
	std::string path; // to write log?


	Logger();
	void add_info(AmplExports *ae, TableInfo *TI);
	void set_level(int level);
	void log(std::string & msg, int code);
};
