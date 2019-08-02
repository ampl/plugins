import os
import sys
import difflib
from subprocess import PIPE, Popen

def run_ampl_script(ampl, dll, script):
    cmd = '{} -i {} {}'.format(ampl, dll, script)
    p = Popen(cmd, shell=True, stdout=PIPE, stderr=PIPE)
    stdout, stderr = p.communicate()
    return stdout, stderr

def diff(str1, str2):
    lst1 = str1.replace('\r', '').split('\n')
    lst2 = str2.replace('\r', '').split('\n')
    return '\n'.join(difflib.unified_diff(lst1, lst2))

def check_stderr(stderr):
    if stderr != '':
        print('STDERR:')
        print(stderr)
        return False
    return True

def check_stdout(stdout, expected_output):
    d = diff(stdout, expected_output)
    if d != '':
        print('FAILED:')
        print(d)
        return False
    else:
        print('OK')
        return True

class Tests:
    def __init__(self):
        self.count = 0
        self.failed = False

    def test_ampl_script(self, script, expected_output_file):
        self.count += 1
        print('Test #{}: {}'.format(self.count, script))
        with open(expected_output_file, 'r') as f:
            expected_output = f.read()
        stdout, stderr = run_ampl_script(ampl, dll, script)
        if not check_stderr(stderr):
            self.failed = True
        if not check_stdout(stdout, expected_output):
            self.failed = True

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

    t = Tests()
    t.test_ampl_script('diet-sql-1.run', 'output-1.txt')
    t.test_ampl_script('diet-sql-2.run', 'output-2.txt')
    if t.failed:
        sys.exit(1)

