#include "myzip.hpp"

#ifdef _WIN32
uLong filetime(
	char *f,                /* name of file to get info on */
	tm_zip *tmzip,             /* return value: access, modific. and creation times */
	uLong *dt             /* dostime */
){
	int ret = 0;
	{
		FILETIME ftLocal;
		HANDLE hFind;
		WIN32_FIND_DATAA ff32;

		hFind = FindFirstFileA(f,&ff32);
		if (hFind != INVALID_HANDLE_VALUE)
		{
			FileTimeToLocalFileTime(&(ff32.ftLastWriteTime),&ftLocal);
			FileTimeToDosDateTime(&ftLocal,((LPWORD)dt)+1,((LPWORD)dt)+0);
			FindClose(hFind);
			ret = 1;
		}
	}
	return ret;
}
#else
#if defined (unix) || defined(__APPLE__)
uLong filetime(
	char *f,               /* name of file to get info on */
	tm_zip *tmzip,         /* return value: access, modific. and creation times */
	uLong *dt             /* dostime */
){
	int ret=0;
	struct stat s;        /* results of stat() */
	struct tm* filedate;
	time_t tm_t=0;

	if (strcmp(f,"-")!=0)
	{
		char name[MAXFILENAME+1];
		int len = strlen(f);
		if (len > MAXFILENAME)
			len = MAXFILENAME;

		strncpy(name, f,MAXFILENAME-1);
		/* strncpy doesnt append the trailing NULL, of the string is too long. */
		name[ MAXFILENAME ] = '\0';

		if (name[len - 1] == '/')
			name[len - 1] = '\0';
		/* not all systems allow stat'ing a file with / appended */
		if (stat(name,&s)==0)
		{
			tm_t = s.st_mtime;
			ret = 1;
		}
	}
	filedate = localtime(&tm_t);

	tmzip->tm_sec  = filedate->tm_sec;
	tmzip->tm_min  = filedate->tm_min;
	tmzip->tm_hour = filedate->tm_hour;
	tmzip->tm_mday = filedate->tm_mday;
	tmzip->tm_mon  = filedate->tm_mon ;
	tmzip->tm_year = filedate->tm_year;

	return ret;
}
#else
uLong filetime(
	char *f,                /* name of file to get info on */
	tm_zip *tmzip,             /* return value: access, modific. and creation times */
	uLong *dt             /* dostime */
){
	return 0;
}
#endif
#endif




int check_exist_file(
	const char* filename
){
	FILE* ftestexist;
	int ret = 1;
	ftestexist = FOPEN_FUNC(filename,"rb");
	if (ftestexist==NULL)
		ret = 0;
	else
		fclose(ftestexist);
	return ret;
}


//~ /* calculate the CRC32 of a file,
   //~ because to encrypt a file, we need known the CRC32 of the file before */
//~ int getFileCrc(const char* filenameinzip,void*buf,unsigned long size_buf,unsigned long* result_crc)
//~ {
	//~ unsigned long calculate_crc=0;
	//~ int err=ZIP_OK;
	//~ FILE * fin = FOPEN_FUNC(filenameinzip,"rb");

	//~ unsigned long size_read = 0;
	//~ unsigned long total_read = 0;
	//~ if (fin==NULL)
	//~ {
		//~ err = ZIP_ERRNO;
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
				//~ calculate_crc = crc32(calculate_crc,buf,size_read);
			//~ total_read += size_read;

		//~ } while ((err == ZIP_OK) && (size_read>0));

	//~ if (fin)
		//~ fclose(fin);

	//~ *result_crc=calculate_crc;
	//~ printf("file %s crc %lx\n", filenameinzip, calculate_crc);
	//~ return err;
//~ }

int isLargeFile(const char* filename)
{
	int largeFile = 0;
	ZPOS64_T pos = 0;
	FILE* pFile = FOPEN_FUNC(filename, "rb");

	if(pFile != NULL)
	{
		int n = FSEEKO_FUNC(pFile, 0, SEEK_END);
		pos = FTELLO_FUNC(pFile);

		//~ printf("File : %s is %lld bytes\n", filename, pos);

		if(pos >= 0xffffffff)
			largeFile = 1;

		fclose(pFile);
	}

	return largeFile;
}

int myzip(
	const char *zipfilename,
	const char *filename_to_replace,
	const char *filename_outside
)
{
	int i;
	int opt_overwrite=0;
	int opt_compress_level=Z_DEFAULT_COMPRESSION;
	int opt_exclude_path=0;
	int zipfilenamearg = 0;
	char filename_try[MAXFILENAME+16];
	int zipok;
	int err=0;
	int size_buf=0;
	void* buf=NULL;
	const char* password=NULL;

	opt_overwrite = 2;
	//~ zipfilenamearg = zipfilename;

	size_buf = WRITEBUFFERSIZE;
	buf = (void*)malloc(size_buf);
	if (buf == NULL)
	{
		//~ printf("Error allocating memory\n");
		return ZIP_INTERNALERROR;
	}

	int len;
	int dot_found=0;

	zipok = 1 ;

	strncpy(filename_try, zipfilename, MAXFILENAME-1);
	/* strncpy doesnt append the trailing NULL, of the string is too long. */
	filename_try[ MAXFILENAME ] = '\0';

	len=(int)strlen(filename_try);
	for (i=0;i<len;i++)
		if (filename_try[i]=='.')
			dot_found=1;

	if (dot_found==0)
		strcat(filename_try,".zip");


	zipFile zf;
	int errclose;
#ifdef USEWIN32IOAPI
	zlib_filefunc64_def ffunc;
	fill_win32_filefunc64A(&ffunc);
	zf = zipOpen2_64(filename_try,(opt_overwrite==2) ? 2 : 0,NULL,&ffunc);
#else
	zf = zipOpen64(filename_try,(opt_overwrite==2) ? 2 : 0);
#endif

	if (zf == NULL)
	{
		//~ printf("error opening %s\n",filename_try);
		err= ZIP_ERRNO;
	}
	else{
		//~ printf("creating %s\n",filename_try);
	}

	//------------------------------------------------------------------
	FILE * fin;
	int size_read;
	//~ const char* filenameinzip = argv[i];
	const char* filenameinzip = filename_to_replace;
	const char *savefilenameinzip;
	zip_fileinfo zi;
	unsigned long crcFile=0;
	int zip64 = 0;

	zi.tmz_date.tm_sec = zi.tmz_date.tm_min = zi.tmz_date.tm_hour =
	zi.tmz_date.tm_mday = zi.tmz_date.tm_mon = zi.tmz_date.tm_year = 0;
	zi.dosDate = 0;
	zi.internal_fa = 0;
	zi.external_fa = 0;
	filetime((char*)filenameinzip,&zi.tmz_date,&zi.dosDate);

	// no passwords allowed
	//~ if ((password != NULL) && (err==ZIP_OK))
		//~ err = getFileCrc(filenameinzip,buf,size_buf,&crcFile);

	zip64 = isLargeFile(filenameinzip);

	/* The path name saved, should not include a leading slash. */
	/*if it did, windows/xp and dynazip couldn't read the zip file. */
	savefilenameinzip = filenameinzip;



	/*
	while( savefilenameinzip[0] == '\\' || savefilenameinzip[0] == '/' )
	{
		savefilenameinzip++;
	}

	const char *tmpptr;
	const char *lastslash = 0;
	for( tmpptr = savefilenameinzip; *tmpptr; tmpptr++)
	{
		if( *tmpptr == '\\' || *tmpptr == '/')
		{
			lastslash = tmpptr;
		}
	}
	if( lastslash != NULL )
	{
		savefilenameinzip = lastslash+1; // base filename follows last slash.
	}
	*/

	//~ printf("filenameinzip %s in zipfile\n",filenameinzip);
	//~ printf("savefilenameinzip %s in zipfile\n",savefilenameinzip);

	err = zipOpenNewFileInZip3_64(zf,filename_to_replace,&zi, // HERE WE OPEN
					 NULL,0,NULL,0,NULL /* comment*/,
					 (opt_compress_level != 0) ? Z_DEFLATED : 0,
					 opt_compress_level,0,
					 /* -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY, */
					 -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY,
					 password,crcFile, zip64);

	if (err != ZIP_OK){
		//~ printf("error in opening %s in zipfile\n",filenameinzip);
	}
	else
	{
		fin = FOPEN_FUNC(filename_outside,"rb"); // </ here we read
		if (fin==NULL)
		{
			err=ZIP_ERRNO;
			//~ printf("error in opening %s for reading\n",filenameinzip);
		}
	}

	if (err == ZIP_OK)
		do
		{
			err = ZIP_OK;
			size_read = (int)fread(buf,1,size_buf,fin);
			if (size_read < size_buf)
				if (feof(fin)==0)
			{
				//~ printf("error in reading %s\n",filenameinzip);
				err = ZIP_ERRNO;
			}

			if (size_read>0)
			{
				err = zipWriteInFileInZip (zf,buf,size_read);
				if (err<0)
				{
					//~ printf("error in writing %s in the zipfile\n", filenameinzip);
				}

			}
		} while ((err == ZIP_OK) && (size_read>0));

	if (fin)
		fclose(fin);

	if (err<0)
		err=ZIP_ERRNO;
	else
	{
		err = zipCloseFileInZip(zf);
		if (err!=ZIP_OK){
			//~ printf("error in closing %s in the zipfile\n",filenameinzip);
		}
	}

	errclose = zipClose(zf,NULL);
	if (errclose != ZIP_OK){
		//~ printf("error in closing %s\n",filename_try);
	}

	free(buf);
	return 0;
}











