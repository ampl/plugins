#include "utils.hpp"


std::string
get_file_extension(const std::string & filepath){

	if(filepath.find_last_of(".") != std::string::npos){
		return filepath.substr(filepath.find_last_of(".") + 1);
	}
	return std::string();
};


bool
check_file_exists(const std::string & filename){
	std::ifstream ifile(filename.c_str());
	return static_cast<bool>(ifile);
};


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


void try_unquote_string(std::string & str, char* quotechar){

	size_t n = str.size();
	if (str[0] == *quotechar && str.size() > 1 && str[n-1] == *quotechar){
		str = str.substr(1, n-2);
	}

	int deb = 1;
};


std::istream& safeGetline(std::istream& is, std::string& t)
{
	t.clear();

	// The characters in the stream are read one-by-one using a std::streambuf.
	// That is faster than reading them one-by-one using the std::istream.
	// Code that uses streambuf this way must be guarded by a sentry object.
	// The sentry object performs various tasks,
	// such as thread synchronization and updating the stream state.

	std::istream::sentry se(is, true);
	std::streambuf* sb = is.rdbuf();

	// original code for reference
	// for(;;) {
		// int c = sb->sbumpc();
		// switch (c) {
		// case '\n':
			// return is;
		// case '\r':
			// if(sb->sgetc() == '\n')
				// sb->sbumpc();
			// return is;
		// case std::streambuf::traits_type::eof():
			// // Also handle the case when the last line has no line ending
			// if(t.empty())
				// is.setstate(std::ios::eofbit);
			// return is;
		// default:
			// t += (char)c;
		// }
	// }

	for(;;) {
		int c = sb->sbumpc();
		if (c == '\n'){
			return is;
		}
		else if (c == '\r'){
			if(sb->sgetc() == '\n'){
				sb->sbumpc();
			}
			return is;
		}
		else if (c == std::streambuf::traits_type::eof()){
			// Also handle the case when the last line has no line ending
			if(t.empty()){
				is.setstate(std::ios::eofbit);
			}
			return is;
		}
		else{
			t += (char)c;
		}
	}
}


void
copy_file(const std::string & source_path, const std::string & dest_path){
	std::ifstream source(source_path.c_str(), std::ios::binary);
	std::ofstream dest(dest_path.c_str(), std::ios::binary);
	dest << source.rdbuf();
};
