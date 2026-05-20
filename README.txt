amplfunc.c contains code for user-defined functions logistic and singpow

To create a dynamic library amplfunc.dll, just invoke `make`

amplfunc.dll is automatically loaded by AMPL if found. Otherwise, load it in ampl with the `load amplfunc.dll;` command

To use defined functions, just define them:
```
function signpow;
function logistic;
```

Try running lspf.mod with Minos or other ASL solvers

