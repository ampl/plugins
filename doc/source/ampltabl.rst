.. _ampltabl:

AMPL Table Handlers for Relational Databases
============================================

The structure of indexed data in AMPL has much in common with the structure of the relational tables widely used in data applications. The AMPL `table` declaration lets you take advantage of this similarity to define explicit connections between sets, parameters, variables, and expressions in AMPL, and relational tables maintained by other software. AMPL’s `read table` and `write table` commands subsequently use these connections to import data values into AMPL and to export data and solution values from AMPL.

.. note::
	A tutorial introduction to using all of the features of `table`, `read table` and `write table` is provided by `chapter 10 <https://ampl.com/BOOK/CHAPTERS/13-tables.pdf>`_ of the `AMPL book <https://ampl.com/resources/the-ampl-book/>`_.

The relational tables read and written by AMPL reside in files or databases whose names and locations you specify as part of the `table` declaration. To work with these files, AMPL relies on database handlers, which are add-ons that can be loaded as needed.

To use AMPL’s features for relational table access, you must install a handler that is designed for your specific computer platform. The sections below provide links to database handlers for the three most popular platforms:

* `Microsoft Windows`_
* `Linux`_ (all popular distributions)
* `macOS`_

These handlers are based on the ODBC standard and have been tested with widely used database systems as indicated below. In the Windows section, we also describe an alternative for permitting data exchanges between 64-bit AMPL and 32-bit Microsoft Office applications. For database access on remote computers, see our notes and download instructions for the :doc:`tableproxy`.

On Microsoft Windows systems, Excel spreadsheet data can also be treated as database tables. But for spreadsheets we recommend using `table` statements in conjunction with our :doc:`amplxl` instead.

Microsoft Windows
-----------------

The Microsoft Windows ODBC table handler is supplied as part of the standard AMPL software distribution. The relevant files in the distribution are as follows:

* 32-bit

  * ampltabl.dll, 32-bit ODBC table handler
  * tableproxy64.exe, connector for 64-bit applications (see below)

* 64-bit

  * ampltabl_64.dll, 64-bit ODBC table handler
  * ampltabl.dll, 32-bit ODBC table handler
  * tableproxy32.exe, connector for 32-bit applications (see below)

These files should be placed in the same folder as `ampl.exe` and the other files of of the AMPL distribution. The table handler can then be used with a variety of data applications as follows.

Microsoft Office
^^^^^^^^^^^^^^^^

ODBC drivers for Microsoft Excel and Microsoft Access come installed with Windows. Thus no further installation steps are needed to use `table`, `read table`, and `write table` in 32-bit AMPL with 32-bit Microsoft Office applications, or in 64-bit AMPL with 64-bit Microsoft Office applications. Simply follow the directions in `chapter 10 <https://ampl.com/BOOK/CHAPTERS/13-tables.pdf>`_ of the AMPL book.

To use 64-bit AMPL with 32-bit Office applications, be sure that all of the above 3 files from the 64-bit AMPL distribution are in your AMPL folder. Then follow the instructions in `chapter 10 <https://ampl.com/BOOK/CHAPTERS/13-tables.pdf>`_ except that wherever `"ODBC"` appears in a `table` statement, subsitute `"tableproxy" "odbc"`. For example:

.. code-block:: dummy

    table Foods IN "tableproxy" "odbc" "diet.xls": ...

To use 32-bit AMPL with 64-bit Office applications, it is possible to follow the same directions, after copying `ampltabl_64.dll` from the 64-bit distribution into the AMPL folder containing all of the files from the 32-bit distribution. However we recommend instead downloading the 64-bit AMPL distribution if you have access to it; in particular all purchases and trials of AMPL include the 64-bit version.

MySQL, Oracle and Microsoft SQL Server
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Go to one of our pages on :doc:`mysql`, :doc:`oracle` or :doc:`sqlserver` and follow the instructions for Windows in the Installation section. Then see the Usage section for instructions on specifying the necessary connection strings in the `table` statement; except for these adjustments, the `table` statement works as described for Microsoft Office applications in `chapter 10 <https://ampl.com/BOOK/CHAPTERS/13-tables.pdf>`_. On the same page you may also want to consult the sections on embedding general SQL statements and on troubleshooting.

Other database applications
^^^^^^^^^^^^^^^^^^^^^^^^^^^

Additional instruction pages are in preparation. For now, contact us at support@ampl.com to get connection string instructions for other database applications.

Linux
-----

Obtain a Linux ODBC table handler from one of the following links:

* 32-bit for use with 32-bit AMPL:

  * `ampltabl.linux32.odbc.1.zip <https://ampl.com/NEW/ampltabl/ampltabl.linux32.odbc.1.zip>`_ for systems with libodbc.so.1
  * `ampltabl.linux32.odbc.2.zip <https://ampl.com/NEW/ampltabl/ampltabl.linux32.odbc.2.zip>`_ for systems with libodbc.so.2

* 64-bit for use with 64-bit AMPL:

  * `ampltabl.linux64.odbc.1.zip <https://ampl.com/NEW/ampltabl/ampltabl.linux64.odbc.1.zip>`_ for systems with libodbc.so.1
  * `ampltabl.linux64.odbc.2.zip <https://ampl.com/NEW/ampltabl/ampltabl.linux64.odbc.2.zip>`_ for systems with libodbc.so.2
  * `ampltabl.linux64.iodbc.2.zip <https://ampl.com/NEW/ampltabl/ampltabl.linux64.iodbc.2.zip>`_ for systems with libiodbc.so.2

Click to download a zipfile, then unpack to obtain the file `ampltabl.dll` (32-bit) or `ampltabl_64.dll` (64-bit). Install by copying this file to the same directory that contains the `ampl` binary and the rest of the AMPL distribution.

Next you will need to install an ODBC database connector, and you will have to adjust your `table` statements to provide the corresponding connection strings. Details depend on which database you are using.

MySQL and Oracle
^^^^^^^^^^^^^^^^

Go to one of our pages on :doc:`mysql` or :doc:`oracle` and follow the instructions for your Linux distribution in the Installation section. Then see the Usage section for instructions on specifying the necessary connection strings in the `table` statement; except for these adjustments, the `table` statement works as described for Microsoft Office applications in `chapter 10 <https://ampl.com/BOOK/CHAPTERS/13-tables.pdf>`_. On the same page you may also want to consult the sections on embedding general SQL statements and on troubleshooting.

Other database applications
^^^^^^^^^^^^^^^^^^^^^^^^^^^

Additional instruction pages are in preparation. For now, contact us at support@ampl.com to get connection string instructions for SQLite and other popular database applications.

macOS
-----
Obtain a 64-bit macOS ODBC table handler (for use with 64-bit AMPL) from the following link:

* `ampltabl.macosx64.zip <https://ampl.com/NEW/ampltabl/ampltabl.macosx64.zip>`_

Click to download a zipfile, then unpack to obtain the file `ampltabl_64.dll`. Install by copying this file to the same folder (directory) that contains the `ampl` binary and the rest of the AMPL distribution.

Next you will need to install an ODBC database connector, and you will have to adjust your `table` statements to provide the corresponding connection strings. Details depend on which database you are using.

MySQL
^^^^^

Go to our page on :doc:`mysql` and follow the instructions for macOS in the Installation section. Then see the Usage section for instructions on specifying the necessary connection strings in the `table` statement; except for these adjustments, the `table` statement works as described for Microsoft Office applications in `chapter 10 <https://ampl.com/BOOK/CHAPTERS/13-tables.pdf>`_. On the same page you may also want to consult the sections on embedding general SQL statements and on troubleshooting.

Other database applications
^^^^^^^^^^^^^^^^^^^^^^^^^^^

Additional instruction pages are in preparation. For now, contact us at support@ampl.com to get connection string instructions for other popular database applications.
