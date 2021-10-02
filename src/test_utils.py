import os
import sys
import difflib
from subprocess import PIPE, Popen


def run_ampl_script(ampl, dll, script):
    cmd = '{} -i {} {}'.format(ampl, dll, script)
    p = Popen(cmd, shell=True, stdout=PIPE, stderr=PIPE)
    stdout, stderr = p.communicate()
    return stdout.decode(), stderr.decode()


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
    d = diff(expected_output, stdout)
    if d != '':
        print('FAILED:')
        print(d)
        return False
    else:
        print('OK')
        return True


class TestSuite:
    def __init__(self, ampl, dll):
        self.count = 0
        self.failed = False
        self.ampl = ampl
        self.dll = dll
        # Avoid "Cannot load library sqlite3th.dll"
        # It fails if it cannot find sqlite3.dll in the system PATH
        os.environ['PATH'] += os.pathsep + os.path.dirname(dll)

    def test_ampl_script(self, script, expected_output_file):
        self.count += 1
        print('Test #{}: {}'.format(self.count, script))
        with open(expected_output_file, 'r') as f:
            expected_output = f.read()
        stdout, stderr = run_ampl_script(self.ampl, self.dll, script)
        if not check_stderr(stderr):
            self.failed = True
        if not check_stdout(stdout, expected_output):
            self.failed = True
