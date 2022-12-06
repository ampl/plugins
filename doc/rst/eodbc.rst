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

The executable `eodbc.dll` is included by default in most of the distributed bundles. If you dont't have eodbc yet
use one of the following links to download the table handler zipfile appropriate to your computer:

* Windows: `eodbc.win64.zip (64-bit) <https://portal.ampl.com/~nfbvs/eodbc/eodbc.win64.zip>`_
* Linux: `eodbc.linux64-unixodbc.zip (64-bit) <https://portal.ampl.com/~nfbvs/eodbc/eodbc.linux64-unixodbc.zip>`_, `eodbc.linux64-iodbc.zip (64-bit) <https://portal.ampl.com/~nfbvs/eodbc/eodbc.linux64-iodbc.zip>`_
* macOS: `eodbc.macos-unixodbc.zip <https://portal.ampl.com/~nfbvs/eodbc/eodbc.macos-unixodbc.zip>`_, `eodbc.macos-iodbc.zip <https://portal.ampl.com/~nfbvs/eodbc/eodbc.macos-iodbc.zip>`_

Double-click the zipfile or use an unzip utility to extract the file `eodbc.dll`. Then move `eodbc.dll` into the same Windows/macOS folder or Linux directory as your AMPL program file. (The AMPL program file is `ampl.exe` on Windows systems, and `ampl` on Linux and macOS systems.)

.. note::

	* To connect with a given database you will need to install the corresponding ODBC driver and provide a connection string. In the usage examples bellow you will find multiple connection strings for popular databases.
	* In Linux and macOS systems you will also need to install an ODBC driver manager. Tipicaly unixODBC or iODBC.


Example
-------
To confirm that your installation is working, download `eodbc-test.zip <https://portal.ampl.com/~nfbvs/eodbc/eodbc-test.zip>`_. Double-click the zipfile or use an unzip utility to extract the contents into an apropriate folder. Consider this as your test folder.

* If you are using command-line AMPL, start your AMPL session and use the `cd` command to move to your test folder.
* If you are using the AMPL IDE, start the AMPL IDE, and use the IDE’s file pane (at the left) to make your test folder current.

The test folder contains multiple `.run` files that start with the instruction

.. code-block:: ampl

	load eodbc.dll;

to make the `eodbc` table handler available.

The `.run` files also contain a **connection string** parameter. Choose the option for your database or consult the documentation of the database you are using in order to provide a correct connection string.

.. code-block:: ampl

	param cs symbolic := "my connection string";


Write example
*************

In the "diet_write.run" example we will load the `diet` model and data files and save the data to a database. The same process may also be used to convert existing '.dat' files into database tables.

The `diet` problem has two indexing sets, **NUTR** and **FOOD**, and several parameters. 
The parameters **n_min** and **n_max** are index by **NUTR** so it's natural to create a table named **nutr** to store the information.

.. code-block:: ampl

	table nutr OUT "eodbc" (cs):
		NUTR -> [nutr], n_min, n_max;

The same reasoning may be aplyed to the **FOOD** set and the **cost**, **f_min** and **f_max** parameters.

.. code-block:: ampl

	table food OUT "eodbc" (cs):
		FOOD -> [food], cost, f_min, f_max;

Finally **amt** is indexed simultaneously by **NUTR** and **FOOD**.

.. code-block:: ampl

	table amt OUT "eodbc" (cs):
		[nutr, food], amt;

Note the **OUT** keyword in the table statements and the brackets around the indexing sets. The -> arrow indicates that the members of our indexing set will be written in the key column.
After the tables are defined we need to invoke a `write` statement for each of the declared tables.

.. code-block:: ampl

	write table nutr;
	write table food;
	write table amt;

The driver will search for a table with the given name, delete the data in the table and write the data from AMPL.
If the table does not exist it will be cretaed.

Read example
************

In the "diet_read.run" example we will load the `diet` model, read the data from the database and call a solver.
We first need to specify the table declarations. They are similar to the write example.

.. code-block:: ampl

	table nutr IN "eodbc" (cs):
		NUTR <- [nutr], n_min, n_max;

	table food IN "eodbc" (cs):
		FOOD <- [food], cost, f_min, f_max;

	table amt IN "eodbc" (cs):
		[nutr, food], amt;

Note the **IN** keyword in the table statements and the brackets around the indexing sets. Also note the <- arrow indication 
that the data for the indexing sets will be read from the table.
After the table declaration we load the data with the `read table` statements

.. code-block:: ampl

	read table nutr;
	read table food;
	read table amt;

and invoke a solver to find a solution for our `diet` problem.

Update example
**************

In the *"diet_update.run"* example we will load the `diet` model, load the data from a database, change some values in the *nutr* and *food* tables and update the tables with these new values. The table declarations are similar to the previous examples

.. code-block:: ampl

	table nutr INOUT "eodbc" (cs):
		NUTR <-> [nutr], n_min, n_max;

	table food INOUT "eodbc" (cs):
		FOOD <-> [food], cost, f_min, f_max;

In this example we are using a single table declaration to read and update the data.
The <-> arrow indicates that the indexing sets will be populated, when using a `read table` instruction.
Conversely the members of the indexing sets will be written to the correponding table, when a `write table` statement is used.
The **INOUT** keyword will trigger an UPDATE statement in the database.

After the table declarations we have the `read table` instructions, we update some values in the parameters with the `let` command and we update the values in the database with the `write table` commands.

Note that if you run the *"diet_read.run"* example afterwards AMPL will display the updated values.



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
