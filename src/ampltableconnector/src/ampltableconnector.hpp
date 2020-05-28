#pragma once

#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <cstdlib>


// headers from AMPL
#include "funcadd.h"
#include "arith.h"	/* for Arith_Kind_ASL and Long */


// auxiliary headers
#include "logger.hpp"
#include "utils.hpp"


// auxiliary macro to allocate memory in AMPLs internal structures (if needed)
#define TM(len) (*ae->Tempmem)(TI->TMI,len)

// current version of the handler
const std::string version = "examplehandler - 0.0.0";


// We need a named enum with DB return values to use in try/catch
enum DBE{	/* return values from (*DbRead)(...) and (*DbWrite)(...) */
	DBE_Done = 0,	/* Table read or written. */
	DBE_Refuse = 1,	/* Refuse to handle this table. */
	DBE_Error = 2	/* Error reading or writing table. */
};


// base class with attributes and methods usefull both to the reader and writer
class Connector{

	public:

	// connections to AMPL
	TableInfo *TI;
	AmplExports *ae;

	// name of the table
	std::string table_name;

	// path for file (or something else) to read/write
	std::string filepath;

	// inout keyword of the table, can be IN, OUT or INOUT
	std::string inout;

	// names that will identify this table handler in the table declaration
	std::vector<std::string> handler_names;

	// file extensions accepted by the handler
	std::vector<std::string> handler_extensions;

	// structures to parse the provided arguments:
	// args - vector of strings of individual arguments passed by AMPL
	// kargs_map - map of the arguments separated by an "=". An argument of the form "key=val" will
	// be automatically processed and accessed with kargs_map[key] = val.
	// kargs - vector of keys in kargs_map, used to preserve the order in which the keys were read
	std::vector<std::string> args;
	std::map<std::string, std::string> kargs_map;
	std::vector<std::string> kargs;

	// in order to have a single parser for the reader/writer we need to be able to identify both
	bool is_writer;

	// for errors and messages to users 
	std::string log_msg;
	Logger logger;

	Connector();

	// add pointers to comunicate with AMPL 
	void add_ampl_connections(AmplExports *ae, TableInfo *TI);

	// let AMPL know how to invoke this handler
	void register_handler_names();

	// define what type of file extensions will be accepted
	void register_handler_extensions();

	// check if the string arg ends with any of the previously defined extensions
	bool is_handler_extensions(const std::string & arg);

	// parses all the arguments passed by ampl to the structures args, kargs_map and kargs. Filters
	// the verbose flag and updates the level in the logger. Launches warnings for redundant/unused
	// arguments.
	void parse_arguments();

	// Validates the structures derived from the parsing.
	void validate_arguments();

	// Check if a path to an external file was defined, otherwise constructs a path based on the
	// table name and the provided extensions.
	void validate_filepath();
};


// class to read data from an external table into AMPL
class ReadConnector:
public Connector{
	public:

	// parse and validate arguments, ensure the external table is found
	void prepare();

	// read data from external table
	// additional methods could be added here, if needed
	void run();

	// Read data from external table
	void read_in();
};


// Class to write data from an AMPL to an external table. Note that to update an external table we
// might need to read it first, update the necessary values and write the information, so this
//  might be simultaneously a reader and a writer.
class WriteConnector:
public Connector{
	public:

	WriteConnector();

	// parse and validate arguments, ensure the external table is found otherwise create it
	void prepare();

	// check if we want to drop or update the external representation of the table
	// additional methods could be added here, if needed
	void run();

	// overwrite external table
	void write_out();

	// update external table
	void write_inout();

	// if the external table does not exist we need a function to create it
	void generate_file();
};
