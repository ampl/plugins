.. _tableproxy:

AMPL’s tableproxy Table Handler
===============================

AMPL’s tableproxy table handler provides a way for a 64-bit version of AMPL to read tables from and write tables to a 32-bit database on the same computer (or vice versa, for a 32-bit AMPL to deal with a 64-bit database). It also provides a way to access databases on remote computers; such remote access is only intended for use when the license agreement for the relevant database program allows remote access. For example, the license agreement for Microsoft Access 2010 appears to permit a licensed user who has several machines to run Access on one machine and use it from another over a network connected to both machines: “The single primary user of the device hosting the remote desktop session may access and use the software remotely from any other device.”

The tableproxy handler is included in the ampltabl.dll library distributed by AMPL Optimization with Microsoft Windows versions of AMPL starting in March 2012. Current versions of this ampltabl.dll for various platforms
are also available here:

Download ampltabl

============================================================================ ================= ====
Platform                                                                     OS                Bits
============================================================================ ================= ====
`Intel x86_32 <https://ampl.com/NEW/TABLEPROXY/ampltabl.linux-intel32.tgz>`_ Linux             32
`Intel x86_64 <https://ampl.com/NEW/TABLEPROXY/ampltabl.linux-intel64.tgz>`_ Linux             64
`Intel IA64 <https://ampl.com/NEW/TABLEPROXY/ampltabl.linux-ia64.tgz>`_      Linux             64
`Intel x86_32 <https://ampl.com/NEW/TABLEPROXY/ampltabl.macosx32.tgz>`_      MacOSX            32
`Intel x86_64 <https://ampl.com/NEW/TABLEPROXY/ampltabl.macosx64.tgz>`_      MacOSX            64
`Intel x86_32 <https://ampl.com/NEW/TABLEPROXY/ampltabl.mswin32.zip>`_       Microsoft Windows 32
`Intel x86_64 <https://ampl.com/NEW/TABLEPROXY/ampltabl.mswin64.zip>`_       Microsoft Windows 64
`Power PC or RS6000 <https://ampl.com/NEW/TABLEPROXY/ampltabl.aix.tgz>`_     AIX               64
`Power PC <https://ampl.com/NEW/TABLEPROXY/ampltabl.linux-ppc.tgz>`_         Linux             64
`Intel x86_64 <https://ampl.com/NEW/TABLEPROXY/ampltabl.solaris-intel.tgz>`_ Solaris           64
`Sparc <https://ampl.com/NEW/TABLEPROXY/ampltabl.solaris-sparc32.tgz>`_      Solaris           32
`Sparc <https://ampl.com/NEW/TABLEPROXY/ampltabl.solaris-sparc64.tgz>`_      Solaris           64
============================================================================ ================= ====

For now, only the Microsoft Windows and MacOSX versions of ampltabl.dll include an ODBC handler as well as the tableproxy handler.

When execution begins, AMPL looks for two shared libraries, amplfunc.dll and ampltabl.dll, and loads them if found. They can provide imported functions and table handlers. AMPL looks for these libraries in $ampl_libpath (one directory per line, unless quoted), which you can specify in an "option ampl_libpath ..." command, such as

.. code-block:: none

  option ampl_libpath '/home/me/bin\
  /usr/local/bin';

or

.. code-block:: none

  option ampl_libpath '"/home/me/bin" "/usr/local/bin"';

or

.. code-block:: none

  option ampl_libpath "'/home/me/bin' '/usr/local/bin'";

which all have the same effect. Starting with AMPL version 20120126, the default $ampl_libpath is the directory in which the AMPL binary resides and (if different) the directory in which execution begins. With earlier version of AMPL, you might need to use a "load" command if ampltabl.dll or amplfunc.dll is not in the directory where you start AMPL.

To see if the tableproxy handler is available in your AMPL session, give the AMPL command

.. code-block:: none

  display _HANDLERS;

If so, the resulting output will mention “tableproxy”, in which case you can issue the AMPL command

.. code-block:: none

  print _handler_desc['tableproxy'];

to see a summary of how to use the tableproxy table handler. Just what you see may change slightly if the handler is updated, but here is what the above command gives with tableproxy version 20120212 and a 64-bit “ampl”:

.. code-block:: none

  Proxy table handler for using local 32-bit table handlers
  and handlers on remote machines (version 20120212).
  Strings expected before ":[...]":
        'tableproxy' Connection ...
  where ... are strings for the other handler.  Connection can involve
  zero or more of
        'ip=...[@ppp]'
        'prog=...'
        'hname=...'
        'rowchunk=mmm'
        'lib=...'
  At most one of 'prog=...' and 'ip=...' may appear.
  The ... in "prog=..." is the desired local program
  The ... in "ip=..." is a remote IP address at which a proxy table handler
  is running; the ppp in "@ppp", if present, is the IP port to use
  (default = 5196).
  The ... in 'hname=...' is the handler name (seen in "display _HANDLERS;"); if
  not given, hname is assumed to be the first string in the strings for the other
  handler (i.e., the ... following Connection).
  For reading tables, the mmm in 'rowchunk=mmm' is the maximum number of rows for
  the remote proxy to cache before sending them to the local proxy (default 512).
  The ... in "lib=..." is a shared library in which the remote proxy should
  look for a suitable handler.  If neither 'ip=' nor 'prog=' appears,
  'prog=tableproxy32' is assumed.

With a 32-bit “ampl”, the last line above would mention ‘prog=tableproxy64’.

Here is an example of using the “ODBC” table handler that has long worked with a 32-bit AMPL on MS Windows systems:

.. code-block:: none

  model diet.mod;
  table dietFoods 'ODBC' 'TABLES/diet.mdb' 'Foods':
     FOOD <- [FOOD], cost IN, f_min IN, f_max IN,
     Buy OUT, Buy.rc ~ BuyRC OUT, {j in FOOD} Buy[j]/f_max[j] ~ BuyFrac;
  table dietNutrs IN 'ODBC' 'TABLES/diet.mdb' 'Nutrients': NUTR <- [NUTR], n_min, n_max;
  table dietAmts IN 'ODBC' 'TABLES/diet.mdb' 'Amounts': [NUTR, FOOD], amt;
  read table dietFoods;
  read table dietNutrs;
  read table dietAmts;
  solve;
  write table dietFoods;

The example gets data from and writes data to a 32-bit Access database held in file diet.mdb. To use the same data base with a 64-bit version of AMPL, accessing the database with help from the tableproxy table handler, we could change

.. code-block:: none

  'ODBC'

to

.. code-block:: none

  'tableproxy' 'odbc'

in the table declarations, i.e., we could change the table declarations to

.. code-block:: none

  table dietFoods 'tableproxy' 'odbc' 'diet.mdb' 'Foods':
     FOOD <- [FOOD], cost IN, f_min IN, f_max IN,
     Buy OUT, Buy.rc ~ BuyRC OUT, {j in FOOD} Buy[j]/f_max[j] ~ BuyFrac;
  table dietNutrs IN 'tableproxy' 'odbc' 'diet.mdb' 'Nutrients':
     NUTR <- [NUTR], n_min, n_max;
  table dietAmts IN 'tableproxy' 'odbc' 'diet.mdb' 'Amounts': [NUTR, FOOD], amt;

Since neither 'prog=...' nor 'ip=...' appears in the above table declarations, the tableproxy handler acts as though 'prog=tableproxy32' had appeared. (With a 32-bit AMPL, 'prog=tableproxy64' would be assumed.)

The 'odbc' string following 'tableproxy' in the above table declarations is the name of the handler that tableproxy will use. Such handler names are the names as seen in AMPL's builtin _HANDLERS set. Older versions of the "standard" ODBC table handler had name 'odbc' but looked for string 'ODBC'. Newer versions (available starting February 2012) look for either 'odbc' or 'ODBC'. To use an the tableproxy32 program with an older version of ampltabl.dll, we could replace

.. code-block:: none

  'odbc'

with

.. code-block:: none

  'hname=odbc' 'ODBC'

which also works with newer versions of ampltabl.dll.

Sometimes it may be convenient to run AMPL on one machine and have it use a database on another machine. This can be done by using the tableproxy handler with AMPL and having it talk to a tableproxy server on the other machine. The tableproxy server is a stand-alone program, whose 32-bit version is called tableproxy32 and whose 64-bit version is called tableproxy64. It is available for various platforms:


Download tableproxy32 or tableproxy64

================================================================================ ================= ====
Platform                                                                         OS                Bits
================================================================================ ================= ====
`Intel x86_32 <https://ampl.com/NEW/TABLEPROXY/tableproxy32.linux-intel32.tgz>`_ Linux             32
`Intel x86_64 <https://ampl.com/NEW/TABLEPROXY/tableproxy64.linux-intel64.tgz>`_ Linux             64
`Intel IA64 <https://ampl.com/NEW/TABLEPROXY/tableproxy64.linux-ia64.tgz>`_      Linux             64
`Intel x86_32 <https://ampl.com/NEW/TABLEPROXY/tableproxy32.macosx32.tgz>`_      MacOSX            32
`Intel x86_64 <https://ampl.com/NEW/TABLEPROXY/tableproxy64.macosx64.tgz>`_      MacOSX            64
`Intel x86_32 <https://ampl.com/NEW/TABLEPROXY/tableproxy32.mswin32.zip>`_       Microsoft Windows 32
`Intel x86_64 <https://ampl.com/NEW/TABLEPROXY/tableproxy64.mswin64.zip>`_       Microsoft Windows 64
`Power PC or RS6000 <https://ampl.com/NEW/TABLEPROXY/tableproxy32.aix.tgz>`_     AIX               32
`Power PC or RS6000 <https://ampl.com/NEW/TABLEPROXY/tableproxy64.aix.tgz>`_     AIX               64
`Power PC <https://ampl.com/NEW/TABLEPROXY/tableproxy64.linux-ppc.tgz>`_         Linux             64
`Intel x86_64 <https://ampl.com/NEW/TABLEPROXY/tableproxy64.solaris-intel.tgz>`_ Solaris           64
`Sparc <https://ampl.com/NEW/TABLEPROXY/tableproxy32.solaris-sparc32.tgz>`_      Solaris           32
`Sparc <https://ampl.com/NEW/TABLEPROXY/tableproxy64.solaris-sparc64.tgz>`_      Solaris           64
================================================================================ ================= ====

Assuming suitable licensing of Microsoft Access, Access could be running (under MS Windows) on a machine where you have invoked

.. code-block:: none

  tableproxy32

or equivalently

.. code-block:: none

  tableproxy32 start

and if necessary have opened the relevant port (5196 by default) in the firewall. If, say, this machine has IP address 192.168.1.102, then you could run the above example with AMPL on another machine by changing the table declarations to

.. code-block:: none

  table dietFoods 'tableproxy' 'ip=192.168.1.102' 'odbc' 'diet.mdb' 'Foods':
     FOOD <- [FOOD], cost IN, f_min IN, f_max IN,
     Buy OUT, Buy.rc ~ BuyRC OUT, {j in FOOD} Buy[j]/f_max[j] ~ BuyFrac;
  table dietNutrs 'tableproxy' 'ip=192.168.1.102' 'odbc' 'diet.mdb' 'Nutrients':
     NUTR <- [NUTR], n_min, n_max;
  table dietAmts 'tableproxy' 'ip=192.168.1.102' 'odbc' 'diet.mdb' 'Amounts':
     [NUTR, FOOD], amt;

If the IP address is subject to change, it may be better to use an AMPL string expression to specify it, as in

.. code-block:: none

  param ip symbolic;
  data ip.dat;
  table dietFoods 'tableproxy' ('ip=' & ip) 'odbc' 'diet.mdb' 'Foods':
     FOOD <- [FOOD], cost IN, f_min IN, f_max IN,
     Buy OUT, Buy.rc ~ BuyRC OUT, {j in FOOD} Buy[j]/f_max[j] ~ BuyFrac;
  table dietNutrs  'tableproxy' ('ip=' & ip) 'odbc' 'diet.mdb' 'Nutrients':
     NUTR <- [NUTR], n_min, n_max;
  table dietAmts 'tableproxy' ('ip=' & ip) 'odbc' 'diet.mdb' 'Amounts':
     [NUTR, FOOD], amt;

The port used by the tableproxynn program (nn = 32 or 64) can be specified on its command line, as in

.. code-block:: none

  tableproxy32 port=5198

The tableproxynn program loads table handlers as needed. To see what it has loaded, you can invoke

.. code-block:: none

  tableproxy32 status

or

.. code-block:: none

  tableproxy64 status

or, e.g.,

.. code-block:: none

  tableproxy32 status port=5198

or (to see what a remote tableproxynn has loaded)

.. code-block:: none

  tableproxy32 status ip=192.168.1.102

Once started as a remote tableproxy server, the tableproxynn program continues to run until instructed to stop:

.. code-block:: none

  tableproxy32 stop

which must done on the machine where tableproxynn is running. On that machine,

.. code-block:: none

  tableproxy32 restart

has the same effect as

.. code-block:: none

  tableproxy32 stop
  tableproxy32 start

and similarly for "tableproxy64 restart". (The "start", "stop", and "restart" arguments permit using tableproxynn as a daemon.)

The 'prog=...' string could specify a suitable program other than tableproxy32 or tableproxy64. If you are comfortable doing your own computer programming, you can write such programs to operate as tableproxy32 and tableproxy64 do. You will need to study the source, which is available in netlib's ampl/tables directory and is also available `here <https://ampl.com/NEW/TABLEPROXY/tables.tgz>`_.

Some little examples (files *.x) and versions of ampltabl.dll and tableproxynn for 32- and 64-bit Linux, MacOSX, and MS Windows are available `here <https://ampl.com/NEW/TABLEPROXY/proxyexamples.tgz>`_. Some of the examples use the "lib-tab" and "simple-bit" table handlers whose source is included with the source mentioned above.
