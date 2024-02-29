#ifndef AMPL_TABLE_CONNECTOR
#define AMPL_TABLE_CONNECTOR

#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>

/**
 * A single header library developed to simplify the implementation of table handlers for the AMPL 
 * modeling language
 *   https://ampl.com/
 * The framework is based on the code available at
 *   http://www.netlib.org/ampl/tables/
 * General principles of table handlers are available at
 *   https://ampl.com/BOOK/CHAPTERS/13-tables.pdf
 * In particular, a table handler should be able to:
 * - read data from an external table;
 * - write data to an external table;
 * - update an external table.
 * This file is structured in the following way:
 * - definitions from funcadd.h
 * - miscelaneous utility functions
 * - a Logger class
 * - a TableConnector class
 * A template is provided in
 *   /examples/template/src/
 * With the files handler.hpp and handler.cpp provided in the template we can be summarized the 
 * implementation of a Table Handler in the following steps:
 * - update the name and version variables to tell AMPL how to identify the handler;
 * - update the doc variables with a description of the options of the handler. This will be 
 *   available to the user with the command "print _handler_desc["handler_name"];";
 * - implement the register_handler_extensions function to check if a file was provided by the user;
 * - implement the register_handler_args function to to tell the parser what keywords to search for;
 * - implement the register_handler_kargs function to to tell the parser what keywords of the form
 *   key=val to search for;
 * - implement the generate_table function to create a table from scratch if none was provided by 
 *   the user (or throw an error if it's not possible);
 * - implement the validate_arguments function to validate the user arguments provided by the 
 *   parser;
 * - implement the read_in, write_out and write_inout to comunicate with the database. In the
 *   considered functions the read/write/update operations are performed row by row.
 * A simple read/write example is provided in 
 *   /examples/basic/src/
 * A more detailed example is provided in amplcsv.
 */


namespace amplp {

std::string amplp_version = "0.0.0";

// Definitions from funcadd.h
// These definitions provide the pointers to interact with AMPL and should not be changed


extern "C" {

typedef struct cryptblock cryptblock;
typedef struct arglist arglist;
typedef struct function function;
typedef struct TVA TVA;
typedef struct AmplExports AmplExports;
typedef struct AuxInfo AuxInfo;
typedef struct TableInfo TableInfo;
typedef struct TMInfo TMInfo;

struct arglist {     /* Information sent to user-defined functions */
	int n;           /* number of args */
	int nr;          /* number of double input args */
	int *at;         /* argument types -- see DISCUSSION below */
	double *ra;      /* pure double args (IN, OUT, and INOUT) */
	const char **sa; /* symbolic IN args */
	double *derivs;  /* for partial derivatives (if nonzero) */
	double *hes;     /* for second partials (if nonzero) */
	char *dig;       /* if (dig && dig[i]) { partials w.r.t.	*/
					 /*	ra[i] will not be used }	*/
	void *funcinfo;  /* for use by the function (if desired) */
	AmplExports *AE; /* functions made visible (via #defines below) */
	function *f;     /* for internal use by AMPL */
	TVA *tva;        /* for internal use by AMPL */
	char *Errmsg;    /* To indicate an error, set this to a */
					 /* description of the error.  When derivs */
					 /* is nonzero and the error is that first */
					 /* derivatives cannot or are not computed, */
					 /* a single quote character (') should be */
					 /* the first character in the text assigned */
					 /* to Errmsg, followed by the actual error */
					 /* message.  Similarly, if hes is nonzero */
					 /* and the error is that second derivatives */
					 /* are not or cannot be computed, a double */
					 /* quote character (") should be the first */
					 /* character in Errmsg, followed by the */
					 /* actual error message text. */
	TMInfo *TMI;     /* used in Tempmem calls */
	void *Private;
	/* The following fields are relevant */
	/* only when imported functions are called */
	/* by AMPL commands (not declarations). */

	int nin;   /* number of input (IN and INOUT) args */
	int nout;  /* number of output (OUT and INOUT) args */
	int nsin;  /* number of symbolic input arguments */
	int nsout; /* number of symbolic OUT and INOUT args */
};

typedef double (*rfunc)(arglist *);
typedef double(ufunc)(arglist *);

 struct
cryptblock {
	size_t bin;	/* number of input bytes */
	size_t bout;	/* number of output bytes */
	size_t bscr;	/* bytes in scratch array (below) */
	size_t state;	/* decoder manipulates as it sees fit */
	void *scratch;	/* decoder uses this as it sees fit */
	void *misc;	/* decoder uses this as it sees fit; crypto */
			/* initializes misc = scratch.  During reset, */
			/* scratch is relocated, but not misc. */
	int (*decoder)(cryptblock*, char *inbuf, char *outbuf);
			/* decoder must be supplied by the crypto caller */
	};

enum AMPLFUNC_AT_BITS { /* Intrepretation of at[i] when the type */
						/* arg to addfunc has the */
						/* FUNCADD_OUTPUT_ARGS bit on.*/
						AMPLFUNC_INARG = 1,  /* IN or INOUT */
						AMPLFUNC_OUTARG = 2, /* OUT or INOUT */
						AMPLFUNC_STRING =
							4, /* Input value is a string (sa[i]) */
						AMPLFUNC_STROUT = 8 /* String output value allowed */
};

enum FUNCADD_TYPE { /* bits in "type" arg to addfunc */

					/* The type arg to addfunc should consist of one of the */
					/* following values ... */

					FUNCADD_double_VALUED =
						0, /* double (double) valued function */
					FUNCADD_STRING_VALUED =
						2, /* char* valued function (AMPL only) */
					FUNCADD_RANDOM_VALUED = 4, /* double random valued */
					FUNCADD_012ARGS =
						6, /* Special case: double random valued */
					/* with 0 <= nargs <= 2 arguments */
					/* passed directly, rather than in */
					/* an arglist structure (AMPL only). */

					/* possibly or-ed with the following... */

					FUNCADD_STRING_ARGS = 1, /* allow string args */
					FUNCADD_OUTPUT_ARGS =
						16, /* allow output args (AMPL only) */
					FUNCADD_TUPLE_VALUED = 32, /* not yet allowed */

					/* internal use */
					FUNCADD_NO_ARGLIST = 8,
					FUNCADD_NO_DUPWARN =
						64, /* no complaint if already defined */
					FUNCADD_NONRAND_BUILTIN =
						128 /* mean, variance, moment, etc. */
};

/* If a constraint involves an imported function and presolve fixes all
 * the arguments of the function, AMPL may later need to ask the
 * function for its partial derivatives -- even though the solver had
 * no reason to call the function.  If so, it will pass an arglist *al
 * with al->derivs nonzero, and it will expect the function to set
 * al->derivs[i] to the partial derivative of the function with respect
 * to al->ra[i].  Solvers that need to evaluate an imported function
 * work the same way -- they set al->derivs to a nonzero value if they
 * require both the function value and its first derivatives.  Solvers
 * that expect Hessians to be supplied to them also set al->hes to a
 * nonzero value if they require second derivatives at the current
 * argument.  In this case, the function should set
 * al->hes[i + j*(j+1)/2] to the partial derivative of the function with
 * respect to al->ra[i] and al->ra[j] for all 0 <= i <= j < al->nr.
 */

typedef void AddFunc(const char *name,
					 rfunc f,        /* cast f to (rfunc) if it returns char* */
					 int type,       /* see FUNCADD_TYPE above */
					 int nargs,      /* >=  0 ==> exactly that many args
									  * <= -1 ==> at least -(nargs+1) args
									  */
					 void *funcinfo, /* for use by the function (if desired) */
					 AmplExports *ae);

typedef void AddRand(const char *name,
					 rfunc f,        /* assumed to be a random function */
					 rfunc icdf,     /* inverse CDF */
					 int type,       /* FUNCADD_STRING_ARGS or 0 */
					 int nargs,      /* >=  0 ==> exactly that many args
									  * <= -1 ==> at least -(nargs+1) args
									  */
					 void *funcinfo, /* for use by the function (if desired) */
					 AmplExports *ae);

typedef void (*RandSeedSetter)(void *, unsigned long);
typedef void AddRandInit(AmplExports *ae, RandSeedSetter, void *);
typedef void Exitfunc(void *);

struct AuxInfo {
	AuxInfo *next;
	char *auxname;
	void *v;
	void (*f)(AmplExports *, void *, ...);
};

struct AmplExports {
	FILE *StdErr;
	AddFunc *Addfunc;
	long ASLdate;
	int (*FprintF)(FILE *, const char *, ...);
	int (*PrintF)(const char *, ...);
	int (*SprintF)(char *, const char *, ...);
	int (*VfprintF)(FILE *, const char *, va_list);
	int (*VsprintF)(char *, const char *, va_list);
	double (*Strtod)(const char *, char **);
	cryptblock *(*Crypto)(char *key, size_t scrbytes);
	void *asl;
	void (*AtExit)(AmplExports *ae, Exitfunc *, void *);
	void (*AtReset)(AmplExports *ae, Exitfunc *, void *);
	void *(*Tempmem)(TMInfo *, size_t);
	void (*Add_table_handler)(int (*DbRead)(AmplExports *ae, TableInfo *TI),
							  int (*DbWrite)(AmplExports *ae, TableInfo *TI),
							  char *handler_info, int flags, void *Vinfo);
	void *Private;
	void (*Qsortv)(void *, size_t, size_t,
				   int (*)(const void *, const void *, void *), void *);

	/* More stuff for stdio in DLLs... */

	FILE *StdIn;
	FILE *StdOut;
	void (*Clearerr)(FILE *);
	int (*Fclose)(FILE *);
	FILE *(*Fdopen)(int, const char *);
	int (*Feof)(FILE *);
	int (*Ferror)(FILE *);
	int (*Fflush)(FILE *);
	int (*Fgetc)(FILE *);
	char *(*Fgets)(char *, int, FILE *);
	int (*Fileno)(FILE *);
	FILE *(*Fopen)(const char *, const char *);
	int (*Fputc)(int, FILE *);
	int (*Fputs)(const char *, FILE *);
	size_t (*Fread)(void *, size_t, size_t, FILE *);
	FILE *(*Freopen)(const char *, const char *, FILE *);
	int (*Fscanf)(FILE *, const char *, ...);
	int (*Fseek)(FILE *, long, int);
	long (*Ftell)(FILE *);
	size_t (*Fwrite)(const void *, size_t, size_t, FILE *);
	int (*Pclose)(FILE *);
	void (*Perror)(const char *);
	FILE *(*Popen)(const char *, const char *);
	int (*Puts)(const char *);
	void (*Rewind)(FILE *);
	int (*Scanf)(const char *, ...);
	void (*Setbuf)(FILE *, char *);
	int (*Setvbuf)(FILE *, char *, int, size_t);
	int (*Sscanf)(const char *, const char *, ...);
	char *(*Tempnam)(const char *, const char *);
	FILE *(*Tmpfile)(void);
	char *(*Tmpnam)(char *);
	int (*Ungetc)(int, FILE *);
	AuxInfo *AI;
	char *(*Getenv)(const char *);
	void (*Breakfunc)(int, void *);
	void *Breakarg;
	/* Items available with ASLdate >= 20020501 start here. */
	int (*SnprintF)(char *, size_t, const char *, ...);
	int (*VsnprintF)(char *, size_t, const char *, va_list);

	AddRand *Addrand; /* for random function/inverse CDF pairs */
	AddRandInit
		*Addrandinit; /* for adding a function to receive a new random seed */
};

extern const char *i_option_ASL, *ix_details_ASL[];

#if defined(_WIN32) && !defined(__MINGW32__)
__declspec(dllexport)
#endif

extern void funcadd_ASL(AmplExports *); /* dynamically linked */
extern void af_libnamesave_ASL(AmplExports *, const char *fullname,
							   const char *name, int nlen);
extern void
note_libuse_ASL(void); /* If funcadd() does not provide any imported */
					   /* functions, it can call note_libuse_ASL() to */
					   /* keep the library loaded; note_libuse_ASL() is */
					   /* called, e.g., by the tableproxy table handler. */
}

typedef struct DbCol {
	double *dval;
	char **sval;
} DbCol;

struct TableInfo {
	int (*AddRows)(TableInfo *TI, DbCol *cols, long nrows);
	char *tname; /* name of this table */
	char **strings;
	char **colnames;
	DbCol *cols;
	char *Missing;
	char *Errmsg;
	void *Vinfo;
	TMInfo *TMI;
	int nstrings;
	int arity;
	int ncols;
	int flags;
	long nrows;
	void *Private;
	int (*Lookup)(double *, char **, TableInfo *);
	long (*AdjustMaxrows)(TableInfo *, long new_maxrows);
	void *(*ColAlloc)(TableInfo *, int ncol, int sval);
	long maxrows;
};

enum {              /* return values from (*DbRead)(...) and (*DbWrite)(...) */
	   DB_Done = 0, /* Table read or written. */
	   DB_Refuse = 1, /* Refuse to handle this table. */
	   DB_Error = 2   /* Error reading or writing table. */
};

enum {                      /* bits in flags field of TableInfo */
	   DBTI_flags_IN = 1,   /* table has IN  or INOUT entities */
	   DBTI_flags_OUT = 2,  /* table has OUT or INOUT entities */
	   DBTI_flags_INSET = 4 /* table has "in set" phrase: */
	   /* DbRead could omit rows for */
	   /* which Lookup(...) == -1; AMPL */
	   /* will ignore such rows if DbRead */
	   /* offers them. */
};

// End of definitions from funcadd.h

/** Function to allocate memory in AMPLs internal structures (if needed).
 * Replaces the previous macro:
 * define TM(len) (*ae->Tempmem)(TI->TMI, len)
 */
inline void* temp_mem(AmplExports *ae, TableInfo *TI, size_t len);

/** Simple replacement of the to_string function if you are building for older versions of the std.
 */
template <class T> std::string numeric_to_string(T num) {
	std::stringstream strs;
	strs << num;
	return strs.str();
};

/** Convert numeric to string with ndecdig decimal digits.
 * Will be used mostly to print cpu times and such.
 */
template <class T> std::string numeric_to_fixed(T num, int ndecdig) {
	std::stringstream strs;
	strs << std::fixed << std::setprecision(ndecdig) << num;
	return strs.str();
};

/** Gets the substring after the last dot in the given string.
 * Used to determine the string represents a file extension.
 */
inline std::string get_file_extension(const std::string &filepath);

/**Check if a file with a given name already exists.
 */
inline bool check_file_exists(const std::string &filename);

/** Verifies if the lower case representation of 2 strings is identical.
 */
inline bool compare_strings_lower(const std::string &str1, const std::string &str2);

/** Vector printing, might be helpfull for development/debug.
 */
template <class T> void print_vector(const std::vector<T> v) {

	std::cout << "[";
	for (int i = 0; i < v.size(); i++) {
		std::cout << v[i];
		if (i < v.size() - 1) {
			std::cout << ", ";
		}
	}
	std::cout << "]";
	std::cout << std::endl;
};

/** Copies the file in source_path to dest_path.
 */
inline void copy_file(const std::string &source_path, const std::string &dest_path);


inline bool case_insensitive_find(const std::string & val, const std::unordered_map<std::string, std::string> & umap);

inline bool case_insensitive_find(const std::string & val, const std::unordered_set<std::string> & uset);


/** Named enum with DB return values to use in try/catch exceptions.
 */
enum DBE { /* return values from (*DbRead)(...) and (*DbWrite)(...) */
		   DBE_Done = 0,   /* Table read or written. */
		   DBE_Refuse = 1, /* Refuse to handle this table. */
		   DBE_Error = 2   /* Error reading or writing table. */
};

/**
 * Class to encapsulate AMPL Exports functions
 */
class FunctionConnector {
public:

	AmplExports *ae;
	FILE* ampl_stderr;
	FILE* ampl_stdin;
	FILE* ampl_stdout;

	FunctionConnector(AmplExports *ae){
		this->ae = ae;
		ampl_stderr = ae->StdErr;
		ampl_stdin = ae->StdIn;
		ampl_stdout = ae->StdOut;
	};

	FILE* ampl_fopen(const char * filename, const char * mode){
		return ae->Fopen(filename, mode);
	};

	int ampl_fclose(FILE * stream){
		int res = ae->Fclose(stream);
		return res;
	};

	int ampl_fprintf(FILE *stream, const char *format, ...){

		va_list va;
		va_start(va, format);
		int res = ae->VfprintF(stream, format, va);
		va_end(va);
		return res;
	};

	int ampl_printf(const char *format, ...){

		va_list va;
		va_start(va, format);
		int res = ampl_vfprintf(ae->StdOut, format, va);
		va_end(va);
		return res;
	};

	int ampl_sprintf(char *str, const char *format, ...){

		va_list va;
		va_start(va, format);
		int res = ampl_vsprintf(str, format, va);
		va_end(va);
		return res;
	};

	int ampl_vfprintf(FILE *stream, const char *format, va_list arg){
		return ae->VfprintF(stream, format, arg);
	};

	int ampl_vsprintf(char *buffer, const char *format, va_list arg){
		return ae->VsprintF(buffer, format, arg);
	};

	int ampl_vsnprintf(char * s, size_t n, const char * format, va_list arg ){
		return ae->VsnprintF(s, n, format, arg);
	};

	double ampl_strtod(const char *str, char **endptr){
		return ae->Strtod(str, endptr);
	};
};

/** Log levels for the Logger Class.
 */
enum LOG_LEVELS {

	LOG_ERROR = 0,
	LOG_WARNING = 1,
	LOG_INFO = 2,
	LOG_DEBUG = 3,
};

/** A simple logger class. Level specifies the level from wich to print the messages.
 * Successive messages are stored in the messages vector. The corresponding code of the error is
 * stored in the code vector and should follow the LOG_LEVELS enum.
 * Messages with error code LOG_ERROR should be issued before throwing an exception without sending
 * further information to AMPL. Other LOG_LEVELS should be used as apropriate. 
 */
class Logger {
  private:

	AmplExports *ae; // for AMPL printf
	TableInfo *TI;   // for AMPL error message
	int level;       // level to print info
	std::vector<std::string> messages; // error messages
	std::vector<int> codes; // code/level of the corresponding eror message

  public:

	Logger(){
		level = 0; // by default no messages are printed
	};

	void add_ampl_pointers(AmplExports *ae, TableInfo *TI){
		this->ae = ae;
		this->TI = TI;
	};

	int get_level(){ return level; };

	void set_level(int level){ this->level = level; };

	/**
	 * Pass a message to the logger. Error messages are printed immendiatly as they should result
	 * from an exception. Other messages are printed acording to their code and the level requested
	 * by the user.
	 */
	void log(const std::string &msg, int code){

		messages.push_back(msg);
		codes.push_back(code);

		// pass error to AMPL
		if (code == 0) {
			ae->SprintF(TI->Errmsg = (char *)temp_mem(ae, TI, msg.size() + 1), "%s", msg.c_str());
		}
		// print message acording to level
		else if (code <= level) {
			if (code == LOG_WARNING) {
				ae->PrintF("WARNING: ");
			} else if (code == LOG_INFO) {
				ae->PrintF("INFO: ");
			} else {
				ae->PrintF("DEBUG: ");
			}
			ae->PrintF("%s\n", msg.c_str());
		}
	};

	/**
	 * Initially the level of the logger is 0. When we get the requested logger level from the user
	 * we print the already existing messages accordingly.
	 */
	void print_log(){

		for (std::size_t i = 0; i < codes.size(); i++) {
			if (codes[i] <= level) {
				if (codes[i] == LOG_WARNING) {
					ae->PrintF("WARNING: ");
				} else if (codes[i] == LOG_INFO) {
					ae->PrintF("INFO: ");
				} else {
					ae->PrintF("DEBUG: ");
				}
				ae->PrintF("%s\n", messages[i].c_str());
			}
		}
	};
};

/** Auxiliary class to handle text files.
 * The created object will use AMPL's fprintf in order to read/write numbers from/to text files and 
 * close automaticaly if an exception is raised.
 */
class FileHandler:
public FunctionConnector{

private:
	Logger logger;
	FILE* f;
	bool closed;

public:
	FileHandler(
		AmplExports *ae,
		Logger & logger,
		const std::string & filename,
		const std::string & mode
	)
	: FunctionConnector(ae)
	{
		this->logger = logger;
		f = ampl_fopen(filename.c_str(), mode.c_str());

		if (f == NULL){
			std::string log_msg = "FileHandler: could not open " + filename;
			logger.log(log_msg, LOG_ERROR);
			throw DBE_Error;
		}
	};
	~FileHandler(){
		close();
	};
	void fprintf(const char *format, ...){

		va_list va;
		va_start(va, format);
		int res = ae->VfprintF(f, format, va);
		va_end(va);
		if (res < 0){
			std::string log_msg = "FileHandler: fprintf error: " + numeric_to_string(res);
			logger.log(log_msg, LOG_ERROR);
			throw DBE_Error;
		};
	};

	void close(){
		if (f){
			ampl_fclose(f);
			f = NULL;
		}
	};
};


/**
 * Main class used to read, write and update tables.
*/
class TableConnector:
public FunctionConnector{
  private:
	/**
	 * Auxiliary vector to keep string data in memory before passing it to AMPL.
	*/
	std::vector<std::string> tmp_row;

	// pointers to struct with table information
	TableInfo *TI;

	/** Sets the size of tmp_row from the number of columns in AMPL's table.
	 */
	void allocate_row_size(int size){ tmp_row.reserve(size); };

  public:

	/**
	 * Current version of the handler.
	 */
	std::string handler_version;

	/**
	 * Name of the table.
	*/
	std::string table_name;

	/**
	 * Path for the file (or something else) to read/write/update.
	*/
	std::string filepath;

	/**
	 * Inout keyword of the table, can be IN, OUT or INOUT.
	*/
	std::string inout;

	/**
	 * Names that will identify this table handler in the table declaration.
	*/
	std::vector<std::string> handler_names;

	/**
	 * File extensions accepted by the handler.
	*/
	std::vector<std::string> handler_extensions;

	/**
	 * Vector with the args parsed from the table handler declaration.
	*/
	std::unordered_set<std::string> user_args;

	/**
	 * Vector with the kargs parsed from the table handler declaration. For every 
	 * provided string of the type "key=val" we will set user_kargs[key] = val.
	*/
	std::unordered_map<std::string, std::string> user_kargs;

	/**
	 * Set with the args that the user is allowed to provide in the table handler declaration. This 
	 * structure should be filled in the table handler constructor.
	 */
	std::unordered_set<std::string> allowed_args;

	/**
	 * Set with the kargs keys that the user is allowed to provide in the table handler declaration.
	 * This structure should be filled in the table handler constructor.
	 */
	std::unordered_set<std::string> allowed_kargs;

	/**
	 * Vector with all the args provided by the user in the table handler declaration.
	*/
	std::vector<std::string> ampl_args;

	/**
	 * In order to have a single parser for the reader/writer we need to be able to tell one from the other.
	 */
	bool is_writer;

	/** Verbosed level specified by the user.
	 */
	int requested_verbose;

	// for errors and messages to users
	std::string log_msg;
	Logger logger;

	/**
	 * Add pointers to comunicate with AMPL.
	*/
	TableConnector(AmplExports *ae, TableInfo *TI)
	: FunctionConnector(ae)
	{
		this->TI = TI;

		if (ae == NULL || TI == NULL) {
			std::cout << "TableConnector: could not add ampl pointers." << std::endl;
			throw DBE_Error;
		}
		is_writer = false; // by default we assume the table handler is not a writer
	};
	virtual ~TableConnector(){};

	/**
	 * Helper function that encapsulates the necessary steps to read/write/update a table.
	*/
	void run(){

		// pass connections to the logger
		logger.add_ampl_pointers(ae, TI);

		// log the name and version of the handler
		log_msg = handler_version;
		logger.log(log_msg, LOG_INFO);

		register_handler_names();
		register_handler_extensions();
		register_handler_args();
		register_handler_kargs();
		prepare();
		check_table();
		process_table();
	};

	/**
	 * Check if any of the string args provide by the user end with any of the previously defined
	 * file extensions.
	*/
	bool is_handler_extensions(const std::string &arg){

		log_msg = "<is_handler_extensions>";
		logger.log(log_msg, LOG_DEBUG);

		// no extensions to validate
		if (handler_extensions.size() == 0) {
			return false;
		}

		std::string extension = get_file_extension(arg);

		// not a potential extension
		if (extension.empty()) {
			return false;
		}

		for (std::size_t i = 0; i < handler_extensions.size(); i++) {
			//~ if (extension == handler_extensions[i]) {
			if (compare_strings_lower(extension, handler_extensions[i])) {
				return true;
			}
		}
		return false;
	};

	/** Parses all the arguments passed by ampl's table declaration into the structures user_args 
	 * and user_kargs according to the definitions in allowed_args and allowed_kargs.
	 * Filters the verbose flag and updates the level in the logger.
	 * Launches warnings for redundant/unused arguments.
	 * The parser will automatically separate the strings provided by the user by the semicolon ";"
	 * and equal "=" symbols so they shoul not be used in allowed_args and allowed_kargs.
	 */
	void parse_arguments(){

		log_msg = "<parse_arguments>";
		logger.log(log_msg, LOG_DEBUG);

		// at least the table handler must be declared
		if (TI->nstrings == 0) {
			log_msg = "No table handler declared.\n";
			logger.log(log_msg, LOG_ERROR);
			throw DBE_Refuse;
		}

		// first string must be the table handler
		bool found = false;
		std::string tmp_str = TI->strings[0];
		for (std::size_t i = 0; i < handler_names.size(); i++) {
			if (compare_strings_lower(handler_names[i], tmp_str)) {
				found = true;
				break;
			}
		}
		if (!found) {
			log_msg = "No table handler declaration found.\n";
			logger.log(log_msg, LOG_ERROR);
			throw DBE_Refuse;
		}

		// get the name of the table
		table_name = TI->tname;
		if (table_name.size() == 0) {
			log_msg = "Could not get the name of the table.\n";
			logger.log(log_msg, LOG_ERROR);
			throw DBE_Error;
		}

		// log table name
		log_msg = "table: " + table_name;
		logger.log(log_msg, LOG_INFO);

		// check value for inout
		if ((TI->flags & DBTI_flags_IN) && (TI->flags & DBTI_flags_OUT)) {
			inout = "INOUT";
		} else if (TI->flags & DBTI_flags_IN) {
			inout = "IN";
		} else if (TI->flags & DBTI_flags_OUT) {
			inout = "OUT";
		} else {
			log_msg = "unsuported inout flag";
			logger.log(log_msg, LOG_ERROR);
			throw DBE_Error;
		}

		// log inout
		log_msg = "inout: " + inout;
		logger.log(log_msg, LOG_INFO);

		requested_verbose = 0;

		// parse remaining args
		for (int i = 1; i < TI->nstrings; i++) {

			std::string mystr = TI->strings[i];
			ampl_args.push_back(mystr);

			if (!mystr.empty()){
				process_string(mystr);
			}


			//~ size_t first = 0;
			//~ size_t last = mystr.find(";");

			//~ while (last != std::string::npos){

				//~ std::string temp = mystr.substr(first, last - first);
				//~ if (!temp.empty()){
					//~ process_string(temp);
				//~ }

				//~ first = last + 1;
				//~ last = mystr.find(";", first);
			//~ }
			//~ std::string temp = mystr.substr(first);
			//~ if (!temp.empty()){
				//~ process_string(temp);
			//~ }
		}

		// print previous messages in logger (if requested)
		if (requested_verbose != 0){
			logger.set_level(requested_verbose);
			if (logger.get_level() > 0) {
				logger.print_log();
			}
		}
	};

	/** Check if a given string is a potential karg, i.e. has the form "key=val".
	 */
	bool is_karg(std::string & str){
		if (str.find("=") == std::string::npos){
			return false;
		}
		return true;
	};

	/** Check if a given string represents an arg or karg and dispatch it acordingly.
	 */
	void process_string(std::string & str){
		if (is_karg(str)){
			process_karg(str);
		}
		else{
			process_arg(str);
		}
	};

	/**
	 * Given a string that represents an arg checks if the string:
	 *   - is "verbose";
	 *   - has a file extension provided in register_handler_extensions;
	 *   - is in allowed_args;
	 *   - represents an alias.
	 * If the string is in allowed_args it will be included in used_args.
	 * Only the first string with a file extension will be accepted, posterior ones will be 
	 * discarded with a warning.
	 * If the string is not "verbose", is not in allowed_args and does not represent a file 
	 * extension it will be interpreted as an alias. Only the first alias will be accepted, 
	 * posterior ones will be discarded with a warning.
	 */
	void process_arg(std::string & arg_string){

		bool has_alias = false;
		bool has_file = false;

		// check verbose
		if (arg_string == "verbose") {
			// set basic level of verbose (warnings)
			log_msg = "verbose level set to 1";
			logger.log(log_msg, LOG_INFO);
			requested_verbose = 1;
		}
		// check if the string is a potential file of the given extensions
		else if (is_handler_extensions(arg_string)) {
			if (!has_file) {
				filepath = arg_string;
				has_file = true;
				log_msg = "file: " + arg_string;
				logger.log(log_msg, LOG_INFO);
			} else {
				log_msg = "ignoring argument: " + arg_string;
				logger.log(log_msg, LOG_INFO);
			}
		}
		// check if arg_string is in allowed_args
		//~ else if (allowed_args.find(arg_string) != allowed_args.end()) {
		else if (case_insensitive_find(arg_string, allowed_args)) {
			user_args.insert(arg_string);
		}
		// check for an alias
		else {
			if (!has_alias) {
				table_name = arg_string;
				has_alias = true;
				log_msg = "using alias: " + arg_string;
				logger.log(log_msg, LOG_WARNING);
			}
			// already has alias, ignore argument
			else {
				log_msg = "ignoring argument: " + arg_string;
				logger.log(log_msg, LOG_WARNING);
			}
		}
	};

	/** Given a string that represents a karg "key=val":
	 *   - splits the string by the "=" character into a key and a value;
	 *   - checks if the key is verbose or in allowed_kargs. If the key is in allowed_kargs sets
	 *     user_kargs[key] = val.
	 */
	void process_karg(std::string & karg_string){

		size_t eq_pos = karg_string.find("=");
		std::string key = karg_string.substr(0, eq_pos);

		if (eq_pos < karg_string.size() - 1) {

			std::string val = karg_string.substr(eq_pos + 1);

			// check verbose
			if (key == "verbose" || key == "VERBOSE") {
				requested_verbose = std::atoi(val.c_str());
				if (requested_verbose > 0) {
					log_msg = "verbose: " + val;
					logger.log(log_msg, LOG_INFO);
				}
			}
			// add valid keys to user_kargs for posterior validation
			//~ else if (allowed_kargs.find(key) != allowed_kargs.end()) {
			else if (case_insensitive_find(key, allowed_kargs)) {
				user_kargs[key] = val;
				log_msg = "parse_arguments: \'" + key + "\' set to \'" + val + "\'";
				logger.log(log_msg, LOG_INFO);
			}
			// discard keys that were not declared
			else {
				log_msg = "key: " + key + " with value: " + val +
						  " was not declared, ignoring: " + karg_string;
				logger.log(log_msg, LOG_WARNING);
			}
		}
		// discard incomplete statements like "something="
		else {
			log_msg = "Could not parse " + karg_string;
			logger.log(log_msg, LOG_WARNING);
		}
	};

	/**
	 * Check if a path to an external file was defined, otherwise construct a
	 * path based on the table name and the provided extensions.
	*/
	void validate_filepath(){

		log_msg = "<validate_filepath>";
		logger.log(log_msg, LOG_DEBUG);

		if (filepath.empty()) {
			filepath = table_name;

			// see if we can add extension
			if (handler_extensions.size() > 0) {
				filepath += ".";
				filepath += handler_extensions[0];
				log_msg = "No file declared. Using \'" + filepath + "\'";
				logger.log(log_msg, LOG_WARNING);
			} else {
				log_msg = "Could not add extension to file \'" + filepath + "\'";
				logger.log(log_msg, LOG_WARNING);
			}
		}
	};

	// Table info functions

	/** Returns the number of arity (key) columns in AMPL's table.
	 */
	size_t nkeycols() { return TI->arity; };

	/** Returns the number of non arity (data) columns in AMPL's table.
	 */
	size_t ndatacols() { return TI->ncols; };

	/** Returns the total number of columns in AMPL's table.
	 */
	size_t ncols() { return TI->arity + TI->ncols; };

	/** Returns the number of rows in AMPL's table.
	 */
	size_t nrows() { return TI->nrows; };

	/**
	 * Sets the value of column in position "col" in AMPL's table to the double "val". 
	 */
	void set_col_val(double val, int col) {
		TI->cols[col].sval[0] = 0;
		TI->cols[col].dval[0] = val;
	};

	/**
	 * Sets the value of column in position "col" in AMPL's table to the string "val".
	 */
	void set_col_val(const std::string &val, int col) {
		tmp_row.push_back(val);
		TI->cols[col].sval[0] = const_cast<char *>(tmp_row.back().c_str());
	};

	/**
	 * Sets the value of column in position "col" in AMPL's table to missing.
	 */
	void set_col_missing_val(int col) {
		TI->cols[col].sval[0] = TI->Missing;
	};

	/** 
	 * Check if value of a given row at a given column in AMPL's table is numeric.
	 */
	bool is_numeric_val(int row, int col) {
		return !is_char_val(row, col);
	};

	/**
	 * Check if value of a given row at a given column in AMPL's table is missing.
	 */
	bool is_missing(int row, int col) {

		if (TI->cols[col].sval && TI->cols[col].sval[row] &&
			TI->cols[col].sval[row] == TI->Missing) {
			return true;
		}
		return false;
	};

	/**
	 * Get the numerical value of a given row at a given column in AMPL's table.
	 */
	double get_numeric_val(int row, int col){
		return TI->cols[col].dval[row];
	};

	/**
	 * Gets a pointer to the char value of a given row at a given column in AMPL's table.
	 */
	char *get_char_val(int row, int col){
		return TI->cols[col].sval[row];
	};

	/**
	 * Check if value of a given row at a given column in AMPL's table is a char.
	 */
	bool is_char_val(int row, int col){

		if (TI->cols[col].sval && TI->cols[col].sval[row]) {
			return true;
		}
		return false;
	};

	/**
	 * Gets a pointer the the first char of the name of column in position col in AMPL's table. 
	 */
	char *get_col_name(int col){
		return TI->colnames[col];
	};

	/**
	 * Get a pointer to the numerical values of a given column.
	 * The number of elements in the column is nrows()
	 */
	double* get_col_double(int col){
		return TI->cols[col].dval;
	};

	/**
	 * Get a pointer to the char values of a given column.
	 * The number of elements in the column is nrows()
	 */
	char** get_col_char(int col){
		return TI->cols[col].sval;
	};

	/**
	 * After we fill all the values of a row with the set_col_value functions we pass the values to
	 * AMPL with this function.
	 */
	void add_row(){
		// pass data to AMPL
		DbCol *db = TI->cols;
		if ((*TI->AddRows)(TI, db, 1)) {
			log_msg = "Error with AddRows.";
			logger.log(log_msg, LOG_WARNING);
			// we only log a warning to preserve the original error message from AMPL
			throw DBE_Error;
		}
		tmp_row.clear();
	};

	// End of table info functions

	//~ // AMPL Exports functions
	//~ // Replacement functions for fprintf, printf, sprintf, vfprintf, vsprintf and strtod.
	//~ // These functions should be used instead of of standart library ones to prevent errors derived
	//~ // from diferent compiler implementations.

	//~ FILE* ampl_fopen(const char * filename, const char * mode){
		//~ return ae->Fopen(filename, mode);
	//~ };

	//~ int ampl_fclose(FILE * stream){
		//~ int res = ae->Fclose(stream);
		//~ return res;
	//~ };

	//~ int ampl_fprintf(FILE *stream, const char *format, ...){

		//~ va_list va;
		//~ va_start(va, format);
		//~ int res = ae->VfprintF(stream, format, va);
		//~ va_end(va);
		//~ return res;
	//~ };

	//~ int ampl_printf(const char *format, ...){

		//~ va_list va;
		//~ va_start(va, format);
		//~ int res = ampl_vfprintf(ae->StdOut, format, va);
		//~ va_end(va);
		//~ return res;
	//~ };

	//~ int ampl_sprintf(char *str, const char *format, ...){

		//~ va_list va;
		//~ va_start(va, format);
		//~ int res = ampl_vsprintf(str, format, va);
		//~ va_end(va);
		//~ return res;
	//~ };

	//~ int ampl_vfprintf(FILE *stream, const char *format, va_list arg){
		//~ return ae->VfprintF(stream, format, arg);
	//~ };

	//~ int ampl_vsprintf(char *buffer, const char *format, va_list arg){
		//~ return ae->VsprintF(buffer, format, arg);
	//~ };

	//~ double ampl_strtod(const char *str, char **endptr){
		//~ return ae->Strtod(str, endptr);
	//~ };

	//~ // End of AMPL Exports functions

	/** Parse and validate arguments, ensure the external file is found.
	 */
	void prepare(){

		log_msg = "<prepare>";
		logger.log(log_msg, LOG_DEBUG);

		parse_arguments();
		validate_arguments();

		if(handler_extensions.size() > 0){
			validate_filepath();

			// check if filepath exists
			if (!check_file_exists(filepath)) {

				if (!is_writer) {
					log_msg = "Cannot find source to read data.";
					logger.log(log_msg, LOG_ERROR);
					throw DBE_Error;
				} else {
					// write as an OUT table as there is nothing to update
					inout = "OUT";
					generate_table();

					log_msg = "generating file: " + filepath;
					logger.log(log_msg, LOG_WARNING);
				}
			}
		}
	};

	/** Decide if we will read, write or update the data.
	 * We also allocate the size of row in case of a read. 
	 */
	void process_table(){

		log_msg = "<process_table>";
		logger.log(log_msg, LOG_DEBUG);

		if (!is_writer) {
			allocate_row_size(ncols());
			read_in();
		} else {
			if (inout == "OUT") {
				write_out();
			} else if (inout == "INOUT") {
				write_inout();
			}
		}
	};

	/** After we check the file exists we might need to
	 * check if the table exists
	 */
	virtual void check_table(){};

	/** Read data from an external an table.
	 */
	virtual void read_in() = 0;

	/** Write data into an external table.
	 */
	virtual void write_out() = 0;

	/**	Update external table.
	 */
	virtual void write_inout() = 0;

	/** If the external table does not exist we need a function to create one.
	 * If it's not possible to create the table from scratch we should log an appropriate message 
	 * and throw DBE_Error.
	 */
	virtual void generate_table() = 0;

	/** In order to tell AMPL how to invoke this handler fill the vector handler_names with 
	 * something like:
	 * 
	 * handler_names = {"somename"};
	 * 
	 * The parser will automatically detect if the table handler was invoked correctly in a table 
	 * declaration such as:
	 * 
	 * table mytable IN "somename" 
	 * 
	 * Note that the search for the table handler name is not case sensitive.
	 */
	virtual void register_handler_names() = 0;

	/** Define what type of file extensions will be accepted. For example, if you set
	 * 
	 * handler_extensions = {"txt"};
	 * 
	 * the parser will search for strings with that extension and assign it to the variable 
	 * filepath.
	 */
	virtual void register_handler_extensions() = 0;

	/** Tell the parser wich keywords to search for in the table declaration.
	 */
	virtual void register_handler_args() = 0;

	/** Tell the parser which key/value pairs (of the form key=type) in the table declaration.
	 */
	virtual void register_handler_kargs() = 0;

	/** Validates the structures derived from the parsing. You should look at user_args and 
	 * user_kargs to see the input from the user and procedd acordingly.
	 */
	virtual void validate_arguments() = 0;

	// Functions to convert arguments from user_kargs

	/** Given a string key checks if it is a key of user_kargs and returns bolean true or false if 
	 * the value of the map of the key is string "true" or "false".
	 * Throws DBE_Error if the value of the key is not "true" or "false" and sets the appropriate
	 * message in the logger.
	 */
	bool get_bool_karg(const std::string &key){

		if (user_kargs[key] == "true") {
			return true;
		} else if (user_kargs[key] == "false") {
			return false;
		} else {
			log_msg =
				"invalid value for argument: " + key + "=" + user_kargs[key];
			logger.log(log_msg, LOG_ERROR);
			throw DBE_Error;
		}
	};

	/**Given a string key checks if it is a key of user_kargs and returns the convertion of the 
	 * corresponding value to integer.
	 */
	int get_int_karg(const std::string &key){

		std::string val = user_kargs[key];
		return std::atoi(val.c_str());
	};

	/** Given a string key checks if it is a key of user_kargs and returns the corresponding double
	 * value associated.
	 * Throws DBE_Error if it's not possible to convert the value to double and sets the appropriate
	 * message in the logger.
	 */
	double get_double_karg(const std::string &key){

		std::string val = user_kargs[key];
		char *se;
		double t;

		// check if val is a number
		t = ampl_strtod(val.c_str(), &se);
		if (!*se) { /* valid number */
			return t;
		} else {
			log_msg = "Error in parameter " + key + "=" + val +
					  ". Could not convert " + val + " to double";
			logger.log(log_msg, LOG_ERROR);
			throw DBE_Error;
		}
	};

	/** Given a string key, a vector of strings orig and a vector of strings dest gets the value val
	 * of key in user_kargs, searches.for val in the orig vector and if val is found returns the
	 * element in the same position at the dest vector.
	 * Throws DBE_Error if the value of the key is not found in the vector orig and sets the 
	 * appropriate message in the logger.
	 */
	std::string get_map_karg(const std::string &key,
							 std::vector<std::string> &orig,
							 std::vector<std::string> &dest){

		std::string val = user_kargs[key];

		assert(orig.size() == dest.size());

		for (std::size_t i = 0; i < orig.size(); i++) {
			if (orig[i] == val) {
				return dest[i];
			}
		}

		log_msg =
			"Error in parameter " + key + "=" + val + ". Invalid option " + val;
		logger.log(log_msg, LOG_ERROR);
		throw DBE_Error;
	};

	// End of functions to convert arguments from user_kargs

	/** Provide a FileHandler object to be used with the TableConnector class.
	 */
	FileHandler get_file_handler(
		const std::string & filename,
		const std::string & mode
	){
		return FileHandler(ae, logger, filename, mode);
	};
};

/** Pass the implemented read and write functions to AMPL.
 * You will not need to call this function if you use the provided template.
 */
inline void add_table_handler(
	AmplExports *ae,
	int (*DbRead)(AmplExports *ae,TableInfo *TI),
	int (*DbWrite)(AmplExports *ae, TableInfo *TI),
	char *handler_info,
	int flags,
	void *Vinfo
);

// Function implementations

void add_table_handler(
	AmplExports *ae,
	int (*DbRead)(AmplExports *ae,TableInfo *TI),
	int (*DbWrite)(AmplExports *ae, TableInfo *TI),
	char *handler_info,
	int flags,
	void *Vinfo
){
	ae->Add_table_handler(DbRead, DbWrite, handler_info, flags, Vinfo);
};

std::string get_file_extension(const std::string &filepath) {

	if (filepath.find_last_of(".") != std::string::npos) {
		return filepath.substr(filepath.find_last_of(".") + 1);
	}
	return std::string();
};

bool check_file_exists(const std::string &filename) {
	if (FILE *file = fopen(filename.c_str(), "r")) {
		fclose(file);
		return true;
	}
	return false;
};

bool compare_strings_lower(const std::string &str1, const std::string &str2) {

	if (str1.size() != str2.size()) {
		return false;
	}

	for (std::size_t i = 0; i < str1.size(); i++) {
		if (std::tolower(str1[i]) != std::tolower(str2[i])) {
			return false;
		}
	}
	return true;
};

void copy_file(const std::string &source_path, const std::string &dest_path) {
	std::ifstream source(source_path.c_str(), std::ios::binary);
	std::ofstream dest(dest_path.c_str(), std::ios::binary);
	dest << source.rdbuf();
};

void* temp_mem(AmplExports *ae, TableInfo *TI, size_t len){
	return ae->Tempmem(TI->TMI, len);
};

bool case_insensitive_find(const std::string & val, const std::unordered_map<std::string, std::string> & umap){

	for(auto it : umap){
		std::string temp = it.first;
		if(compare_strings_lower(val, temp)){
			return true;
		}
	}
	return false;
};

bool case_insensitive_find(const std::string & val, const std::unordered_set<std::string> & uset){

	for(auto &temp : uset){
		if(compare_strings_lower(val, temp)){
			return true;
		}
	}
	return false;
};




// End of function implementations
} // namespace ampl

/** AMPL expects a funcadd function to bridge AMPL and the external connector.
 */
#define funcadd amplp::funcadd_ASL

#endif
