#include "copyzip.hpp"

int
copy_uchanged_files(
	std::string& zip_orig,
	std::string& zip_dest,
	std::vector<std::string>& removed_files)
{
	int res;

	// open source and destination files
# ifdef USEWIN32IOAPI
	zlib_filefunc64_def ffunc;
	fill_win32_filefunc64A(&ffunc);
	zipFile szip = unzOpen2_64(zip_orig.c_str(), &ffunc);
# else
	zipFile szip = unzOpen64(zip_orig.c_str());
# endif

	if (szip == NULL){
		// could not open source zip
		return -1;
	}

# ifdef USEWIN32IOAPI
	zipFile dzip = zipOpen2_64(zip_dest.c_str(), APPEND_STATUS_CREATE, NULL, &ffunc);
# else
	zipFile dzip = zipOpen64(zip_dest.c_str(), APPEND_STATUS_CREATE);
# endif

	if (dzip == NULL){
		// could not open destin zip
		unzClose(szip);
		return -1;
	}

	// get global commentary
	unz_global_info glob_info;
	res = unzGetGlobalInfo(szip, &glob_info);
	if (res != UNZ_OK){
		// could not get global info from source zip
		zipClose(dzip, NULL);
		unzClose(szip);
		return -1;
	}

	char* glob_comment = NULL;
	if (glob_info.size_comment > 0)
	{
		glob_comment = (char*)malloc(glob_info.size_comment + 1);

		if ((glob_comment == NULL) && (glob_info.size_comment != 0)){
			// could not alloc glob_comment
			zipClose(dzip, NULL);
			unzClose(szip);
			return -1;
		}

		res = unzGetGlobalComment(
			szip,
			glob_comment,
			glob_info.size_comment+1
		);

		if ((unsigned int)res != glob_info.size_comment){
			// could not get glob_comment
			zipClose(dzip, NULL);
			unzClose(szip);
			free(glob_comment);
			return -1;
		}
	}

	// copy files
	int n_files = 0;

	// iterate files in zip
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

		if (res != UNZ_OK){
			break;
		}

		// check if it is one the files to remove
		bool found = false;
		for (int i = 0; i < removed_files.size(); i++){

			if (removed_files[i] == dos_fn){
				found = true;
				break;
			}
		}

		if (!found){

			// file was not found, copy to destin zip

			char* extrafield = NULL;
			if (unzfi.size_file_extra > 0){
				extrafield = (char*)malloc(unzfi.size_file_extra);

				// check allocation
				if (extrafield == NULL){
					// could not allocate extrafield
					break;
				}
			}

			char* commentary = NULL;
			if (unzfi.size_file_comment > 0){
				commentary = (char*)malloc(unzfi.size_file_comment);

				// check allocation
				if (commentary == NULL){
					// could not allocate commentary
					free(extrafield);
					break;
				}
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
				// could not get file info
				free(extrafield);
				free(commentary);
				break;
			}

			// open file for RAW reading
			int method;
			int level;

			res = unzOpenCurrentFile2(szip, &method, &level, 1);

			if (res != UNZ_OK){
				// could not open file
				free(extrafield);
				free(commentary);
				break;
			}

			int size_local_extra = unzGetLocalExtrafield(szip, NULL, 0);

			if (size_local_extra < 0){
				// invalid size_local_extra
				free(extrafield);
				free(commentary);
				break;
			}

			void* local_extra = NULL;
			if (size_local_extra > 0){
				local_extra = malloc(size_local_extra);

				// check allocation
				if (local_extra == NULL){
					// could not allocate local_extra
					free(extrafield);
					free(commentary);
					break;
				}
			}

			res = unzGetLocalExtrafield(szip, local_extra, size_local_extra);

			if (res < 0){
				// could not get local extrafield
				free(extrafield);
				free(commentary);
				free(local_extra);
				break;
			}

			// this malloc may fail if file very large
			void* buf = NULL;
			if (unzfi.compressed_size > 0){
				buf = malloc(unzfi.compressed_size);

				// check allocation
				if (buf == NULL){
					// could not allocate buffer
					free(extrafield);
					free(commentary);
					free(local_extra);
					break;
				}
			}

			// read file
			int sz = unzReadCurrentFile(szip, buf, unzfi.compressed_size);

			if ((unsigned int)sz != unzfi.compressed_size){
				// size read and size info mismatch
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

//~ std::cout << std::endl;
//~ std::cout << "dzip\t" << dzip << std::endl;
//~ std::cout << "dos_fn\t" << dos_fn << std::endl;
//~ std::cout << "zfi\t" << &zfi << std::endl;
//~ std::cout << "local_extra\t" << (char*)local_extra << std::endl;
//~ std::cout << "size_local_extra\t" << size_local_extra << std::endl;
//~ std::cout << "extrafield\t" << extrafield << std::endl;
//~ std::cout << "unzfi.size_file_extra\t" << unzfi.size_file_extra << std::endl;
//~ std::cout << "commentary\t" << commentary << std::endl;
//~ std::cout << "method\t" << method << std::endl;
//~ std::cout << "level\t" << level << std::endl;
//~ std::cout << "1\t" << 1 << std::endl;
//~ std::cout << "zip64\t" << zip64 << std::endl;
//~ std::cout << std::endl;

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
				// could not open file in dest zip
				free(extrafield);
				free(commentary);
				free(local_extra);
				free(buf);
				break;
			}

			// write file
			res = zipWriteInFileInZip(dzip, buf, unzfi.compressed_size);

			if (res != UNZ_OK){
				// could not write file in dest zip
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
				// could not close file
				free(extrafield);
				free(commentary);
				free(local_extra);
				free(buf);
				break;
			}

			res = unzCloseCurrentFile(szip);

			if (res == UNZ_CRCERROR){
				// could not close file in source zip
				free(extrafield);
				free(commentary);
				free(local_extra);
				free(buf);
				break;
			}

			// everything went well
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
		remove(zip_dest.c_str());
		return -1;
	}

	return 0;
};
