/*
based in miniunzip.c
*/

#pragma once


#if (!defined(_WIN32)) && (!defined(WIN32)) && (!defined(__APPLE__))
		#ifndef __USE_FILE_OFFSET64
				#define __USE_FILE_OFFSET64
		#endif
		#ifndef __USE_LARGEFILE64
				#define __USE_LARGEFILE64
		#endif
		#ifndef _LARGEFILE64_SOURCE
				#define _LARGEFILE64_SOURCE
		#endif
		#ifndef _FILE_OFFSET_BIT
				#define _FILE_OFFSET_BIT 64
		#endif
#endif

#ifdef __APPLE__
// In darwin and perhaps other BSD variants off_t is a 64 bit value, hence no need for specific 64 bit functions
#define FOPEN_FUNC(filename, mode) fopen(filename, mode)
#define FTELLO_FUNC(stream) ftello(stream)
#define FSEEKO_FUNC(stream, offset, origin) fseeko(stream, offset, origin)
#else
#define FOPEN_FUNC(filename, mode) fopen64(filename, mode)
#define FTELLO_FUNC(stream) ftello64(stream)
#define FSEEKO_FUNC(stream, offset, origin) fseeko64(stream, offset, origin)
#endif


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>

#ifdef _WIN32
# include <direct.h>
# include <io.h>
#else
# include <unistd.h>
# include <utime.h>
# include <sys/stat.h>
#endif


#include "unzip.h"

#define CASESENSITIVITY (0)
#define WRITEBUFFERSIZE (8192)
#define MAXFILENAME (256)

#ifdef _WIN32
#define USEWIN32IOAPI
#include "iowin32.h"
#endif


/* change_file_date : change the date/time of a file
	filename : the filename of the file where date/time must be modified
	dosdate : the new date at the MSDos format (4 bytes)
	tmu_date : the SAME new date at the tm_unz format */
void change_file_date(
	const char *filename,
	uLong dosdate,
	tm_unz tmu_date
);

/* mymkdir and change_file_date are not 100 % portable
   As I don't know well Unix, I wait feedback for the unix portion */

int mymkdir(
	const char* dirname
);

int makedir (
	const char *newdir
);


void Display64BitsSize(
	ZPOS64_T n,
	int size_char
);


int do_list(
	unzFile uf
);


int do_extract_currentfile(
	unzFile uf,
	const int* popt_extract_without_path,
	int* popt_overwrite,
	const char* password,
	const char* dirname
);


int do_extract(
	unzFile uf,
	int opt_extract_without_path,
	int opt_overwrite,
	const char* password,
	const char* dirname
);


int do_extract_onefile(
	unzFile uf,
	const char* filename,
	int opt_extract_without_path,
	int opt_overwrite,
	const char* password,
	const char* dirname
);


int myunzip(
	const char *zipfilename,
	const char *filename_to_extract,
	const char *dirname
);

/*
int main(
    int argc,
    char *argv[]
){


	char mybuff[100];

	//~ char* mybuffptr = &mybuff;

	printf("wd: %s\n", getcwd(&mybuff[0], 100));

	const char *zipfilename = argv[1];
	const char *filename_to_extract = argv[2];
	const char *dirname = argv[3];

	int myresponse = myunzip(zipfilename, filename_to_extract, dirname);

	//~ unzFile uf;
	//~ uf = unzOpen64(zipfilename);
	//~ printf("unzOpen64 value: %p\n", uf);
	//~ int myresponse = unzClose(uf);
	//~ printf("unzClose value: %d\n", myresponse);


	//~ uf = unzOpen64(zipfilename);
	//~ printf("unzOpen64 value: %p\n", uf);
	//~ myresponse = unzClose(uf);
	//~ printf("unzClose value: %d\n", myresponse);


	//~ uf = unzOpen64(zipfilename);
	//~ printf("unzOpen64 value: %p\n", uf);
	//~ myresponse = unzClose(uf);
	//~ printf("unzClose value: %d\n", myresponse);

	printf("wd: %s\n", getcwd(&mybuff[0], 100));


	return myresponse;
};
*/







