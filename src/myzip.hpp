/*
based on minizip.c
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
# include <sys/types.h>
# include <sys/stat.h>
#endif

#include "zip.h"

#ifdef _WIN32
#define USEWIN32IOAPI
#include "iowin32.h"
#endif



#define WRITEBUFFERSIZE (16384)
#define MAXFILENAME (256)

uLong filetime(
	char *f,                /* name of file to get info on */
	tm_zip *tmzip,             /* return value: access, modific. and creation times */
	uLong *dt             /* dostime */
);



int check_exist_file(
	const char* filename
);

/* calculate the CRC32 of a file,
   because to encrypt a file, we need known the CRC32 of the file before */
//~ int getFileCrc(
	//~ const char* filenameinzip,
	//~ void *buf,
	//~ unsigned long size_buf,
	//~ unsigned long* result_crc
//~ );

int isLargeFile(const char* filename);



int myzip(
	const char *zipfilename,
	const char *filename_to_replace,
	const char *filename_outside
);


//~ int main(argc,argv)
	//~ int argc;
	//~ char *argv[];
//~ {
	//~ int i;
	//~ int opt_overwrite=0;
	//~ int opt_compress_level=Z_DEFAULT_COMPRESSION;
	//~ int opt_exclude_path=0;
	//~ int zipfilenamearg = 0;
	//~ char filename_try[MAXFILENAME+16];
	//~ int zipok;
	//~ int err=0;
	//~ int size_buf=0;
	//~ void* buf=NULL;
	//~ const char* password=NULL;


	//~ do_banner();
	//~ if (argc==1)
	//~ {
		//~ do_help();
		//~ return 0;
	//~ }
	//~ else
	//~ {
		//~ for (i=1;i<argc;i++)
		//~ {
			//~ if ((*argv[i])=='-')
			//~ {
				//~ const char *p=argv[i]+1;

				//~ while ((*p)!='\0')
				//~ {
					//~ char c=*(p++);;
					//~ if ((c=='o') || (c=='O'))
						//~ opt_overwrite = 1;
					//~ if ((c=='a') || (c=='A'))
						//~ opt_overwrite = 2;
					//~ if ((c>='0') && (c<='9'))
						//~ opt_compress_level = c-'0';
					//~ if ((c=='j') || (c=='J'))
						//~ opt_exclude_path = 1;

					//~ if (((c=='p') || (c=='P')) && (i+1<argc))
					//~ {
						//~ password=argv[i+1];
						//~ i++;
					//~ }
				//~ }
			//~ }
			//~ else
			//~ {
				//~ if (zipfilenamearg == 0)
				//~ {
					//~ zipfilenamearg = i ;
				//~ }
			//~ }
		//~ }
	//~ }

	//~ size_buf = WRITEBUFFERSIZE;
	//~ buf = (void*)malloc(size_buf);
	//~ if (buf==NULL)
	//~ {
		//~ printf("Error allocating memory\n");
		//~ return ZIP_INTERNALERROR;
	//~ }

	//~ if (zipfilenamearg==0)
	//~ {
		//~ zipok=0;
	//~ }
	//~ else
	//~ {
		//~ int i,len;
		//~ int dot_found=0;

		//~ zipok = 1 ;
		//~ strncpy(filename_try, argv[zipfilenamearg],MAXFILENAME-1);
		//~ /* strncpy doesnt append the trailing NULL, of the string is too long. */
		//~ filename_try[ MAXFILENAME ] = '\0';

		//~ len=(int)strlen(filename_try);
		//~ for (i=0;i<len;i++)
			//~ if (filename_try[i]=='.')
				//~ dot_found=1;

		//~ if (dot_found==0)
			//~ strcat(filename_try,".zip");

		//~ if (opt_overwrite==2)
		//~ {
			//~ /* if the file don't exist, we not append file */
			//~ if (check_exist_file(filename_try)==0)
				//~ opt_overwrite=1;
		//~ }
		//~ else
		//~ if (opt_overwrite==0)
			//~ if (check_exist_file(filename_try)!=0)
			//~ {
				//~ char rep=0;
				//~ do
				//~ {
					//~ char answer[128];
					//~ int ret;
					//~ printf("The file %s exists. Overwrite ? [y]es, [n]o, [a]ppend : ",filename_try);
					//~ ret = scanf("%1s",answer);
					//~ if (ret != 1)
					//~ {
					   //~ exit(EXIT_FAILURE);
					//~ }
					//~ rep = answer[0] ;
					//~ if ((rep>='a') && (rep<='z'))
						//~ rep -= 0x20;
				//~ }
				//~ while ((rep!='Y') && (rep!='N') && (rep!='A'));
				//~ if (rep=='N')
					//~ zipok = 0;
				//~ if (rep=='A')
					//~ opt_overwrite = 2;
			//~ }
	//~ }

	//~ if (zipok==1)
	//~ {
		//~ zipFile zf;
		//~ int errclose;
//~ #ifdef USEWIN32IOAPI
		//~ zlib_filefunc64_def ffunc;
		//~ fill_win32_filefunc64A(&ffunc);
		//~ zf = zipOpen2_64(filename_try,(opt_overwrite==2) ? 2 : 0,NULL,&ffunc);
//~ #else
		//~ zf = zipOpen64(filename_try,(opt_overwrite==2) ? 2 : 0);
//~ #endif

		//~ if (zf == NULL)
		//~ {
			//~ printf("error opening %s\n",filename_try);
			//~ err= ZIP_ERRNO;
		//~ }
		//~ else
			//~ printf("creating %s\n",filename_try);

		//~ for (i=zipfilenamearg+1;(i<argc) && (err==ZIP_OK);i++)
		//~ {
			//~ if (!((((*(argv[i]))=='-') || ((*(argv[i]))=='/')) &&
					//~ ((argv[i][1]=='o') || (argv[i][1]=='O') ||
					//~ (argv[i][1]=='a') || (argv[i][1]=='A') ||
					//~ (argv[i][1]=='p') || (argv[i][1]=='P') ||
					//~ ((argv[i][1]>='0') || (argv[i][1]<='9'))) &&
					//~ (strlen(argv[i]) == 2)))
			//~ {
				//~ FILE * fin;
				//~ int size_read;
				//~ const char* filenameinzip = argv[i];
				//~ const char *savefilenameinzip;
				//~ zip_fileinfo zi;
				//~ unsigned long crcFile=0;
				//~ int zip64 = 0;

				//~ zi.tmz_date.tm_sec = zi.tmz_date.tm_min = zi.tmz_date.tm_hour =
				//~ zi.tmz_date.tm_mday = zi.tmz_date.tm_mon = zi.tmz_date.tm_year = 0;
				//~ zi.dosDate = 0;
				//~ zi.internal_fa = 0;
				//~ zi.external_fa = 0;
				//~ filetime(filenameinzip,&zi.tmz_date,&zi.dosDate);

//~ /*
				//~ err = zipOpenNewFileInZip(zf,filenameinzip,&zi,
								 //~ NULL,0,NULL,0,NULL / * comment * /,
								 //~ (opt_compress_level != 0) ? Z_DEFLATED : 0,
								 //~ opt_compress_level);
//~ */
				//~ if ((password != NULL) && (err==ZIP_OK))
					//~ err = getFileCrc(filenameinzip,buf,size_buf,&crcFile);

				//~ zip64 = isLargeFile(filenameinzip);

				//~ /* The path name saved, should not include a leading slash. */
				//~ /*if it did, windows/xp and dynazip couldn't read the zip file. */
				//~ savefilenameinzip = filenameinzip;
				//~ while( savefilenameinzip[0] == '\\' || savefilenameinzip[0] == '/' )
				//~ {
					//~ savefilenameinzip++;
				//~ }

				//~ /*should the zip file contain any path at all?*/
				//~ if( opt_exclude_path )
				//~ {
					//~ const char *tmpptr;
					//~ const char *lastslash = 0;
					//~ for( tmpptr = savefilenameinzip; *tmpptr; tmpptr++)
					//~ {
						//~ if( *tmpptr == '\\' || *tmpptr == '/')
						//~ {
							//~ lastslash = tmpptr;
						//~ }
					//~ }
					//~ if( lastslash != NULL )
					//~ {
						//~ savefilenameinzip = lastslash+1; // base filename follows last slash.
					//~ }
				//~ }

				 //~ /**/
				//~ err = zipOpenNewFileInZip3_64(zf,savefilenameinzip,&zi,
								 //~ NULL,0,NULL,0,NULL /* comment*/,
								 //~ (opt_compress_level != 0) ? Z_DEFLATED : 0,
								 //~ opt_compress_level,0,
								 //~ /* -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY, */
								 //~ -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY,
								 //~ password,crcFile, zip64);

				//~ if (err != ZIP_OK)
					//~ printf("error in opening %s in zipfile\n",filenameinzip);
				//~ else
				//~ {
					//~ fin = FOPEN_FUNC(filenameinzip,"rb");
					//~ if (fin==NULL)
					//~ {
						//~ err=ZIP_ERRNO;
						//~ printf("error in opening %s for reading\n",filenameinzip);
					//~ }
				//~ }

				//~ if (err == ZIP_OK)
					//~ do
					//~ {
						//~ err = ZIP_OK;
						//~ size_read = (int)fread(buf,1,size_buf,fin);
						//~ if (size_read < size_buf)
							//~ if (feof(fin)==0)
						//~ {
							//~ printf("error in reading %s\n",filenameinzip);
							//~ err = ZIP_ERRNO;
						//~ }

						//~ if (size_read>0)
						//~ {
							//~ err = zipWriteInFileInZip (zf,buf,size_read);
							//~ if (err<0)
							//~ {
								//~ printf("error in writing %s in the zipfile\n", filenameinzip);
							//~ }

						//~ }
					//~ } while ((err == ZIP_OK) && (size_read>0));

				//~ if (fin)
					//~ fclose(fin);

				//~ if (err<0)
					//~ err=ZIP_ERRNO;
				//~ else
				//~ {
					//~ err = zipCloseFileInZip(zf);
					//~ if (err!=ZIP_OK)
						//~ printf("error in closing %s in the zipfile\n",
									//~ filenameinzip);
				//~ }
			//~ }
		//~ }
		//~ errclose = zipClose(zf,NULL);
		//~ if (errclose != ZIP_OK)
			//~ printf("error in closing %s\n",filename_try);
	//~ }
	//~ else
	//~ {
	   //~ do_help();
	//~ }

	//~ free(buf);
	//~ return 0;
//~ }











