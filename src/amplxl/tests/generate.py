import os
import sys

DIET_IN="""
option display_precision 0;
model diet.mod;

param dname symbolic := "amplxl";
param fname symbolic := "#1#";

table Amounts IN (dname) (fname):
    [NUTR,FOOD], amt;
table Foods IN (dname) (fname):
    FOOD <- [FOOD], cost, f_min, f_max;
table Nutrients IN (dname) (fname):
    NUTR <- [NUTR], n_min, n_max;

read table Foods;
read table Nutrients;
read table Amounts;

display NUTR;
display FOOD;
display amt;

solve;

display {j in FOOD} (Buy[j], Buy.rc[j],  Buy[j]/f_max[j]);
"""

DIET_INOUT="""
option display_precision 0;
model diet.mod;

param dname symbolic := "amplxl";
param fname_in symbolic := "#1#";
param fname_out symbolic := "#2#";

table Amounts IN (dname) (fname_in):
    [NUTR,FOOD], amt;
table Foods IN (dname) (fname_in):
    FOOD <- [FOOD], cost, f_min, f_max;
table Nutrients IN (dname) (fname_in):
    NUTR <- [NUTR], n_min, n_max;

read table Foods;
read table Nutrients;
read table Amounts;

display NUTR;
display FOOD;
display amt;

solve;

display {j in FOOD} (Buy[j], Buy.rc[j],  Buy[j]/f_max[j]);

table ExportFoods (dname) (fname_out) "Foods":
    [FOOD] IN, Buy OUT, Buy.rc ~ BuyRC OUT, {j in FOOD} Buy[j]/f_max[j] ~ BuyFrac OUT;
write table ExportFoods;

# read exported table to validate results
reset;
option display_precision 0;

param dname symbolic := "amplxl";
param fname_out symbolic := "#2#";

set FOOD;
param cost{FOOD};
param f_min{FOOD};
param f_max{FOOD};
param Buy{FOOD};
param BuyFrac{FOOD};
param BuyRC{FOOD};

table Foods IN (dname) (fname_out):
    FOOD <- [FOOD], cost, f_min, f_max, Buy, BuyFrac, BuyRC;

read table Foods;

display FOOD;
display cost;
display f_min;
display f_max;
display Buy;
display BuyFrac;
display BuyRC;
"""

DIET_OUT="""
option display_precision 0;
model diet.mod;

param dname symbolic := "amplxl";
param fname_in symbolic := "#1#";
param fname_out symbolic := "#2#";

table Amounts IN (dname) (fname_in):
    [NUTR,FOOD], amt;
table Foods IN (dname) (fname_in):
    FOOD <- [FOOD], cost, f_min, f_max;
table Nutrients IN (dname) (fname_in):
    NUTR <- [NUTR], n_min, n_max;

read table Foods;
read table Nutrients;
read table Amounts;

display NUTR;
display FOOD;
display amt;

solve;

display {j in FOOD} (Buy[j], Buy.rc[j],  Buy[j]/f_max[j]);

table ExportFoods OUT (dname) (fname_out) "Foods":
    [FOOD], cost, f_min, f_max, Buy, Buy.rc ~ BuyRC, {j in FOOD} Buy[j]/f_max[j] ~ BuyFrac;
write table ExportFoods;

# read exported table to validate results
reset;
option display_precision 0;

param dname symbolic := "amplxl";
param fname_out symbolic := "#2#";

set FOOD;
param cost{FOOD};
param f_min{FOOD};
param f_max{FOOD};
param Buy{FOOD};
param BuyFrac{FOOD};
param BuyRC{FOOD};

table Foods IN (dname) (fname_out):
    FOOD <- [FOOD], cost, f_min, f_max, Buy, BuyFrac, BuyRC;

read table Foods;

display FOOD;
display cost;
display f_min;
display f_max;
display Buy;
display BuyFrac;
display BuyRC;
"""

DIET_INOUT_SINGLE="""
option display_precision 0;
model diet.mod;

param dname symbolic := "amplxl";
param fname_out symbolic := "#1#";

table Amounts IN (dname) (fname_out):
    [NUTR,FOOD], amt;

table Nutrients IN (dname) (fname_out):
    NUTR <- [NUTR], n_min, n_max;

table Foods (dname) (fname_out):
    [FOOD] IN, cost IN, f_min IN, f_max IN,
    Buy OUT, Buy.rc ~ BuyRC OUT,
    {j in FOOD} Buy[j]/f_max[j] ~ BuyFrac OUT;

read table Foods;
read table Nutrients;
read table Amounts;

display NUTR;
display FOOD;
display amt;

solve;

display {j in FOOD} (Buy[j], Buy.rc[j],  Buy[j]/f_max[j]);

write table Foods;

# read exported table to validate results
reset;
option display_precision 0;

param dname symbolic := "amplxl";
param fname_out symbolic := "#1#";

set FOOD;
param cost{FOOD};
param f_min{FOOD};
param f_max{FOOD};
param Buy{FOOD};
param BuyFrac{FOOD};
param BuyRC{FOOD};

table Foods IN (dname) (fname_out):
    FOOD <- [FOOD], cost, f_min, f_max, Buy, BuyFrac, BuyRC;

read table Foods;

display FOOD;
display cost;
display f_min;
display f_max;
display Buy;
display BuyFrac;
display BuyRC;
"""

if __name__ == '__main__':
    NTYPES = 3
    RUN_DIR = 'run/'
    XLSX_DIR = ''
    generated = []

    for i in range(NTYPES):
        name_in = os.path.join(XLSX_DIR, 'diet_in_{}.xlsx'.format(i+1))
        runfile = os.path.join(RUN_DIR, 'diet_in_{}.run'.format(i+1))
        with open(runfile, 'w') as f:
            f.write(
                DIET_IN.replace('#1#', name_in)
            )
        generated.append(runfile)

    for i in range(NTYPES):
        name_in = os.path.join(XLSX_DIR, 'diet_in_{}.xlsx'.format(i+1))
        name_out = os.path.join(XLSX_DIR, 'diet_inout_{}.xlsx'.format(i+1))
        runfile = os.path.join(RUN_DIR, 'diet_inout_{}.run'.format(i+1))
        with open(runfile, 'w') as f:
            f.write(
                DIET_INOUT.replace('#1#', name_in).replace('#2#', name_out)
            )
        generated.append(runfile)

    for i in range(NTYPES):
        name_in = os.path.join(XLSX_DIR, 'diet_in_{}.xlsx'.format(i+1))
        name_out = os.path.join(XLSX_DIR, 'diet_out_{}.xlsx'.format(i+1))
        runfile = os.path.join(RUN_DIR, 'diet_out_{}.run'.format(i+1))
        with open(runfile, 'w') as f:
            f.write(
                DIET_OUT.replace('#1#', name_in).replace('#2#', name_out)
            )
        generated.append(runfile)

    for i in range(NTYPES):
        name = os.path.join(XLSX_DIR, 'diet_inout_single_{}.xlsx'.format(i+1))
        runfile = os.path.join(RUN_DIR, 'diet_inout_single_{}.run'.format(i+1))
        with open(runfile, 'w') as f:
            f.write(
                DIET_INOUT_SINGLE.replace('#1#', name)
            )
        generated.append(runfile)

    for script in generated:
        os.system(
            'ampl -i amplxl.dll {} > {}'.format(
                script, script.replace('.run', '.out')
            )
        )
