import os, glob

# scan for test cases; any file matching "test_*.py"
__all__ = list()
testdir = os.path.dirname(__file__)
pattern = os.path.join(testdir, "test_*.py")
files = glob.glob(pattern)
for f in files:
    if os.path.isfile(f):
        (root, ext) = os.path.splitext(os.path.basename(f))
        __all__.append(root)
