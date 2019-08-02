/* Experimental bridge between AMPL and sqlite3 allowing users to
 * read/write data from sqlite3 databases from AMPL.
 *
 * mingodad@gmail.com, July 2019.
 */

#include <string.h>
#include <stdlib.h>
#include <sqlite3.h>
#include "funcadd.h"

#define AMPLEXPORTS_KEY "AmplExports"
#define AMP_ADDFUNC_KEY "AmplAddFunc"

void showTableInfo(AmplExports *ae, TableInfo *TI)
{
	int i;
	printf("TableInfo\n");
	printf("Name = %s\n", TI->tname);
	printf("flags = %d, nstrings = %d, arity = %d, ncols = %d, nrows = %d, maxrows = %d\n",
		TI->flags, TI->nstrings, TI->arity, TI->ncols, (int)TI->nrows, (int)TI->maxrows);
	for(i=0; i < TI->nstrings; ++i)
		printf("String %d = %s\n", i, TI->strings[i]);

	printf("Arity cols\n");
	int ncols = TI->arity;
	for(i=0; i < ncols; ++i)
		printf("ColName %d = %s\n", i, TI->colnames[i]);

	printf("Other cols\n");
	ncols += TI->ncols;
	for(; i < ncols; ++i)
		printf("ColName %d = %s\n", i, TI->colnames[i]);
}

static int Read_ampl_sqlite3(AmplExports *ae, TableInfo *TI){
	int result = 0;
	//printf("Read_ampl_sqlite3\n");
	//showTableInfo(ae, TI);
	
	//(flags & DBTI_flags_IN) update else insert
	
	if(TI->nstrings >= 2)
	{
		if(strcmp("sqlite3", TI->strings[0]))
		{
			return DB_Refuse;
		}
		const char *table_name = TI->tname;
		const char *db_fname = TI->strings[1];
		char *sql = (TI->nstrings >= 3) ? TI->strings[2] : NULL;
		sqlite3 *sq3db;
		int asked_ncols = TI->arity + TI->ncols;
		int rc = sqlite3_open(db_fname, &sq3db);
		if(rc == SQLITE_OK)
		{
			sqlite3_stmt *stmt;
			rc = SQLITE_MISUSE;
			if(sql)
			{
				size_t sql_sz = strlen(sql);
				size_t sql_header_sz = sizeof("SQL=")-1;
				if(sql_sz > sql_header_sz) rc = sqlite3_prepare_v2(sq3db, sql+sql_header_sz, -1, &stmt, NULL);
			}
			else
			{
				size_t field_names_size = 0;
				for(int i=0; i < asked_ncols; ++i)
				{
					field_names_size += strlen(TI->colnames[i])+1; /* +1 for the separator */
				}
				size_t sql_buf_size = field_names_size+1; /* +1 for '\0' terminator */
				char *buf = sqlite3_malloc(sql_buf_size);
				if(buf)
				{
					const char *separator = "";
					int rc_sz;
					int used_size = 0;
					sql = buf;
					buf[0] = '\0';
					for(int i=0; i < asked_ncols; ++i)
					{
						if(i > 0) separator = ",";
						rc_sz = snprintf(sql + used_size, sql_buf_size - used_size, "%s%s", separator, TI->colnames[i]);
						used_size += rc_sz;
					}
					sql = sqlite3_mprintf("select %s from %s", buf, table_name);
					sqlite3_free(buf);
					if(sql)
					{
						rc = sqlite3_prepare_v2(sq3db, sql, -1, &stmt, NULL);
						//printf("SQL=%s\n", sql);
						sqlite3_free(sql);
						sql = NULL;
					}
				}
			}
			if(rc == SQLITE_OK)
			{
				int ncols = sqlite3_column_count(stmt);
				if(ncols != asked_ncols)
				{
					result = DB_Error;
				}
				else
				{
					while(sqlite3_step(stmt) == SQLITE_ROW)
					{
						DbCol *db = TI->cols;
						for(int icol=0; icol < ncols; ++icol)
						{
							switch(sqlite3_column_type(stmt, icol))
							{
								case SQLITE_TEXT:
									//const : we expect db->sval is readonly
									db->sval[0] = sqlite3_column_text(stmt, icol);
									db->dval[0] = 0.0;
								break;
								case SQLITE_INTEGER:
									db->sval[0] = NULL;
									db->dval[0] = (real)sqlite3_column_int64(stmt, icol);
								break;
								case SQLITE_FLOAT:
									db->sval[0] = NULL;
									db->dval[0] = sqlite3_column_double(stmt, icol);
								break;
								
								default:
									result = DB_Error;
							}
							++db;
						}
						
						if ((*TI->AddRows)(TI, TI->cols, 1)){
							result = DB_Error;
						}
						if(result != 0) break;
					}
				}
				rc = sqlite3_finalize(stmt);
			}
			rc = sqlite3_close(sq3db);
		}
	}
	return result;
};


static int bind_param(sqlite3_stmt *stmt, int stmt_icol, DbCol *db, int irow)
{
	int rc = SQLITE_MISUSE;
	if(db->sval) rc = sqlite3_bind_text(stmt, stmt_icol, db->sval[irow], -1, NULL);
	else rc = sqlite3_bind_double(stmt, stmt_icol, db->dval[irow]);
	return rc;
}


static int Write_ampl_sqlite3(AmplExports *ae, TableInfo *TI){
	//return DB_Refuse;
	//printf("Write_ampl_sqlite3\n");
	//showTableInfo(ae, TI);
	/*
	To ask David about the difference between nrows and maxrows
	*/
	int result = 0;

	if(TI->nstrings >= 3)
	{
		if(strcmp("sqlite3", TI->strings[0]))
		{
			return DB_Refuse;
		}
		const char *table_name = TI->tname;
		const char *db_fname = TI->strings[1];
		char *sql = (TI->nstrings >= 3) ? TI->strings[2] : NULL;
		sqlite3 *sq3db;
		int asked_ncols = TI->arity + TI->ncols;
		int rc = sqlite3_open(db_fname, &sq3db);
		if(rc == SQLITE_OK)
		{
			sqlite3_stmt *stmt;
			rc = SQLITE_MISUSE;
			if(sql)
			{
				size_t sql_sz = strlen(sql);
				size_t sql_header_sz = sizeof("SQL=")-1;
				if(sql_sz > sql_header_sz) rc = sqlite3_prepare_v2(sq3db, sql+sql_header_sz, -1, &stmt, NULL);
			}
			if(rc == SQLITE_OK)
			{
				int ncols = sqlite3_bind_parameter_count(stmt);
				if(ncols != asked_ncols)
				{
					result = DB_Error;
				}
				else
				{
					DbCol *db = TI->cols;
					rc = sqlite3_exec(sq3db, "begin", NULL, NULL, NULL);
					for(int irow=0; irow < TI->nrows; ++irow)
					{	
						for(int icol=1; icol <= TI->ncols; ++icol)
						{
							rc = bind_param(stmt, icol, db +(icol + TI->arity-1), irow);
							if(rc != SQLITE_OK) break;
						}
						for(int icol=1; icol <= TI->arity; ++icol)
						{
							rc = bind_param(stmt, icol + TI->ncols, db + (icol-1), irow);
							if(rc != SQLITE_OK) break;
						}
						if(rc == SQLITE_OK)
						{
							rc = sqlite3_step(stmt);
							if(rc == SQLITE_DONE)
							{
								rc = sqlite3_reset(stmt);
							}
						}						
					}
					rc = sqlite3_exec(sq3db, "commit", NULL, NULL, NULL);
				}
				rc = sqlite3_finalize(stmt);
			}
			rc = sqlite3_close(sq3db);
		}
	}
	return result;
};

#if defined(_WIN32)
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT 
#endif

/* called from ampl to load new functions */
DLL_EXPORT void funcadd(AmplExports *ae) {
	static char tname[] = "sqlite3\n"
	"AMPL SQLITE3 handler (20190730): expected 2 or 3 strings before \":[...]\":\n"
	"  'sqlite3, database file name, [option [option...]]\n"
	"The first string should be 'sqlite3's\n"
	"The second string should be the sqlite3 database filename\n"
	"If there is a third string it's expected to que a query to get the rows as 'SQL=sqlstmt'\n"
	"otherwise the query will be made against the table name and the fields declared.\n"
	;
    if (!ae->asl)
	add_table_handler(Read_ampl_sqlite3, Write_ampl_sqlite3, tname, DBTI_flags_IN | DBTI_flags_OUT, 0);
}
