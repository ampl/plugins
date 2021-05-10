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

namespace amplt {

typedef struct cryptblock cryptblock;

extern "C" {

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

// macro to allocate memory in AMPLs internal structures (if needed)
//~ #define TM(len) (*ae->Tempmem)(TI->TMI, len)
void* temp_mem(AmplExports *ae, TableInfo *TI, size_t len);


void add_table_handler(
	AmplExports *ae,
	int (*DbRead)(AmplExports *ae,TableInfo *TI),
	int (*DbWrite)(AmplExports *ae, TableInfo *TI),
	char *handler_info,
	int flags,
	void *Vinfo
);


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


// Simple replacement for the to_string function since we are building for
// -std=c++03
template <class T> std::string numeric_to_string(T num) {
    std::stringstream strs;
    strs << num;
    return strs.str();
};

// Convert numeric to string with ndecdig decimal digits
// will be used mostly to print cpu times and such
template <class T> std::string numeric_to_fixed(T num, int ndecdig) {
    std::stringstream strs;
    strs << std::fixed << std::setprecision(ndecdig) << num;
    return strs.str();
};

// Gets the substring after the last dot in the given string
std::string get_file_extension(const std::string &filepath);

// Check if a file with a given name already exists.
bool check_file_exists(const std::string &filename);

// Verifies if the lower case representation of 2 strings is identical
bool compare_strings_lower(const std::string &str1, const std::string &str2);

// vector printing, might be helpfull
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

// copies the file in source_path to dest_path
void copy_file(const std::string &source_path, const std::string &dest_path);

enum LOG_LEVELS {

    LOG_ERROR = 0,
    LOG_WARNING = 1,
    LOG_INFO = 2,
    LOG_DEBUG = 3,
};

class Logger {
  private:
    std::vector<std::string> messages;
    std::vector<int> codes;

  public:
    AmplExports *ae; // for AMPL printf
    TableInfo *TI;   // for AMPL error message
    int level;       // level to print info

    Logger();
    void add_info(AmplExports *ae, TableInfo *TI);
    void set_level(int level);
    void log(const std::string &msg, int code);
    void print_log();
};

// We need a named enum with DB return values to use in try/catch
enum DBE { /* return values from (*DbRead)(...) and (*DbWrite)(...) */
           DBE_Done = 0,   /* Table read or written. */
           DBE_Refuse = 1, /* Refuse to handle this table. */
           DBE_Error = 2   /* Error reading or writing table. */
};


class FileHandler {

private:
	AmplExports *ae;
	Logger logger;
	FILE* f;
	bool closed;

public:
	FileHandler(
		AmplExports *ae,
		Logger & logger,
		const std::string & filename,
		const std::string & mode
	);
	~FileHandler();
	void ampl_fprintf(const char *format, ...);
	void close();
};


// base class with attributes and methods usefull both to the reader and writer
class Connector {

  private:
    // auxiliary vector to keep data in memory before passing it to AMPL
    std::vector<std::string> tmp_row;

  public:
    // current version of the handler
    std::string version;

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

    //~ // attributes that may be modified by the user in AMPL's table handler
    //declaration ~ std::map<std::string, std::string> user_args;

    // structures to parse the arguments provided by AMPL:
    // args - vector of strings of individual arguments passed by AMPL, if an
    // argument "arg" is found in the table declaration then ampl_args_map[arg]
    // is set to true
    std::map<std::string, bool> ampl_args_map;
    // kargs_map - map of the arguments separated by an "=". An argument of the
    // form "key=val" will be automatically processed and accessed with
    // kargs_map[key] = val.
    std::map<std::string, std::string> ampl_kargs_map;

    std::vector<std::string> used_args;
    std::vector<std::string> used_kargs;

    // in order to have a single parser for the reader/writer we need to be able
    // to identify both
    bool is_writer;

    // for errors and messages to users
    std::string log_msg;
    Logger logger;

    Connector();
    virtual ~Connector();

    // add pointers to comunicate with AMPL
    void add_ampl_connections(AmplExports *ae, TableInfo *TI);

    // check if the string arg ends with any of the previously defined
    // extensions
    bool is_handler_extensions(const std::string &arg);

    // parses all the arguments passed by ampl to the structures args, kargs_map
    // and kargs. Filters the verbose flag and updates the level in the logger.
    // Launches warnings for redundant/unused arguments.
    void parse_arguments();

    // Check if a path to an external file was defined, otherwise constructs a
    // path based on the table name and the provided extensions.
    void validate_filepath();

    // Table info functions

    // returns the numer of arity columns in AMPL's table
    int nkeycols();

    // returns the number of non arity (data) columns in AMPL's table
    int ndatacols();

    // returns the total number of columns in AMPL's table
    int ncols();

    // returns the number of rows in AMPL's table
    int nrows();

    void set_col_val(double val, int col);
    void set_col_val(const std::string &val, int col);
    void set_col_val(char *val, int col);
    void set_col_missing_val(int col);

    double get_numeric_val(int row, int col);
    char *get_char_val(int row, int col);

    bool is_numeric_val(int row, int col);
    bool is_char_val(int row, int col);
    bool is_missing(int row, int col);

    char *get_col_name(int col);

    void add_row();

    // AMPL Export functions

    int ampl_fprintf(FILE *stream, const char *format, ...);

    int ampl_printf(const char *format, ...);

    int ampl_sprintf(char *str, const char *format, ...);

    int ampl_vfprintf(FILE *stream, const char *format, va_list arg);

    int ampl_vsprintf(char *buffer, const char *format, va_list arg);

    double ampl_strtod(const char *str, char **endptr);

    // parse and validate arguments, ensure the external table is found
    void prepare();

    // read data from external table
    // additional methods could be added here, if needed
    void run();

    // read data from external table
    virtual void read_in() = 0;

    // overwrite external table
    virtual void write_out() = 0;

    // update external table
    virtual void write_inout() = 0;

    // if the external table does not exist we need a function to create it
    virtual void generate_table() = 0;

    // let AMPL know how to invoke this handler
    virtual void register_handler_names() = 0;

    // define what type of file extensions will be accepted
    virtual void register_handler_extensions() = 0;

    // Validates the structures derived from the parsing.
    virtual void validate_arguments();

    // functions to convert types from ampl_kargs_map
    bool get_bool_karg(const std::string &key);
    int get_int_karg(const std::string &key);
    double get_double_karg(const std::string &key);

    std::string get_map_karg(const std::string &key,
                             std::vector<std::string> &orig,
                             std::vector<std::string> &dest);

    //
    void allocate_row_size(int size);

	FileHandler get_file_handler(
		const std::string & filename,
		const std::string & mode
	);
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

Logger::Logger() { level = 0; };

void Logger::add_info(AmplExports *ae, TableInfo *TI) {

    this->ae = ae;
    this->TI = TI;
};

void Logger::set_level(int level) { this->level = level; };

void Logger::log(const std::string &msg, int code) {

    messages.push_back(msg);
    codes.push_back(code);

    // pass error to AMPL
    if (code == 0) {
        //~ ae->SprintF(TI->Errmsg = (char *)TM(msg.size() + 1), "%s", msg.c_str());
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

void Logger::print_log() {

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

Connector::Connector() {
    version = "examplehandler - 0.0.0";
    is_writer = false;
};

Connector::~Connector(){};

// Table info functions

int Connector::nkeycols() { return TI->arity; };

int Connector::ndatacols() { return TI->ncols; };

int Connector::ncols() { return TI->arity + TI->ncols; };

int Connector::nrows() { return TI->nrows; };

void Connector::set_col_val(double val, int col) {
    TI->cols[col].sval[0] = 0;
    TI->cols[col].dval[0] = val;
};

void Connector::set_col_val(const std::string &val, int col) {
    tmp_row.push_back(val);
    TI->cols[col].sval[0] = const_cast<char *>(tmp_row.back().c_str());
};

void Connector::set_col_missing_val(int col) {
    TI->cols[col].sval[0] = TI->Missing;
};

void Connector::add_row() {
    // pass data to AMPL
    DbCol *db = TI->cols;
    if ((*TI->AddRows)(TI, db, 1)) {
        log_msg = "Error with AddRows";
        logger.log(log_msg, LOG_WARNING);
        // we only log a warning to preserve the original error message from AMPL
        throw DBE_Error;
    }
    tmp_row.clear();
};

double Connector::get_numeric_val(int row, int col) {
    return TI->cols[col].dval[row];
};

char *Connector::get_char_val(int row, int col) {
    return TI->cols[col].sval[row];
};

bool Connector::is_char_val(int row, int col) {

    if (TI->cols[col].sval && TI->cols[col].sval[row]) {
        return true;
    }
    return false;
};

bool Connector::is_numeric_val(int row, int col) {
    return !is_char_val(row, col);
};

bool Connector::is_missing(int row, int col) {

    if (TI->cols[col].sval && TI->cols[col].sval[row] &&
        TI->cols[col].sval[row] == TI->Missing) {
        return true;
    }
    return false;
};

char *Connector::get_col_name(int col) { return TI->colnames[col]; };

// AMPL Export functions

int Connector::ampl_fprintf(FILE *stream, const char *format, ...) {

    va_list va;
    va_start(va, format);
    int res = ae->VfprintF(stream, format, va);
    va_end(va);
    return res;
};

int Connector::ampl_printf(const char *format, ...) {

    va_list va;
    va_start(va, format);
    int res = ampl_vfprintf(ae->StdOut, format, va);
    va_end(va);
    return res;
};

int Connector::ampl_sprintf(char *str, const char *format, ...) {

    va_list va;
    va_start(va, format);
    int res = ampl_vsprintf(str, format, va);
    va_end(va);
    return res;
};

int Connector::ampl_vfprintf(FILE *stream, const char *format, va_list arg) {
    return ae->VfprintF(stream, format, arg);
};

int Connector::ampl_vsprintf(char *buffer, const char *format, va_list arg) {
    return ae->VsprintF(buffer, format, arg);
};

double Connector::ampl_strtod(const char *str, char **endptr) {
    return ae->Strtod(str, endptr);
};

void Connector::add_ampl_connections(AmplExports *ae, TableInfo *TI) {

    this->ae = ae;
    this->TI = TI;

    if (ae == NULL) {
        std::cout << "Connector: could not add ampl exports." << std::endl;
        throw DBE_Error;
    }

    if (TI == NULL) {
        std::cout << "Connector: could not add table info." << std::endl;
        throw DBE_Error;
    }
    // pass connections to the logger
    logger.add_info(ae, TI);

    // log the name and version of the handler
    log_msg = version;
    logger.log(log_msg, LOG_INFO);

    register_handler_names();
    register_handler_extensions();
};

void Connector::parse_arguments() {

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

    bool has_alias = false;
    bool has_file = false;

	int verbose_level = 0;

    // parse remaining args and check for verbose
    for (int i = 1; i < TI->nstrings; i++) {

        std::string arg_string = TI->strings[i];
        size_t eq_pos = std::string(TI->strings[i]).find("=");

        // single argument
        if (eq_pos == std::string::npos) {

            // check verbose
            if (arg_string == "verbose") {
                // set basic level of verbose (warnings)
                log_msg = "verbose level set to 1";
                logger.log(log_msg, LOG_INFO);
                verbose_level = 1;
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
            // check if arg_string is in the keys of ampl_args_map
            // if so set its value to true
            else if (ampl_args_map.find(arg_string) != ampl_args_map.end()) {
                ampl_args_map[arg_string] = true;
                used_args.push_back(arg_string);
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
        }
        // key, value arguments with "=" separator
        else {
            std::string key = arg_string.substr(0, eq_pos);

            if (eq_pos < arg_string.size() - 1) {

                std::string val = arg_string.substr(eq_pos + 1);

                // check verbose
                if (key == "verbose") {
                    verbose_level = std::atoi(val.c_str());
                    if (verbose_level > 0) {
                        log_msg = "verbose: " + val;
                        logger.log(log_msg, LOG_INFO);
                    }
                }
                // add valid keys to ampl_kargs_map for posterior validation
                else if (ampl_kargs_map.find(key) != ampl_kargs_map.end()) {
                    ampl_kargs_map[key] = val;
                    used_kargs.push_back(key);
                    log_msg = "parse_arguments: \'" + key + "\' set to \'" + val + "\'";
                    logger.log(log_msg, LOG_DEBUG);
                }
                // discard keys that were not declared
                else {
                    log_msg = "key: " + key + " with value: " + val +
                              " was not declared, ignoring: " + arg_string;
                    logger.log(log_msg, LOG_WARNING);
                }
            }
            // discard incomplete statements like "something="
            else {
                log_msg = "Could not parse " + arg_string;
                logger.log(log_msg, LOG_WARNING);
            }
        }
    }

    // print previous messages in logger (if requested)
	if (verbose_level != 0){
		logger.set_level(verbose_level);
		if (logger.level > 0) {
			logger.print_log();
		}
	}
};

void Connector::validate_arguments() {

    log_msg = "<validate_arguments>";
    logger.log(log_msg, LOG_DEBUG);
};

bool Connector::is_handler_extensions(const std::string &arg) {

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
        if (extension == handler_extensions[i]) {
            return true;
        }
    }
    return false;
};

void Connector::validate_filepath() {

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

void Connector::prepare() {

    log_msg = "<prepare>";
    logger.log(log_msg, LOG_DEBUG);

    parse_arguments();
    validate_arguments();
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
};

void Connector::run() {

    log_msg = "<run>";
    logger.log(log_msg, LOG_DEBUG);

    if (!is_writer) {
        read_in();
    } else {
        if (inout == "OUT") {
            write_out();
        } else if (inout == "INOUT") {
            write_inout();
        }
    }
};

bool Connector::get_bool_karg(const std::string &key) {

    if (ampl_kargs_map[key] == "true") {
        return true;
    } else if (ampl_kargs_map[key] == "false") {
        return false;
    } else {
        log_msg =
            "invalid value for argument: " + key + "=" + ampl_kargs_map[key];
        logger.log(log_msg, LOG_ERROR);
        throw DBE_Error;
    }
};

int Connector::get_int_karg(const std::string &key) {

    std::string val = ampl_kargs_map[key];
    return std::atoi(val.c_str());
};

double Connector::get_double_karg(const std::string &key) {

    std::string val = ampl_kargs_map[key];
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

std::string Connector::get_map_karg(const std::string &key,
                                    std::vector<std::string> &orig,
                                    std::vector<std::string> &dest) {

    std::string val = ampl_kargs_map[key];

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

void Connector::allocate_row_size(int size) { tmp_row.reserve(size); };


FileHandler Connector::get_file_handler(
	const std::string & filename,
	const std::string & mode
){
	return FileHandler(ae, logger, filename, mode);
};


FileHandler::FileHandler(
	AmplExports *ae,
	Logger & logger,
	const std::string & filename,
	const std::string & mode
){
	this->ae = ae;
	this->logger = logger;
	f = fopen(filename.c_str(), mode.c_str());
	closed = false;

	if (f == NULL){
		std::string log_msg = "FileHandler: could not open " + filename;
		logger.log(log_msg, LOG_ERROR);
		throw DBE_Error;
	}
};

void
FileHandler::close(){
	fclose(f);
	closed = true;
};


FileHandler::~FileHandler(){
	if (!closed){
		fclose(f);
	}
};


void
FileHandler::ampl_fprintf(const char *format, ...) {

	va_list va;
	va_start(va, format);
	int res = ae->VfprintF(f, format, va);
	va_end(va);
	if (res <= 0){
		std::string log_msg = "FileHandler: ampl_fprintf error: " + numeric_to_string(res);
		logger.log(log_msg, LOG_ERROR);
		throw DBE_Error;
	};
};


} // namespace ampl

#define funcadd amplt::funcadd_ASL

#endif
