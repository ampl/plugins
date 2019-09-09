#include "copyzip.hpp"

int
copy_uchanged_files(
	std::string& zip_string,
	std::string& zip_copy_string,
	std::vector<std::string>& removed_files)
{
	int res;
	const char* zip_name = &zip_string[0u];
	const char* tmp_name = &zip_copy_string[0u];

	//~ std::cout << "zip string: " << zip_string << std::endl;
	//~ std::cout << "zip copy string: " << zip_copy_string << std::endl;

	//~ for (int i=0; i<removed_files.size(); i++){

		//~ std::cout << "zip remove: " << removed_files[i] << std::endl;
	//~ }



	// open source and destination files

# ifdef USEWIN32IOAPI
	zlib_filefunc64_def ffunc;
	fill_win32_filefunc64A(&ffunc);
	zipFile szip = unzOpen2_64(zip_name, &ffunc);
# else
	zipFile szip = unzOpen64(zip_name);
# endif

	if (szip == NULL){
		//~ free(tmp_name);
		return -1;
	}

# ifdef USEWIN32IOAPI
	zipFile dzip = zipOpen2_64(tmp_name, APPEND_STATUS_CREATE, NULL, &ffunc);
# else
	zipFile dzip = zipOpen64(tmp_name, APPEND_STATUS_CREATE);
# endif

	if (dzip == NULL){
		unzClose(szip);
		//~ free(tmp_name);
		return -1;
	}

	// get global commentary
	unz_global_info glob_info;
	res = unzGetGlobalInfo(szip, &glob_info);
	if (res != UNZ_OK){
		zipClose(dzip, NULL);
		unzClose(szip);
		//~ free(tmp_name);
		return -1;
	}

	char* glob_comment = NULL;
	if (glob_info.size_comment > 0)
	{
		glob_comment = (char*)malloc(glob_info.size_comment + 1);

		if ((glob_comment == NULL) && (glob_info.size_comment != 0)){
			zipClose(dzip, NULL);
			unzClose(szip);
			//~ free(tmp_name);
			return -1;
		}

		res = unzGetGlobalComment(
			szip,
			glob_comment,
			glob_info.size_comment+1
		);

		if ((unsigned int)res != glob_info.size_comment){
			zipClose(dzip, NULL);
			unzClose(szip);
			free(glob_comment);
			//~ free(tmp_name);
			return -1;
		}
	}

	// copying files
	int n_files = 0;

	int rv = unzGoToFirstFile(szip);
	while (rv == UNZ_OK)
	{
		// get zipped file info
		unz_file_info64 unzfi;
		char dos_fn[MAX_PATH];

		res = unzGetCurrentFileInfo64(
			szip,
			&unzfi,
			dos_fn,
			MAX_PATH,
			NULL,
			0,
			NULL,
			0
		);

		//~ std::cout << szip << std::endl;
		//~ std::cout << unzfi << std::endl;
		//~ std::cout << dos_fn << std::endl;
		//~ std::cout << " " << std::endl;



		if (res != UNZ_OK){
			break;
		}

		//~ char fn[MAX_PATH];
		//~ OemToChar(dos_fn, fn); // <<<<<<<<<///////////////

		//~ // if not need delete this file
		//~ if (stricmp(fn, del_file) == 0) // lowercase comparison
			//~ some_was_del = true;


		bool found = false;

		for (int i = 0; i < removed_files.size(); i++){

			if (removed_files[i] == std::string(dos_fn)){
				found = true;
				break;
			}
		}

		if (!found){

			//~ std::cout << std::string(dos_fn) << " found..." << std::endl;

			char* extrafield = (char*)malloc(unzfi.size_file_extra);

			if ((extrafield == NULL) && (unzfi.size_file_extra != 0)){
				break;
			}

			char* commentary = (char*)malloc(unzfi.size_file_comment);

			if ((commentary == NULL) && (unzfi.size_file_comment != 0)){
				free(extrafield);
				break;
			}

			res = unzGetCurrentFileInfo64(
				szip,
				&unzfi,
				dos_fn,
				MAX_PATH,
				extrafield,
				unzfi.size_file_extra,
				commentary,
				unzfi.size_file_comment
			);

			if (res != UNZ_OK){
				free(extrafield);
				free(commentary);
				break;
			}

			// open file for RAW reading
			int method;
			int level;

			res = unzOpenCurrentFile2(szip, &method, &level, 1);

			if (res != UNZ_OK){
				free(extrafield);
				free(commentary);
				break;
			}

			int size_local_extra = unzGetLocalExtrafield(szip, NULL, 0);

			if (size_local_extra < 0){
				free(extrafield);
				free(commentary);
				break;
			}
			void* local_extra = malloc(size_local_extra);

			if ((local_extra == NULL) && (size_local_extra != 0)){
				free(extrafield);
				free(commentary);
				break;
			}

			res = unzGetLocalExtrafield(szip, local_extra, size_local_extra);

			if (res < 0){
				free(extrafield);
				free(commentary);
				free(local_extra);
				break;
			}

			// this malloc may fail if file very large
			void* buf = malloc(unzfi.compressed_size);

			if ((buf == NULL) && (unzfi.compressed_size != 0)){
				free(extrafield);
				free(commentary);
				free(local_extra);
				break;
			}

			// read file
			int sz = unzReadCurrentFile(szip, buf, unzfi.compressed_size);

			if ((unsigned int)sz != unzfi.compressed_size){
				free(extrafield);
				free(commentary);
				free(local_extra);
				free(buf);
				break;
			}

			// open destination file
			zip_fileinfo zfi;
			memcpy (&zfi.tmz_date, &unzfi.tmu_date, sizeof(tm_unz));
			zfi.dosDate = unzfi.dosDate;
			zfi.internal_fa = unzfi.internal_fa;
			zfi.external_fa = unzfi.external_fa;

			int zip64 = isLargeFile(dos_fn);

			res = zipOpenNewFileInZip2_64(
				dzip,
				dos_fn,
				&zfi,
				local_extra,
				size_local_extra,
				extrafield,
				unzfi.size_file_extra,
				commentary,
				method,
				level,
				1,
				zip64
			);

			if (res !=UNZ_OK){
				free(extrafield);
				free(commentary);
				free(local_extra);
				free(buf);
				break;
			}

			// write file
			res = zipWriteInFileInZip(dzip, buf, unzfi.compressed_size);

			if (res != UNZ_OK){
				free(extrafield);
				free(commentary);
				free(local_extra);
				free(buf);
				break;
			}

			res = zipCloseFileInZipRaw64(
				dzip,
				unzfi.uncompressed_size,
				unzfi.crc
			);

			if (res != UNZ_OK){
				free(extrafield);
				free(commentary);
				free(local_extra);
				free(buf);
				break;
			}

			res = unzCloseCurrentFile(szip);

			if (res == UNZ_CRCERROR){
				free(extrafield);
				free(commentary);
				free(local_extra);
				free(buf);
				break;
			}

			free(commentary);
			free(buf);
			free(extrafield);
			free(local_extra);

			n_files ++;
		}

		rv = unzGoToNextFile(szip);
	}

	zipClose(dzip, glob_comment);
	unzClose(szip);

	free(glob_comment);
	

	// if fail
	if (rv != UNZ_END_OF_LIST_OF_FILE)
	{
		remove(tmp_name);
		//~ free(tmp_name);
		return -1;
	}

	//~ remove(zip_name);
	//~ if (rename(tmp_name, zip_name) != 0)
	//~ {
		//~ free(tmp_name);
		//~ return -1;
	//~ }

	//~ free(tmp_name);
	return 0;
};

//~ int main(){

	//~ std::string zip_name = std::string("/home/nsantos/Documents/ampl/to_kill/t6.xlsx");
	//~ std::string zip_copy_name = std::string("/home/nsantos/Documents/ampl/temp/xlcopy.tmp");

	//~ std::vector<std::string> to_remove;

	//~ to_remove.push_back("xl/sharedStrings.xml");


	//~ std::cout << zip_name << std::endl;



	//~ copy_uchanged_files(zip_name, zip_copy_name, to_remove);

	//~ return 0;
//~ };

//g++ -I../zlib-1.2.11 -std=c++03 -g -c  myunz.cpp unzip.c ioapi.c myzip.cpp zip.c temp.cpp
//g++ -o cenas myunz.o unzip.o ioapi.o myzip.o zip.o temp.o -Wl,--whole-archive ../zlib-1.2.11/libz.a -Wl,--no-whole-archive


