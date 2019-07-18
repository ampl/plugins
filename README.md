AMPL_XL - AMPL table handler to manage Microsoft Excel .xlsx and .xlsm files.


LIMITATIONS:
- currently, in windows platform, the temporary folder to extract files needs to be created manualy. In the same folder of the binary add a folder named "ampltemp".
- no cmake provided, build with the python script invoking "python build.py", it will create an executable named "ampl_xl.dll" in the /src folder. 


TODO:
- manage cmake
- finish multiplatform temp folder management
- improve column search (if a column name is not correct its taking a long time to give the error)
- pass AE and TI to the zip library to return error codes from the zip process itself
- document code and table handler search process


The main directory has 3 folders:
    - solvers2: provides headers to link to AMPL,
    - src: the actual source code, pugi_xml and minizip included, for simplicity,
    - zlib-1.2.11: library to extract and compress the zip archives.

Generic information about ooxml files is available in 
https://docs.microsoft.com/en-us/dotnet/api/documentformat.openxml.spreadsheet?view=openxml-2.8.1
Microsoft Excel .xlsx and .xlsm files are basicaly ziped files with the following inner structure:

    + docProps
    + _rels
    + xl
    - [Content_Types].xml

The information we need is located in the xl folder, which has the following structure:

    + _rels
    + theme
    + worksheets
    - sharedStrings.xml
    - styles.xml
    - workbook.xml

Information of a given table tipicaly comes in a definedName, so to access the information we need to:

- open workbook.xml and see if the defined name exists with the name of the table and get the corresponding range
- workbook.xml also has the relation information of the excel sheets, "r:id", we need to store the information 
- parse the obtained excel range and obtain the name of the sheet and the excel range








