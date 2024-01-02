import os
import sys

sys.path.append("../..")
import test_utils


if __name__ == "__main__":
    os.chdir(os.path.dirname(__file__) or os.curdir)
    args = sys.argv
    if len(args) != 3:
        print("Usage: {} ampl dll".format(args[0]))
        sys.exit(1)
    ampl, dll = args[1], args[2]

    ts = test_utils.TestSuite(ampl, dll)
    RUN_DIR = "run"
    os.chdir(RUN_DIR)
    for fname in os.listdir("."):
        if fname.endswith(".run"):
            # ~ fname = os.path.join(RUN_DIR, fname)
            ts.test_ampl_script(fname, fname.replace(".run", ".out"))
    if ts.failed:
        sys.exit(1)
