import os
import sys


import test_utils


tests_file = "tests.txt"

if __name__ == "__main__":

	os.chdir(os.path.dirname(__file__) or os.curdir)
	args = sys.argv
	if len(args) != 3:
		print("Usage: {} ampl dll".format(args[0]))
		sys.exit(1)
	ampl, dll = args[1], args[2]

	ts = test_utils.TestSuite(ampl, dll)
	has_test_list = os.path.exists(tests_file)
	print("has_test_list:", has_test_list)

	if has_test_list:
		test_list = test_utils.get_test_list(tests_file)
		print("test_list:", test_list)
		for fname in test_list:
			ts.test_ampl_script(fname + ".run", fname + ".out")
	else:
		for fname in os.listdir("."):
			if fname.endswith(".run"):
				ts.test_ampl_script(fname, fname.replace(".run", ".out"))
	if ts.failed:
		sys.exit(1)
