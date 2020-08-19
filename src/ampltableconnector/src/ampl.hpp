#pragma once


#ifndef Char
#define Char void
#endif

typedef struct cryptblock cryptblock;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct arglist arglist;
typedef struct function function;
typedef struct TVA TVA;
typedef struct AmplExports AmplExports;
typedef struct AuxInfo AuxInfo;
typedef struct TableInfo TableInfo;
typedef struct TMInfo TMInfo;

//~ #ifndef No_arglist_def

#undef Const
#define Const const

struct
arglist {			/* Information sent to user-defined functions */
	int n;			/* number of args */
	int nr;			/* number of double input args */
	int *at;		/* argument types -- see DISCUSSION below */
	double *ra;		/* pure double args (IN, OUT, and INOUT) */
	Const char **sa;	/* symbolic IN args */
	double *derivs;		/* for partial derivatives (if nonzero) */
	double *hes;		/* for second partials (if nonzero) */
	char *dig;		/* if (dig && dig[i]) { partials w.r.t.	*/
				/*	ra[i] will not be used }	*/
	Char *funcinfo;		/* for use by the function (if desired) */
	AmplExports *AE;	/* functions made visible (via #defines below) */
	function *f;		/* for internal use by AMPL */
	TVA *tva;		/* for internal use by AMPL */
	char *Errmsg;		/* To indicate an error, set this to a */
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
	TMInfo *TMI;		/* used in Tempmem calls */
	Char *Private;
				/* The following fields are relevant */
				/* only when imported functions are called */
				/* by AMPL commands (not declarations). */

	int nin;		/* number of input (IN and INOUT) args */
	int nout;		/* number of output (OUT and INOUT) args */
	int nsin;		/* number of symbolic input arguments */
	int nsout;		/* number of symbolic OUT and INOUT args */
};

typedef double (*rfunc) (arglist *);
typedef double (ufunc) (arglist *);

//~ #endif /* No_arglist_def */

enum AMPLFUNC_AT_BITS {	/* Intrepretation of at[i] when the type */
				/* arg to addfunc has the */
				/* FUNCADD_OUTPUT_ARGS bit on.*/
	AMPLFUNC_INARG  = 1,	/* IN or INOUT */
	AMPLFUNC_OUTARG = 2,	/* OUT or INOUT */
	AMPLFUNC_STRING = 4,	/* Input value is a string (sa[i]) */
	AMPLFUNC_STROUT = 8	/* String output value allowed */
};

enum FUNCADD_TYPE {			/* bits in "type" arg to addfunc */

		/* The type arg to addfunc should consist of one of the */
		/* following values ... */

	FUNCADD_double_VALUED = 0,	/* double (double) valued function */
	FUNCADD_STRING_VALUED = 2,	/* char* valued function (AMPL only) */
	FUNCADD_RANDOM_VALUED = 4,	/* double random valued */
	FUNCADD_012ARGS = 6,		/* Special case: double random valued */
					/* with 0 <= nargs <= 2 arguments */
					/* passed directly, rather than in */
					/* an arglist structure (AMPL only). */

		/* possibly or-ed with the following... */

	FUNCADD_STRING_ARGS = 1,	/* allow string args */
	FUNCADD_OUTPUT_ARGS = 16,	/* allow output args (AMPL only) */
	FUNCADD_TUPLE_VALUED = 32,	/* not yet allowed */

		/* internal use */
	FUNCADD_NO_ARGLIST = 8,
	FUNCADD_NO_DUPWARN = 64,	/* no complaint if already defined */
	FUNCADD_NONRAND_BUILTIN = 128	/* mean, variance, moment, etc. */
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

typedef void AddFunc (
	const char *name,
	rfunc f,	/* cast f to (rfunc) if it returns char* */
	int type,	/* see FUNCADD_TYPE above */
	int nargs,	/* >=  0 ==> exactly that many args
			 * <= -1 ==> at least -(nargs+1) args
			 */
	void *funcinfo,	/* for use by the function (if desired) */
	AmplExports *ae
);

typedef void AddRand (
	const char *name,
	rfunc f,	/* assumed to be a random function */
	rfunc icdf,	/* inverse CDF */
	int type,	/* FUNCADD_STRING_ARGS or 0 */
	int nargs,	/* >=  0 ==> exactly that many args
			 * <= -1 ==> at least -(nargs+1) args
			 */
	void *funcinfo,	/* for use by the function (if desired) */
	AmplExports *ae
);

typedef void (*RandSeedSetter) (void*, unsigned long);
typedef void AddRandInit (AmplExports *ae, RandSeedSetter, void*);
typedef void Exitfunc (void*);

struct
AuxInfo {
	AuxInfo *next;
	char *auxname;
	void *v;
	void (*f) (AmplExports*, void*, ...);
};

struct
AmplExports {
	FILE *StdErr;
	AddFunc *Addfunc;
	long ASLdate;
	int (*FprintF)  (FILE*, const char*, ...);
	int (*PrintF)   (const char*, ...);
	int (*SprintF)  (char*, const char*, ...);
	int (*VfprintF) (FILE*, const char*, va_list);
	int (*VsprintF) (char*, const char*, va_list);
	double (*Strtod) (const char*, char**);
	cryptblock *(*Crypto) (char *key, size_t scrbytes);
	Char *asl;
	void (*AtExit)  (AmplExports *ae, Exitfunc*, void*);
	void (*AtReset) (AmplExports *ae, Exitfunc*, void*);
	Char *(*Tempmem) (TMInfo*, size_t);
	void (*Add_table_handler) (
		int (*DbRead) (AmplExports *ae, TableInfo *TI),
		int (*DbWrite)(AmplExports *ae, TableInfo *TI),
		char *handler_info,
		int flags,
		void *Vinfo
	);
	Char *Private;
	void (*Qsortv) (void*, size_t, size_t, int(*)(const void*,const void*,void*), void*);

	/* More stuff for stdio in DLLs... */

	FILE	*StdIn;
	FILE	*StdOut;
	void	(*Clearerr)	(FILE*);
	int	(*Fclose)	(FILE*);
	FILE*	(*Fdopen)	(int, const char*);
	int	(*Feof)		(FILE*);
	int	(*Ferror)	(FILE*);
	int	(*Fflush)	(FILE*);
	int	(*Fgetc)	(FILE*);
	char*	(*Fgets)	(char*, int, FILE*);
	int	(*Fileno)	(FILE*);
	FILE*	(*Fopen)	(const char*, const char*);
	int	(*Fputc)	(int, FILE*);
	int	(*Fputs)	(const char*, FILE*);
	size_t	(*Fread)	(void*, size_t, size_t, FILE*);
	FILE*	(*Freopen)	(const char*, const char*, FILE*);
	int	(*Fscanf)	(FILE*, const char*, ...);
	int	(*Fseek)	(FILE*, long, int);
	long	(*Ftell)	(FILE*);
	size_t	(*Fwrite)	(const void*, size_t, size_t, FILE*);
	int	(*Pclose)	(FILE*);
	void	(*Perror)	(const char*);
	FILE*	(*Popen)	(const char*, const char*);
	int	(*Puts)		(const char*);
	void	(*Rewind)	(FILE*);
	int	(*Scanf)	(const char*, ...);
	void	(*Setbuf)	(FILE*, char*);
	int	(*Setvbuf)	(FILE*, char*, int, size_t);
	int	(*Sscanf)	(const char*, const char*, ...);
	char*	(*Tempnam)	(const char*, const char*);
	FILE*	(*Tmpfile)	(void);
	char*	(*Tmpnam)	(char*);
	int	(*Ungetc)	(int, FILE*);
	AuxInfo *AI;
	char*	(*Getenv)	(const char*);
	void	(*Breakfunc)	(int,void*);
	Char	*Breakarg;
	/* Items available with ASLdate >= 20020501 start here. */
	int (*SnprintF) (char*, size_t, const char*, ...);
	int (*VsnprintF) (char*, size_t, const char*, va_list);

	AddRand *Addrand;	/* for random function/inverse CDF pairs */
	AddRandInit *Addrandinit; /* for adding a function to receive a new random seed */
};

extern const char *i_option_ASL, *ix_details_ASL[];

#define funcadd funcadd_ASL

#if defined(_WIN32) && !defined(__MINGW32__)
__declspec(dllexport)
#endif
extern void funcadd (AmplExports*);	/* dynamically linked */
extern void af_libnamesave_ASL (AmplExports*, const char *fullname, const char *name, int nlen);
extern void note_libuse_ASL (void);	/* If funcadd() does not provide any imported */
						/* functions, it can call note_libuse_ASL() to */
						/* keep the library loaded; note_libuse_ASL() is */
						/* called, e.g., by the tableproxy table handler. */

#ifdef __cplusplus
	}
#endif

typedef struct
DbCol {
	double	*dval;
	char	**sval;
} DbCol;

struct
TableInfo {
	int (*AddRows) (TableInfo *TI, DbCol *cols, long nrows);
	char *tname;	/* name of this table */
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
	int (*Lookup) (double*, char**, TableInfo*);
	long (*AdjustMaxrows) (TableInfo*, long new_maxrows);
	void *(*ColAlloc) (TableInfo*, int ncol, int sval);
	long maxrows;
};

enum {	/* return values from (*DbRead)(...) and (*DbWrite)(...) */
	DB_Done = 0,	/* Table read or written. */
	DB_Refuse = 1,	/* Refuse to handle this table. */
	DB_Error = 2	/* Error reading or writing table. */
};

enum {	/* bits in flags field of TableInfo */
	DBTI_flags_IN = 1,	/* table has IN  or INOUT entities */
	DBTI_flags_OUT = 2,	/* table has OUT or INOUT entities */
	DBTI_flags_INSET = 4	/* table has "in set" phrase: */
				/* DbRead could omit rows for */
				/* which Lookup(...) == -1; AMPL */
				/* will ignore such rows if DbRead */
				/* offers them. */
};

// macro to allocate memory in AMPLs internal structures (if needed)
#define TM(len) (*ae->Tempmem)(TI->TMI,len)

