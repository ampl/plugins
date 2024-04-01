AMPL Plugins Documentation
==========================

A collection of plugins with CMake support for the `AMPL <https://ampl.com>`_ modelling language.

There are two main types of plugins:

* Table Handlers to read/write data from/to an external medium, such as a database, file, or spreadsheet
* Shared Libraries for user-defined functions

This repository includes:

* :doc:`amplp <amplp>` - a single header framework to develop plugins
* :doc:`amplcsv <amplcsv>` - a table handler for csv files
* :doc:`amplsqlite3 <amplsqlite3>` - an SQLite3 table handler
* :doc:`amplxl <amplxl>` - a table handler for xlsx files
* :doc:`eodbc <eodbc>` - an extended ODBC driver
* funclink - shared libraries for user-defined functions

* the original table handlers with cmake support

Guides
------

.. toctree::
   :maxdepth: 2

   userguides
   devguides
