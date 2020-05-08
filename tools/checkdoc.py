#!/usr/bin/env bash
"""Validate the docstring.

Need numpydoc 1.0, install latest develop:
    pip install git+https://github.com/numpy/numpydoc.git
"""

import argparse
import fnmatch
import inspect
import importlib
import re
import sys
import textwrap

try:
    from numpydoc.validate import validate
except ImportError:
    raise ImportError(
        "Can't import numpydoc or numpy doc." + "\n".join(__doc__.splitlines()[1:])
    )

IGNORE = [
    # Docstring text (summary) should start in the line immediately after the opening quotes
    # (not in the same line, or leaving a blank line in between)
    "GL01",
    "ES01",  # No extended summary found
    "RT01",  # No Returns section found
    "YD01",  # No Yields section found
    "SA01",  # See Also section not found
    "EX01",  # No examples section found
    "PR01",  # Parameters not documented
]


def _valid_obj(full_import_path, obj, verbose):
    if verbose:
        v = "# -- " + full_import_path + " "
        v += "-" * (80 - len(v))
        print(v)

    try:
        _ = inspect.signature(obj)
    except ValueError:
        sys.stderr.write(full_import_path)
        sys.stderr.write(": can't get signature\n")
        return 1

    errs = validate(full_import_path)["errors"]
    if not errs:
        return 0

    exit_code = 0
    for code, msg in errs:
        if code in IGNORE:
            continue
        output = "%s:%s:%s" % (full_import_path, code, msg)
        sys.stderr.write("\n\t".join(textwrap.wrap(output, 79)))
        sys.stderr.write("\n")
        exit_code = 1
    return exit_code


def _find_obj(glob_name, include_parent=True):
    for maxsplit in range(0, glob_name.count(".") + 1):
        module, *func_parts = glob_name.rsplit(".", maxsplit)
        try:
            obj = importlib.import_module(module)
        except ImportError:
            pass
        else:
            break
    else:
        raise ImportError("No module can be imported " 'from "{}"'.format(glob_name))

    glob = ""
    if func_parts[-1].endswith("*"):
        glob = func_parts[-1]
        func_parts = func_parts[:-1]

    path_parts = [module]
    for part in func_parts:
        obj = getattr(obj, part)
        path_parts.append(part)

    base = ".".join(path_parts)
    objs = []
    if include_parent and not inspect.ismodule(obj):
        objs.append((base, obj))

    if not glob:
        return objs

    recursive = False

    pub_match = lambda x: not x.startswith("_")  # noqa

    if glob == "*":
        match = pub_match
    elif glob == "**":
        recursive = True
        match = pub_match
    else:
        match = re.compile(glob.rstrip("*") + ".*").match
        recursive = glob.endswith("**")

    for attr in dir(obj):
        if not match(attr):
            continue
        variable = getattr(obj, attr)

        attr_name = base + "." + attr
        objs.append((attr_name, variable))
        if recursive:
            objs.extend(_find_obj(attr_name + ".**", include_parent=False))

    return objs


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0].lstrip())
    parser.add_argument("-s", "--skip", action="append")
    parser.add_argument("import_path", action="append")
    parser.add_argument("-v", "--verbose", action="store_true")

    args = parser.parse_args()
    verbose = args.verbose
    import_path = args.import_path
    skip = args.skip or ()

    exit_code = 0
    for g in import_path:
        for full_import_path, var in _find_obj(g.strip()):
            s = False
            for i in skip:
                if fnmatch.fnmatch(full_import_path, i):
                    if verbose:
                        v = "# -- " + full_import_path + " "
                        v += "-" * (80 - len(v))
                        print(v)
                    s = True
                    print(full_import_path + ": skip by ignore")
                    break
            if s:
                continue

            if _valid_obj(full_import_path, var, verbose):
                exit_code = 1

    return exit_code


if __name__ == "__main__":
    sys.path.insert(0, ".")
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        sys.stderr.flush()
        sys.exit(127)
    except BrokenPipeError:
        sys.exit(0)
