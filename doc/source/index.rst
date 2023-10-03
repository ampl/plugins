AMPL Plugins Documentation
==========================

A collection of plugins with CMake support for the `AMPL <https://ampl.com>`_ modelling language.

There are two main types of plugins:

* Table Handlers to read/write data from/to an external medium, such as a database, file, or spreadsheet
* Shared Libraries for user-defined functions

This repository includes:

* amplp - a single header framework to develop plugins
* :doc:`amplcsv <amplcsv>` - a table handler for csv files
* eodbc - an experimental ODBC driver
* funclink - shared libraries for user-defined functions
* :doc:`amplxl <amplxl>` - a table handler for xlsx files
* the original table handlers with cmake support
* an experimental SQLite driver

Guides
------

.. toctree::
   :maxdepth: 2

   userguides
   devguides
