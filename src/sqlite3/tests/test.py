import os
import sys
import test_utils

if __name__ == '__main__':
    os.chdir(os.path.dirname(__file__) or os.curdir)
    args = sys.argv
    if len(args) != 3:
        print('Usage: {} ampl dll'.format(args[0]))
        sys.exit(1)
    ampl, dll = args[1], args[2]

    try:
        os.remove('diet.dat.db')
    except:
        pass
    os.system('sqlite3 diet.dat.db < diet.dat.sql')

    ts = test_utils.TestSuite(ampl, dll)
    ts.test_ampl_script('diet-sql-1.run', 'output-1.txt')
    ts.test_ampl_script('diet-sql-2.run', 'output-2.txt')
    if ts.failed:
        sys.exit(1)

