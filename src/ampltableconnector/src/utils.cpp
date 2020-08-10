#include "masterheader.hpp"


std::string
get_file_extension(const std::string & filepath){

	if(filepath.find_last_of(".") != std::string::npos){
		return filepath.substr(filepath.find_last_of(".") + 1);
	}
	return std::string();
};


//~ bool
//~ check_file_exists(const std::string & filename){
	//~ std::ifstream ifile(filename.c_str());
	//~ return static_cast<bool>(ifile);
//~ };


bool
check_file_exists (const std::string & filename) {
	if (FILE *file = fopen(filename.c_str(), "r")) {
		fclose(file);
		return true;
	} else {
		return false;
	}   
}


bool
compare_strings_lower(const std::string & str1, const std::string & str2){

	if (str1.size() != str2.size()){
		return false;
	}

	for (int i = 0; i < str1.size(); i++){
		if (std::tolower(str1[i]) != std::tolower(str2[i])){
			return false;
		}
	}
	return true;
};




