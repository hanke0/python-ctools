#!/usr/bin/env bash
"""
Check doc use numpydoc.
Need numpydoc 1.0, install latest develop:
    pip install git+https://github.com/numpy/numpydoc.git
"""

import sys
import subprocess


def ismethod(m):
    return m.__class__.__name__ == "method_descriptor"


def isclass(c):
    return c.__class__.__name__ == "type"


def main():
    import ctools

    names = [i for i in dir(ctools) if not i.startswith("_")]
    paths = []
    for name in names:
        variable = getattr(ctools, name)
        if not isclass(variable):
            paths.append("ctools." + name)

        for m in dir(variable):
            if m.startswith("_"):
                continue
            method = getattr(variable, m)
            if ismethod(method):
                paths.append("ctools." + method.__qualname__)

    exit_code = 0
    cmd = [sys.executable, "-m", "numpydoc", "--validate", "placeholder"]
    for path in paths:
        cmd[-1] = path
        v = "# -- " + path + " "
        print(v, "-" * (80 - len(v)))
        e = subprocess.call(cmd)
        if e != 0:
            exit_code = e
    return exit_code


if __name__ == "__main__":
    sys.path.insert(0, ".")
    sys.exit(main())
