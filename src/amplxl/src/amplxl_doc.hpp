#pragma once

#include <string.h>

std::string name = "amplxl";
std::string version = "0.1.12";

std::string doc = name + "\n" + name + "-" + version + "\n"
"\n"
"A table handler for spreadsheet files (.xlsx).\n"
"\n"
"General information on table handlers and data correspondence between AMPL and \n"
"an external table is available at chapter 10 of the AMPL book:\n"
"\n"
"    https://ampl.com/learn/ampl-book/\n"
"\n"
"The available options for amplxl are:\n"
"\n"
"2D\n"
"    Keyword to to specify that data indexed over two sets will be represented in\n"
"    a two-dimensional table, with keys from one set labeling the rows, and keys\n"
"    from the other set labeling the columns.\n"
"\n"
"    Example:\n"
"        table foo IN \"amplxl\" \"2D\": [keycol1, keycol2], valcol;\n"
"\n"
"alias:\n"
"    Instead of writing the data to a specific .xlsx file it is possible to\n"
"    define an alias. In the following example the table handler will search for\n"
"    the file bar.xlsx to write the data. If the file does not exist it will be\n"
"    created.\n"
"\n"
"    Example:\n"
"        table foo OUT \"amplxl\" \"bar\": [A], B;\n"
"\n"
"backup=option\n"
"    Whether or not to backup the existing .xlsx file before writing to it.\n"
"    The backup will create a file with the same name as the provided .xlsx file\n"
"    but with an .amplback extension. To use the backup file rename the .amplback\n"
"    extension to .xlsx.\n"
"    Options: true (default), false.\n"
"\n"
"    Example:\n"
"        table foo OUT \"amplxl\" \"backup=false\": [keycol1, keycol2], valcol;\n"
"\n"
"external-table-spec:\n"
"    Specifies the path to the .xlsx file to be read or written with the read \n"
"    table and write table commands. If no file is specified, amplxl will search\n"
"    for a file with the table name and the .xlsx file extension in the current\n"
"    directory. If the table is to be written and the file does not exist it will\n"
"    be created.\n"
"\n"
"    Example:\n"
"        table foo OUT \"amplxl\" \"bar.xlsx\": [keycol], valcol;\n"
"\n"
"verbose:\n"
"    Display warnings during the execution of the read table and write table\n"
"    commands.\n"
"\n"
"    Example:\n"
"        table foo OUT \"amplxl\" \"verbose\": [keycol], valcol;\n"
"\n"
"verbose=option:\n"
"    Display information according to the specified option. Available options:\n"
"        0 (default) - display information only on error,\n"
"        1 - display warnings,\n"
"        2 - display general information\n"
"        3 - display debug information.\n"
"\n"
"    Example:\n"
"        table foo OUT \"amplxl\" \"verbose=2\": [keycol], valcol;\n"
"\n"
"write=option\n"
"    Define how the data is written in OUT mode. Available options:\n"
"        delete (default) - deletes all the rows of the current table (if it\n"
"            exists) before writing the data from AMPL.\n"
"        append - append the rows in AMPL to the external representation of the\n"
"            table.\n"
;