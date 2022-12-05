.. _eodbc:

eodbc - Experimental ODBC Driver
========================

.. note::
    A beta test version of this new feature is now available.  
    Please send suggestions and error reports to beta@ampl.com.  

This page describes eodbc, an experimental AMPL interface for `ODBC <https://en.wikipedia.org/wiki/Open_Database_Connectivity>`_ compliant programs. It uses the same table statements as AMPL’s other data interfaces and expects a relational table representation of the data.

The new features of the interface include:

* Improved write speed
* Table deletion support * - use column types previously defined
* Table update support * - query from large tables and update only the necessary information

(*) must be supported by the database

If you are using spreadsheet software, you may also be interested in :doc:`amplxl` or :doc:`amplcsv` for .xlsx and .csv format files.

Installation
------------

The executable `eodbc.dll` is included by default in most of the generated bundles. If you dont't have eodbc yet
use one of the following links to download the table handler zipfile appropriate to your computer:

* Windows: `eodbc.win64.zip (64-bit) <https://ampl.com/dl/nfbvs/experimental/eodbc-0.0.2/eodbc.win64.zip>`_
* Linux: `eodbc.linux64-unixodbc.zip (64-bit) <https://ampl.com/dl/nfbvs/experimental/eodbc-0.0.2/eodbc.linux64-unixodbc.zip>`_, `eodbc.linux64-iodbc.zip (64-bit) <https://ampl.com/dl/nfbvs/experimental/eodbc-0.0.2/eodbc.linux64-iodbc.zip>`_
* macOS: `eodbc.macos-unixodbc.zip <https://ampl.com/dl/nfbvs/experimental/eodbc-0.0.2/eodbc.macos-unixodbc.zip>`_, `eodbc.macos-iodbc.zip <https://ampl.com/dl/nfbvs/experimental/eodbc-0.0.2/eodbc.macos-iodbc.zip>`_

Double-click the zipfile or use an unzip utility to extract the file `eodbc.dll`. Then move `eodbc.dll` into the same Windows/macOS folder or Linux directory as your AMPL program file. (The AMPL program file is `ampl.exe` on Windows systems, and `ampl` on Linux and macOS systems.)

.. note::

	* To connect with a given database you will need to install the corresponding ODBC driver and provide a connection string. In the usage examples bellow you will find multiple connection strings for popular databases.
	* In Linux and macOS systems you will also need to install an ODBC driver manager. Tipicaly unixODBC or iODBC.


Example
------------
To confirm that your installation is working, download `eodbc-test.zip <https://ampl.com/dl/eodbc/eodbc-test.zip>`_. Double-click the zipfile or use an unzip utility to extract the contents into an apropriate folder.

* If you are using command-line AMPL, move the 5 files into the folder (or Linux directory) where you have put `eodbc.dll`. Then start AMPL from that folder.
* If you are using the AMPL IDE, move the 5 files into any convenient folder (or Linux directory). Start the AMPL IDE, and use the IDE’s file pane (at the left) to make that folder current.


Learning more
-------------
The AMPL book’s chapter 10 `Database Access <https://ampl.com/BOOK/CHAPTERS/13-tables.pdf>`_ introduces the use of table statements for data transfer. Although the presentation is not specific to ODBC connections, the examples in that chapter can be adapted to work with the new experimental ODBC table handler. Thus we recommend reading at least sections 10.1 though 10.4 if you have not used any AMPL data table interface previously.

The eodbc table handler recognizes the following option strings when they are included in AMPL table statements. (After `load eodbc.dll;` has been executed, you can also display this listing by use of the AMPL command `print _handler_desc["eodbc"];`.)

.. code-block:: none

    EODBC: experimental ODBC driver for AMPL.

    Main differences from previous ODBC driver:
    - Autocommit is off by default, leading to faster write times.
    - Table columns must contain numeric or character data. Columns with both types
      are not supported.
    - No numerical conversion from/to timestamp columns. Data from the mentioned
      type will be loaded as character data.
    - In OUT mode, by default, tables will be deleted rather than dropped.
    - INOUT mode will use an SQL update statement.
    - If a table created by AMPL has key columns they will be declared as primary
      keys.
    - Files to load the data from must be declared in the DBQ option of the
      connection string.
    - Explicit loading of the library with the command "load eodbc.dll;" is needed.

    General information on table handlers and data correspondence between AMPL and
    an external table is available at:

        https://ampl.com/BOOK/CHAPTERS/13-tables.pdf

    The available options for eodbc are (cs denotes the connectionstring for the
    data provider in use):

    alias:
        Instead of using the string after the table keyword to define the table name
        to read/write/update the data from/to it is possible to define an alias.
        This is particularly useful when you need multiple declarations to
        read/write/update data from/to the same table.
        When writing data, if the table does not exist, it will be created.

        Example:
            table tablename OUT "eodbc" (cs) "tablealias": [A], B;

    autocommit=option:
        Whether or not to interpret every database operation as a transaction.
        Options: true, false (default).

        Example:
            table tablename OUT "eodbc" (cs) "autocommit=false": [A], B;

    connectionstring:
        An explicit ODBC connection string of the form "DSN=..." or "DRIVER=...".
        Additional fields depend on the data provider.

        Example:
            param connectionstring symbolic := "DRIVER=...;DATABASE=...;USER=...;";
            table tablename IN "eodbc" (connectionstring): [A], B;

    SQL=statement:
        (IN only) Provide a particular SQL statement to read data into AMPL.

        Example:
            table tablename IN "eodbc" (cs) "SQL=SELECT * FROM sometable;": [A], B;

    verbose:
        Display warnings during the execution of the read table and
        write table commands.

        Example:
            table tablename OUT "eodbc" "verbose": [keycol], valcol;

    verbose=option:
        Display information according to the specified option. Available
        options:
            0 (default) - display information only on error,
            1 - display warnings,
            2 - display general information
            3 - display debug information.

        Example:
            table tablename OUT "eodbc" (cs) "verbose=2": [keycol], valcol;

    write=option
        Define how the data is written in OUT mode. Available options:
            delete (default) - delete the rows in the external table before
                writing the data from AMPL.
            drop - drop the current table and create a new one before writing the
                data. The new table will only have double and varchar columns,
                depending on the original data from AMPL and the types available in
                the database.
            append - append the rows in AMPL to the external representation of the
                table.

        Example:
            table tablename OUT "eodbc" (cs) "write=append": [keycol], valcol;
