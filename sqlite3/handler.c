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
	
	if(TI->nstrings >= 2)
	{
		if(strcmp("sqlite3", TI->strings[0]))
		{
			return DB_Refuse;
		}
		const char *table_name = TI->tname;
		const char *db_fname = TI->strings[1];
		char *sql = (TI->nstrings >= 3) ? TI->strings[2] : NULL;
		char sql_buf[256];
		sqlite3 *db;
		int asked_ncols = TI->arity + TI->ncols;
		int rc = sqlite3_open(db_fname, &db);
		if(rc == SQLITE_OK)
		{
			sqlite3_stmt *stmt;
			rc = SQLITE_MISUSE;
			if(sql)
			{
				size_t sql_sz = strlen(sql);
				size_t sql_header_sz = sizeof("SQL=")-1;
				if(sql_sz > sql_header_sz) rc = sqlite3_prepare_v2(db, sql+sql_header_sz, -1, &stmt, NULL);
			}
			else
			{
				size_t sql_buf_size = sizeof(sql_buf);
				int used_size = 0;
				sql = sql_buf;
				int rc_sz = snprintf(sql_buf, sql_buf_size, "select ");
				used_size += rc_sz;
				const char *separator = "";
				for(int i=0; i < asked_ncols; ++i)
				{
					if(i > 0) separator = ",";
					rc_sz = snprintf(sql + used_size, sql_buf_size - used_size, "%s%s", separator, TI->colnames[i]);
					used_size += rc_sz;
				}
				rc_sz = snprintf(sql + used_size, sql_buf_size - used_size, " from %s", table_name);
				used_size += rc_sz;
				sql = sql_buf;
				//printf("SQL=%s\n", sql);
				rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
			}
			if(rc == SQLITE_OK)
			{
				DbCol *db = TI->cols;
				int ncols = sqlite3_column_count(stmt);
				if(ncols != asked_ncols)
				{
					result = DB_Error;
				}
				else
				{
					while(sqlite3_step(stmt) == SQLITE_ROW)
					{
						for(int icol=0; icol < ncols; ++icol)
						{
							switch(sqlite3_column_type(stmt, icol))
							{
								case SQLITE_TEXT:
									//const : we expect db->sval is readonly
									db->sval[icol] = sqlite3_column_text(stmt, icol);
									db->dval[icol] = 0.0;
								break;
								case SQLITE_INTEGER:
									db->sval[icol] = NULL;
									db->dval[icol] = (real)sqlite3_column_int64(stmt, icol);
								break;
								case SQLITE_FLOAT:
									db->sval[icol] = NULL;
									db->dval[icol] = sqlite3_column_double(stmt, icol);
								break;
								
								default:
									result = DB_Error;
							}
						}
						
						if ((*TI->AddRows)(TI, TI->cols, 1)){
							result = DB_Error;
						}
						if(result != 0) break;
					}
				}
				rc = sqlite3_finalize(stmt);
			}
			rc = sqlite3_close(db);
		}
	}
	return result;
};


static int Write_ampl_sqlite3(AmplExports *ae, TableInfo *TI){
	return DB_Refuse;
	//printf("Write_ampl_sqlite3\n");
	//showTableInfo(ae, TI);
	return 0;
};


/* called from ampl to load new functions */
void funcadd(AmplExports *ae) {
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
