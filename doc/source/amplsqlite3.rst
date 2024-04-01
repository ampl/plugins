.. _amplsqlite3:

amplsqlite3 - AMPL Driver for the SQLite3 database
==================================================

.. note::
    A beta test version of this new feature is now available.
    Please send suggestions and error reports to beta@ampl.com.

This page describes amplsqlite3, an experimental AMPL interface for `SQLite3 <https://www.sqlite.org>`_. It uses the same table statements as AMPL’s other data interfaces and expects a relational table representation of the data.

The amplsqlite3 driver includes it's own SQLite3 embedded database engine(version 3.45.1) so you don't need to install any additional software. You can use amplsqlite3 concurrently, with your local install of SQLite3 or just save the results in a .db file to be exported to another machine for the final data analysis.

If you are using spreadsheet software, you may be interested in :doc:`amplxl` or :doc:`amplcsv` for .xlsx and .csv format files.
For other `ODBC <https://en.wikipedia.org/wiki/Open_Database_Connectivity>`_ compliant databases consult :doc:`eodbc`.


Installation
------------

The executable `amplsqlite3.dll` is included by default in most of the distributed bundles.
If you dont't have amplsqlite3 yet go to the `AMPL Portal website <https://portal.ampl.com>`_, select
**My Downloads**, scroll down to **Individual modules** and download the **plugins** module compatible with you platform.

Double-click the zipfile or use an unzip utility to extract the file `amplsqlite3.dll`. Then move `amplsqlite3.dll` into the same Windows/macOS folder or Linux directory as your AMPL program file. (The AMPL program file is `ampl.exe` on Windows systems, and `ampl` on Linux and macOS systems.)

Example
-------
To confirm that your installation is working, download `amplsqlite3-diet.zip <https://portal.ampl.com/~nfbvs/samples/amplsqlite3/amplsqlite3-diet.zip>`_. Double-click the zipfile or use an unzip utility to extract the contents into an appropriate folder. Consider this as your test folder.

* If you are using command-line AMPL, start your AMPL session and use the `cd` command to move to your test folder.
* If you are using the AMPL IDE, start the AMPL IDE, and use the IDE’s file pane (at the left) to make your test folder current.

The test folder contains multiple `.run` files that start with the instruction

.. code-block:: ampl

    load amplsqlite3.dll;

to make the `amplsqlite3` table handler available.


Write example
*************

In the "diet_write.run" example we will load the `diet` model and data files and save the data to a database file named "diet.db". The same process may also be used to convert existing '.dat' files into database tables.

The `diet` problem has two indexing sets, **NUTR** and **FOOD**, and several parameters.
The parameters **n_min** and **n_max** are index by **NUTR** so it's natural to create a table named **nutr** to store the information.

.. code-block:: ampl

    table nutr OUT "amplsqlite3" "diet.db":
        NUTR -> [nutr], n_min, n_max;

The same reasoning may be aplyed to the **FOOD** set and the **cost**, **f_min** and **f_max** parameters.

.. code-block:: ampl

    table food OUT "amplsqlite3" "diet.db":
        FOOD -> [food], cost, f_min, f_max;

Finally **amt** is indexed simultaneously by **NUTR** and **FOOD**.

.. code-block:: ampl

    table amt OUT "amplsqlite3" "diet.db":
        [nutr, food], amt;

Note the **OUT** keyword in the table statements and the brackets around the indexing sets. The **->** arrow indicates that the members of our indexing set will be written in the key column.
After the tables are defined we need to invoke a `write` statement for each of the declared tables.

.. code-block:: ampl

    write table nutr;
    write table food;
    write table amt;

The driver will search for a table with the given name, delete the data in the table and write the data from AMPL.
If the table does not exist it will be created.

Read example
************

In the "diet_read.run" example we will load the `diet` model, read the data from the database and call a solver.
We first need to specify the table declarations. They are similar to the write example.

.. code-block:: ampl

    table nutr IN "amplsqlite3" "diet.db":
        NUTR <- [nutr], n_min, n_max;

    table food IN "amplsqlite3" "diet.db":
        FOOD <- [food], cost, f_min, f_max;

    table amt IN "amplsqlite3" "diet.db":
        [nutr, food], amt;

Note the **IN** keyword in the table statements and the brackets around the indexing sets. Also note the **<-** arrow indication 
that the data for the indexing sets will be read from the table.
After the table declaration we load the data with the `read table` statements

.. code-block:: ampl

    read table nutr;
    read table food;
    read table amt;

and invoke a solver to find a solution for our `diet` problem.

Update example
**************

In the *"diet_update.run"* example we will load the `diet` model, load the data from a database, change some values in the *nutr*, *food* and *amt* tables and update the tables with these new values. The table declarations are similar to the previous examples

.. code-block:: ampl

    table nutr INOUT "amplsqlite3" "diet.db":
        NUTR <-> [nutr], n_min, n_max;

    table food INOUT "amplsqlite3" "diet.db":
        FOOD <-> [food], cost, f_min, f_max;

    table amt INOUT "amplsqlite3" "diet.db":
        [nutr, food], amt;

In this example we are using a single table declaration to read and update the data.
The **<->** arrow indicates that the indexing sets will be populated, when using a `read table` instruction.
Conversely the members of the indexing sets will be written to the corresponding table, when a `write table` statement is used.
The **INOUT** keyword will trigger an UPDATE statement in the database.

After the table declarations we have the `read table` instructions, we update some values in the parameters with the `let` command and we update the values in the database with the `write table` commands.

Note that if you run the *"diet_read.run"* example afterwards AMPL will display the updated values.



Learning more
-------------
The AMPL book’s chapter 10 `Database Access <https://ampl.com/BOOK/CHAPTERS/13-tables.pdf>`_ introduces the use of table statements for data transfer. Although the presentation is not specific to the amplsqlite3 driver, the examples in that chapter can be adapted to work with the new experimental amplsqlite3 table handler. Thus we recommend reading at least sections 10.1 though 10.4 if you have not used any AMPL data table interface previously.

The amplsqlite3 table handler recognizes the following option strings when they are included in AMPL table statements. (After `load amplsqlite3.dll;` has been executed, you can also display this listing by use of the AMPL command `print _handler_desc["amplsqlite3"];`.)

.. code-block:: none

    A table handler for sqlite3 databases.

    General information on table handlers and data correspondence between AMPL and 
    an external table is available at chapter 10 of the AMPL book:

        https://ampl.com/learn/ampl-book/

    Information on sqlite3 is available at

        https://www.sqlite.org

    The available options for amplsqlite3 are:

    alias:
        Instead of writing the data to the table with the name defined in the table
        declaration it's possible define the table name with an alias. In the
        following example the table handler will search for a table named "bar"
        instead of a table named "foo" as in the table declaration.

        Example:
            table foo OUT "amplsqlite3" "bar": [A], B;

    external-table-spec:
        Specifies the path to the .db file to be read or written with the read 
        table and write table commands. If no file is specified, amplsqlite3 will
        search for a file with the table name and the .db file extension in the
        current directory. If the table is to be written and the file does not exist
        it will be created.

        Example:
            table foo OUT "amplsqlite3" "bar.db": [keycol], valcol;

    verbose:
        Display warnings during the execution of the read table and write table
        commands.

        Example:
            table foo OUT "amplsqlite3" "verbose": [keycol], valcol;

    verbose=option:
        Display information according to the specified option. Available options:
            0 (default) - display information only on error,
            1 - display warnings,
            2 - display general information
            3 - display debug information.

        Example:
            table foo OUT "amplsqlite3" "verbose=2": [keycol], valcol;

    write=option
        Define how the data is written in OUT mode. Available options:
            delete (default) - deletes all the rows of the corresponding table (if
            it exists) before writing the data from AMPL.
            drop - drops the corresponding table and creates a new one.

        Example:
            table foo OUT "amplsqlite3" "write=drop": [keycol], valcol;
