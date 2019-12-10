#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <ctime>


#include "pugixml.hpp"
#include "zip.h"
#include "unzip.h"

#include "copyzip.hpp"
#include "myzip.hpp"
#include "myunz.hpp"



/*
templates for a blanc oxml file
Required files:

[Content_Types].xml
xl/_rels/workbook.xml.rels
xl/worksheets/sheet1.xml
xl/workbook.xml
xl/styles.xml
_rels/.rels
docProps/app.xml
docProps/core.xml
*/

const std::string content_types_xml =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\"><Override PartName=\"/xl/_rels/workbook.xml.rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/><Override PartName=\"/xl/worksheets/sheet1.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml\"/><Override PartName=\"/xl/workbook.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml\"/><Override PartName=\"/xl/styles.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.styles+xml\"/><Override PartName=\"/_rels/.rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/><Override PartName=\"/docProps/app.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.extended-properties+xml\"/><Override PartName=\"/docProps/core.xml\" ContentType=\"application/vnd.openxmlformats-package.core-properties+xml\"/>\n"
"</Types>\n";

const std::string xl_rels_workbook_xml_rels =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\"><Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles\" Target=\"styles.xml\"/><Relationship Id=\"rId2\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet\" Target=\"worksheets/sheet1.xml\"/>\n"
"</Relationships>\n";

const std::string xl_worksheets_sheet1_xml =
"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
"<worksheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\" xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\"><sheetPr filterMode=\"false\"><pageSetUpPr fitToPage=\"false\"/></sheetPr><dimension ref=\"A1\"/><sheetViews><sheetView showFormulas=\"false\" showGridLines=\"true\" showRowColHeaders=\"true\" showZeros=\"true\" rightToLeft=\"false\" tabSelected=\"true\" showOutlineSymbols=\"true\" defaultGridColor=\"true\" view=\"normal\" topLeftCell=\"A1\" colorId=\"64\" zoomScale=\"100\" zoomScaleNormal=\"100\" zoomScalePageLayoutView=\"100\" workbookViewId=\"0\"><selection pane=\"topLeft\" activeCell=\"A1\" activeCellId=\"0\" sqref=\"A1\"/></sheetView></sheetViews><sheetFormatPr defaultRowHeight=\"12.8\" zeroHeight=\"false\" outlineLevelRow=\"0\" outlineLevelCol=\"0\"></sheetFormatPr><cols><col collapsed=\"false\" customWidth=\"false\" hidden=\"false\" outlineLevel=\"0\" max=\"1025\" min=\"1\" style=\"0\" width=\"11.52\"/></cols><sheetData/><printOptions headings=\"false\" gridLines=\"false\" gridLinesSet=\"true\" horizontalCentered=\"false\" verticalCentered=\"false\"/><pageMargins left=\"0.7875\" right=\"0.7875\" top=\"1.05277777777778\" bottom=\"1.05277777777778\" header=\"0.7875\" footer=\"0.7875\"/><pageSetup paperSize=\"1\" scale=\"100\" firstPageNumber=\"1\" fitToWidth=\"1\" fitToHeight=\"1\" pageOrder=\"downThenOver\" orientation=\"portrait\" blackAndWhite=\"false\" draft=\"false\" cellComments=\"none\" useFirstPageNumber=\"true\" horizontalDpi=\"300\" verticalDpi=\"300\" copies=\"1\"/><headerFooter differentFirst=\"false\" differentOddEven=\"false\"><oddHeader>&amp;C&amp;&quot;Times New Roman,Regular&quot;&amp;12&amp;A</oddHeader><oddFooter>&amp;C&amp;&quot;Times New Roman,Regular&quot;&amp;12Page &amp;P</oddFooter></headerFooter></worksheet>\n";

const std::string xl_workbook_xml =
"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
"<workbook xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\" xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\"><fileVersion appName=\"Calc\"/><workbookPr backupFile=\"false\" showObjects=\"all\" date1904=\"false\"/><workbookProtection/><bookViews><workbookView showHorizontalScroll=\"true\" showVerticalScroll=\"true\" showSheetTabs=\"true\" xWindow=\"0\" yWindow=\"0\" windowWidth=\"16384\" windowHeight=\"8192\" tabRatio=\"500\" firstSheet=\"0\" activeTab=\"0\"/></bookViews><sheets><sheet name=\"Sheet1\" sheetId=\"1\" state=\"visible\" r:id=\"rId2\"/></sheets><calcPr iterateCount=\"100\" refMode=\"A1\" iterate=\"false\" iterateDelta=\"0.001\"/><extLst><ext xmlns:loext=\"http://schemas.libreoffice.org/\" uri=\"{7626C862-2A13-11E5-B345-FEFF819CDC9F}\"><loext:extCalcPr stringRefSyntax=\"CalcA1\"/></ext></extLst></workbook>\n";

const std::string xl_styles_xml =
"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
"<styleSheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\"><numFmts count=\"1\"><numFmt numFmtId=\"164\" formatCode=\"General\"/></numFmts><fonts count=\"4\"><font><sz val=\"10\"/><name val=\"Arial\"/><family val=\"2\"/></font><font><sz val=\"10\"/><name val=\"Arial\"/><family val=\"0\"/></font><font><sz val=\"10\"/><name val=\"Arial\"/><family val=\"0\"/></font><font><sz val=\"10\"/><name val=\"Arial\"/><family val=\"0\"/></font></fonts><fills count=\"2\"><fill><patternFill patternType=\"none\"/></fill><fill><patternFill patternType=\"gray125\"/></fill></fills><borders count=\"1\"><border diagonalUp=\"false\" diagonalDown=\"false\"><left/><right/><top/><bottom/><diagonal/></border></borders><cellStyleXfs count=\"20\"><xf numFmtId=\"164\" fontId=\"0\" fillId=\"0\" borderId=\"0\" applyFont=\"true\" applyBorder=\"true\" applyAlignment=\"true\" applyProtection=\"true\"><alignment horizontal=\"general\" vertical=\"bottom\" textRotation=\"0\" wrapText=\"false\" indent=\"0\" shrinkToFit=\"false\"/><protection locked=\"true\" hidden=\"false\"/></xf><xf numFmtId=\"0\" fontId=\"1\" fillId=\"0\" borderId=\"0\" applyFont=\"true\" applyBorder=\"false\" applyAlignment=\"false\" applyProtection=\"false\"></xf><xf numFmtId=\"0\" fontId=\"1\" fillId=\"0\" borderId=\"0\" applyFont=\"true\" applyBorder=\"false\" applyAlignment=\"false\" applyProtection=\"false\"></xf><xf numFmtId=\"0\" fontId=\"2\" fillId=\"0\" borderId=\"0\" applyFont=\"true\" applyBorder=\"false\" applyAlignment=\"false\" applyProtection=\"false\"></xf><xf numFmtId=\"0\" fontId=\"2\" fillId=\"0\" borderId=\"0\" applyFont=\"true\" applyBorder=\"false\" applyAlignment=\"false\" applyProtection=\"false\"></xf><xf numFmtId=\"0\" fontId=\"0\" fillId=\"0\" borderId=\"0\" applyFont=\"true\" applyBorder=\"false\" applyAlignment=\"false\" applyProtection=\"false\"></xf><xf numFmtId=\"0\" fontId=\"0\" fillId=\"0\" borderId=\"0\" applyFont=\"true\" applyBorder=\"false\" applyAlignment=\"false\" applyProtection=\"false\"></xf><xf numFmtId=\"0\" fontId=\"0\" fillId=\"0\" borderId=\"0\" applyFont=\"true\" applyBorder=\"false\" applyAlignment=\"false\" applyProtection=\"false\"></xf><xf numFmtId=\"0\" fontId=\"0\" fillId=\"0\" borderId=\"0\" applyFont=\"true\" applyBorder=\"false\" applyAlignment=\"false\" applyProtection=\"false\"></xf><xf numFmtId=\"0\" fontId=\"0\" fillId=\"0\" borderId=\"0\" applyFont=\"true\" applyBorder=\"false\" applyAlignment=\"false\" applyProtection=\"false\"></xf><xf numFmtId=\"0\" fontId=\"0\" fillId=\"0\" borderId=\"0\" applyFont=\"true\" applyBorder=\"false\" applyAlignment=\"false\" applyProtection=\"false\"></xf><xf numFmtId=\"0\" fontId=\"0\" fillId=\"0\" borderId=\"0\" applyFont=\"true\" applyBorder=\"false\" applyAlignment=\"false\" applyProtection=\"false\"></xf><xf numFmtId=\"0\" fontId=\"0\" fillId=\"0\" borderId=\"0\" applyFont=\"true\" applyBorder=\"false\" applyAlignment=\"false\" applyProtection=\"false\"></xf><xf numFmtId=\"0\" fontId=\"0\" fillId=\"0\" borderId=\"0\" applyFont=\"true\" applyBorder=\"false\" applyAlignment=\"false\" applyProtection=\"false\"></xf><xf numFmtId=\"0\" fontId=\"0\" fillId=\"0\" borderId=\"0\" applyFont=\"true\" applyBorder=\"false\" applyAlignment=\"false\" applyProtection=\"false\"></xf><xf numFmtId=\"43\" fontId=\"1\" fillId=\"0\" borderId=\"0\" applyFont=\"true\" applyBorder=\"false\" applyAlignment=\"false\" applyProtection=\"false\"></xf><xf numFmtId=\"41\" fontId=\"1\" fillId=\"0\" borderId=\"0\" applyFont=\"true\" applyBorder=\"false\" applyAlignment=\"false\" applyProtection=\"false\"></xf><xf numFmtId=\"44\" fontId=\"1\" fillId=\"0\" borderId=\"0\" applyFont=\"true\" applyBorder=\"false\" applyAlignment=\"false\" applyProtection=\"false\"></xf><xf numFmtId=\"42\" fontId=\"1\" fillId=\"0\" borderId=\"0\" applyFont=\"true\" applyBorder=\"false\" applyAlignment=\"false\" applyProtection=\"false\"></xf><xf numFmtId=\"9\" fontId=\"1\" fillId=\"0\" borderId=\"0\" applyFont=\"true\" applyBorder=\"false\" applyAlignment=\"false\" applyProtection=\"false\"></xf></cellStyleXfs><cellXfs count=\"1\"><xf numFmtId=\"164\" fontId=\"0\" fillId=\"0\" borderId=\"0\" xfId=\"0\" applyFont=\"false\" applyBorder=\"false\" applyAlignment=\"false\" applyProtection=\"false\"><alignment horizontal=\"general\" vertical=\"bottom\" textRotation=\"0\" wrapText=\"false\" indent=\"0\" shrinkToFit=\"false\"/><protection locked=\"true\" hidden=\"false\"/></xf></cellXfs><cellStyles count=\"6\"><cellStyle name=\"Normal\" xfId=\"0\" builtinId=\"0\" customBuiltin=\"false\"/><cellStyle name=\"Comma\" xfId=\"15\" builtinId=\"3\" customBuiltin=\"false\"/><cellStyle name=\"Comma [0]\" xfId=\"16\" builtinId=\"6\" customBuiltin=\"false\"/><cellStyle name=\"Currency\" xfId=\"17\" builtinId=\"4\" customBuiltin=\"false\"/><cellStyle name=\"Currency [0]\" xfId=\"18\" builtinId=\"7\" customBuiltin=\"false\"/><cellStyle name=\"Percent\" xfId=\"19\" builtinId=\"5\" customBuiltin=\"false\"/></cellStyles></styleSheet>\n";

const std::string rels_rels =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\"><Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument\" Target=\"xl/workbook.xml\"/><Relationship Id=\"rId2\" Type=\"http://schemas.openxmlformats.org/package/2006/relationships/metadata/core-properties\" Target=\"docProps/core.xml\"/><Relationship Id=\"rId3\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/extended-properties\" Target=\"docProps/app.xml\"/>\n"
"</Relationships>\n";

const std::string docProps_app_xml =
"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
"<Properties xmlns=\"http://schemas.openxmlformats.org/officeDocument/2006/extended-properties\" xmlns:vt=\"http://schemas.openxmlformats.org/officeDocument/2006/docPropsVTypes\"><Template></Template><TotalTime>0</TotalTime><Application>LibreOffice/6.0.7.3$Linux_X86_64 LibreOffice_project/00m0$Build-3</Application></Properties>\n";

const std::string docProps_core_xml =
"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
"<cp:coreProperties xmlns:cp=\"http://schemas.openxmlformats.org/package/2006/metadata/core-properties\" xmlns:dc=\"http://purl.org/dc/elements/1.1/\" xmlns:dcterms=\"http://purl.org/dc/terms/\" xmlns:dcmitype=\"http://purl.org/dc/dcmitype/\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"><dcterms:created xsi:type=\"dcterms:W3CDTF\">2019-10-09T11:21:35Z</dcterms:created><dc:creator></dc:creator><dc:description></dc:description><dc:language>en-US</dc:language><cp:lastModifiedBy></cp:lastModifiedBy><cp:revision>0</cp:revision><dc:subject></dc:subject><dc:title></dc:title></cp:coreProperties>\n";

/*
Template to add an additional sheet to an already existing workbook
*/
const std::string xl_worksheets_sheet2_xml =
"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
"<worksheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\" xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\"><sheetPr filterMode=\"false\"><pageSetUpPr fitToPage=\"false\"/></sheetPr><dimension ref=\"A1\"/><sheetViews><sheetView showFormulas=\"false\" showGridLines=\"true\" showRowColHeaders=\"true\" showZeros=\"true\" rightToLeft=\"false\" tabSelected=\"false\" showOutlineSymbols=\"true\" defaultGridColor=\"true\" view=\"normal\" topLeftCell=\"A1\" colorId=\"64\" zoomScale=\"100\" zoomScaleNormal=\"100\" zoomScalePageLayoutView=\"100\" workbookViewId=\"0\"><selection pane=\"topLeft\" activeCell=\"A1\" activeCellId=\"0\" sqref=\"A1\"/></sheetView></sheetViews><sheetFormatPr defaultRowHeight=\"12.8\" zeroHeight=\"false\" outlineLevelRow=\"0\" outlineLevelCol=\"0\"></sheetFormatPr><cols><col collapsed=\"false\" customWidth=\"false\" hidden=\"false\" outlineLevel=\"0\" max=\"1025\" min=\"1\" style=\"0\" width=\"11.52\"/></cols><sheetData/><printOptions headings=\"false\" gridLines=\"false\" gridLinesSet=\"true\" horizontalCentered=\"false\" verticalCentered=\"false\"/><pageMargins left=\"0.7875\" right=\"0.7875\" top=\"1.05277777777778\" bottom=\"1.05277777777778\" header=\"0.7875\" footer=\"0.7875\"/><pageSetup paperSize=\"1\" scale=\"100\" firstPageNumber=\"1\" fitToWidth=\"1\" fitToHeight=\"1\" pageOrder=\"downThenOver\" orientation=\"portrait\" blackAndWhite=\"false\" draft=\"false\" cellComments=\"none\" useFirstPageNumber=\"false\" horizontalDpi=\"300\" verticalDpi=\"300\" copies=\"1\"/><headerFooter differentFirst=\"false\" differentOddEven=\"false\"><oddHeader>&amp;C&amp;&quot;Times New Roman,Regular&quot;&amp;12&amp;A</oddHeader><oddFooter>&amp;C&amp;&quot;Times New Roman,Regular&quot;&amp;12Page &amp;P</oddFooter></headerFooter></worksheet>";

/*
Template to add the shared strings file to an existing workbook
*/
const std::string shared_strings_tplt =
"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
"<sst xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\" count=\"0\" uniqueCount=\"0\"></sst>";

// name of xml files in temp folder 
static const char* char_zip_orig[] = {
	"content_types_xml",
	"xl_rels_workbook_xml_rels",
	"xl_worksheets_sheet1_xml",
	"xl_styles_xml",
	"xl_workbook_xml",
	"rels_rels",
	"docProps_app_xml",
	"docProps_core_xml"
};

// name of the corresponding xml files in the zip
static const char* char_zip_dest[] = {
	"[Content_Types].xml",
	"xl/_rels/workbook.xml.rels",
	"xl/worksheets/sheet1.xml",
	"xl/styles.xml",
	"xl/workbook.xml",
	"_rels/.rels",
	"docProps/app.xml",
	"docProps/core.xml"
};

/*
Adds already existing files to a zip archive
Parameters:
	zip_file - name of the final zip archive
	temp_path - temporary folder that contains the files to zip
	zip_orig - name of the xml files in the temporary folder
	zip_dest - name of the xml files in the zip archive
Returns:
	0 success
	1 failure
*/
int
zip_xml_files(
	std::string & zip_file,
	std::string & temp_path,
	std::vector<std::string> & zip_orig,
	std::vector<std::string> & zip_dest
);

/*
Reuses an existing std::ofstream. The ofstream will open the file dest and stream the info in orig.
*/
int
reuse_ofstream(std::ofstream & ofs, std::string & orig, std::string & dest);

/*
Adds a new sheet to an existing oxml file.
Parameters:
	oxml_file - file to add the new sheet
	sheet_name - the name of the new sheet
	temp_folder - temporary folder to extract temporary files
Returns:
	0 success
	1 failure
*/
int
oxml_add_new_sheet(std::string & oxml_file, std::string & sheet_name);

/*
Adds a shared strings file to an existing oxml file.
Parameters:
	oxml_file - file to add the shared strings
	temp_folder - temporary folder to extract temporary files
Returns:
	0 success
	1 failure
*/
int
oxml_add_shared_strings(std::string & oxml_file);

/*
Adds a sheet with number new_sheet_number to xl/worksheets/ inside the oxml file.
Parameters:
	oxml_file - file to add the new sheet
	new_sheet_number - the number of the new sheet
	temp_folder - temporary folder to extract temporary files
Returns:
	0 success
	1 failure
*/
int
add_sheet(std::string & oxml_file, int new_sheet_number, std::string & temp_folder);

/*
Gets the number of the last sheet in an oxml file.
Parameters:
	oxml_file - file
	temp_folder - temporary folder to extract temporary files
Returns:
	number of the last sheet on success
	-1 failure
*/
int
get_last_sheet_number(std::string & oxml_file);

/*
Gets the number of the last relation in xl/_rels/workbook.xml.rels of an oxml file.
Parameters:
	oxml_file - file
	temp_folder - temporary folder to extract temporary files
Returns:
	number of the last sheet on success
	-1 failure
*/
int
get_last_relation_number(std::string & oxml_file);

/*
Builds an oxml file from the templates in a Libreoffice xlsx document.
Parameters:
	oxml_file - path of the file to create
	temp_folder - temporary folder to extract temporary files
Returns:
	0 success
	1 failure
*/
int
oxml_build_file(std::string & oxml_file);

/*
Updates the relations of an oxml file after adding a new sheet.
Parameters:
	oxml_file - file were the new sheet was added
	new_sheet_number - the number of the new sheet that was added
	new_rel_number - the number of the new relation of the new sheet
	sheet_name - the name of the new sheet
	temp_folder - temporary folder to extract temporary files
Returns:
	0 success
	1 failure
*/
int
add_sheet_update_relations(std::string & oxml_file, int new_sheet_number, int new_rel_number, std::string & sheet_name, std::string & temp_folder);

/*
Updates the content files of an oxml document.
Parameters:
	oxml_file - name of the file
	part_name - value of the partName attribute in the new Override node
	content_type - value of the contentType attribute in the new Override node
	temp_folder - temporary folder to extract temporary files
Returns:
	0 success
	1 failure
*/
int
update_content_types(std::string & oxml_file, std::string & part_name, std::string & content_type, std::string & temp_folder);

/*
Updates the xl/workbook of an oxml document after adding a new sheet.
Parameters:
	oxml_file - name of the file
	new_sheet_number - the number of the new sheet that was added
	new_rel_number - the number of the new relation of the new sheet
	sheet_name - the name of the new sheet
	temp_folder - temporary folder to extract temporary files
Returns:
	0 success
	1 failure
*/
int
add_sheet_update_xl_workbook(std::string & oxml_file, int new_sheet_number, int new_rel_number, std::string & sheet_name, std::string & temp_folder);

/*
Updates the xl/_rels/workbook of an oxml document after adding a new sheet.
Parameters:
	oxml_file - name of the file
	new_sheet_number - the number of the new sheet that was added
	new_rel_number - the number of the new relation of the new sheet
	temp_folder - temporary folder to extract temporary files
Returns:
	0 success
	1 failure
*/
int
add_sheet_update_xl_rels_workbook(std::string & oxml_file, int new_sheet_number, int new_rel_number, std::string & temp_folder);

/*
Adds a shared string xml to an existing oxml file.
Parameters:
	oxml_file - name of the file
	temp_folder - temporary folder to extract temporary files
Returns:
	0 success
	1 failure
*/
int
add_shared_strings_file(std::string & oxml_file, std::string & temp_folder);

/*
Updates the relations of an oxml file after adding a shared strings table.
Parameters:
	oxml_file - name of the file
	new_rel_number - number of the new relation to add
	temp_folder - temporary folder to extract temporary files
Returns:
	0 success
	1 failure
*/
int
add_shared_strings_update_relations(std::string & oxml_file, int new_rel_number, std::string & temp_folder);

/*
Updates xl/_rels/workbook after adding a new shared strings table.
Parameters:
	oxml_file - name of the file
	new_rel_number - number of the new relation to add
	temp_folder - temporary folder to extract temporary files
Returns:
	0 success
	1 failure
*/
int
add_shared_strings_update_xl_rels_workbook(std::string & oxml_file, int new_rel_number, std::string & temp_folder);

/*
Joins a path with a file name with os dependant separator. 
*/
void
join_path(const std::string & folder, const std::string & file, std::string & join);

/*
Gets the current date in gmtime and return a string with the date in the format required for an oxml file.
*/
std::string get_current_date();


std::string
get_current_date2();


/*
Updates the created date attribute of the docProps/core.xml file in an oxml document.
Parameters:
	current_date - string with the date we want to insert
	path - path to the file in the temporary folder were the date will be replaced
Returns:
	0 success
	1 failure
*/
int
update_date_created(std::string & current_date, std::string & path);

/*
Updates the modified date attribute of the docProps/core.xml file in an oxml document.
Parameters:
	current_date - string with the date we want to insert
	path - path to the file in the temporary folder were the date will be replaced
Returns:
	0 success
	1 failure
*/
int
update_date_modified(std::string & current_date, std::string & path);

/*
Verifies if a given oxml file has a shared strings table defined in its Content_Types
Parameters:
	oxml_file - name of the file
	temp_folder - temporary folder to extract temporary files
Returns:
	0 false
	1 true
	-1 error
*/
int
has_shared_strings(std::string & oxml_file);

/*
Check if a given filename exists.
*/
bool
check_file_exists(const std::string & filename);


void
my_copy_file(const std::string & source_path, const std::string & dest_path);



int
get_temp_folder(const std::string & base, std::string & temp_folder);

int
remove_temp_folder(const std::string & temp_folder);


