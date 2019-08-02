import os
import sys

DIET_IN="""
model diet.mod;

param fname symbolic := "#1#";

table Amounts IN "amplxl" (fname):
    [NUTR,FOOD], amt;
table Foods IN "amplxl" (fname):
    FOOD <- [FOOD], cost, f_min, f_max;
table Nutrients IN "amplxl" (fname):
    NUTR <- [NUTR], n_min, n_max;

read table Foods;
read table Nutrients;
read table Amounts;

display NUTR;
display FOOD;
display amt;

solve;
"""

DIET_INOUT="""
model diet.mod;

param fname_in symbolic := "#1#";
param fname_out symbolic := "#2#";

table Amounts IN "amplxl" (fname_in):
    [NUTR,FOOD], amt;
table Foods IN "amplxl" (fname_in):
    FOOD <- [FOOD], cost, f_min, f_max;
table Nutrients IN "amplxl" (fname_in):
    NUTR <- [NUTR], n_min, n_max;

read table Foods;
read table Nutrients;
read table Amounts;

display NUTR;
display FOOD;
display amt;

solve;

table ExportFoods "amplxl" (fname_out) "Foods":
    [FOOD], Buy INOUT;
write table ExportFoods;
"""

DIET_OUT="""
model diet.mod;

param fname_in symbolic := "#1#";
param fname_out symbolic := "#2#";

table Amounts IN "amplxl" (fname_in):
    [NUTR,FOOD], amt;
table Foods IN "amplxl" (fname_in):
    FOOD <- [FOOD], cost, f_min, f_max;
table Nutrients IN "amplxl" (fname_in):
    NUTR <- [NUTR], n_min, n_max;

read table Foods;
read table Nutrients;
read table Amounts;

display NUTR;
display FOOD;
display amt;

solve;

table ExportFoods OUT "amplxl" (fname_out) "Foods":
    FOOD <- [FOOD], Buy, Buy.rc ~ BuyRC, {j in FOOD} Buy[j]/f_max[j] ~ BuyFrac;
write table ExportFoods;
"""

DIET_OUT_SINGLE="""
model diet.mod;

param fname_out symbolic := "#1#";

table Amounts IN "amplxl" (fname_out):
    [NUTR,FOOD], amt;

table Nutrients IN "amplxl" (fname_out):
    NUTR <- [NUTR], n_min, n_max;

table Foods "amplxl" (fname_out):
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

write table Foods;
"""

if __name__ == '__main__':
    NTYPES = 3
    RUN_DIR = 'diet_run/'
    XLSX_DIR = 'diet_xlsx/'
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
        name = os.path.join(XLSX_DIR, 'diet_out_single_{}.xlsx'.format(i+1))
        runfile = os.path.join(RUN_DIR, 'diet_out_single_{}.run'.format(i+1))
        with open(runfile, 'w') as f:
            f.write(
                DIET_OUT_SINGLE.replace('#1#', name)
            )
        generated.append(runfile)

    for script in generated:
        os.system(
            'ampl -i amplxl.dll {} > {}'.format(
                script, script.replace('.run', '.out')
            )
        )

