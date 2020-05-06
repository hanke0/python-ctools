#!/usr/bin/env python
"""
A auto unit tests runner for CTools.

Use standard unittest library, so we can run in any python environment (no need install pytest).

"""
import sys
import os
import shutil
import unittest
import argparse

info = """
Testing Information
    Python： %(py)s
    PythonVer: %(pyver)s
    Version: %(version)s
    DebugBuild: %(debug)s
    PackageDir：%(package_path)s
    WorkingDir: %(path)s
"""


def _show_info():
    import ctools

    separate = "-" * shutil.get_terminal_size()[0]
    print(separate)
    print(
        info
        % dict(
            py=sys.executable,
            pyver=sys.version.replace("\n", " "),
            version=ctools.__version__,
            debug="true" if ctools.build_with_debug() else "false",
            package_path=ctools.__file__,
            path=os.getcwd(),
        )
    )
    print(separate)
    sys.stdout.flush()
    sys.stderr.flush()


parser = argparse.ArgumentParser(
    prog=os.path.basename(sys.argv[0]), description=__doc__.lstrip().splitlines()[0]
)
parser.add_argument(
    "-k", dest="pattern", help="only run tests which match the given substring"
)
parser.add_argument("-v", "--verbose", dest="verbose", help="verbose output")
parser.add_argument("-q", "--quiet", dest="quiet", help="quiet output")
parser.add_argument(
    "-s",
    "--start-directory",
    default=".",
    dest="start_directory",
    help="directory to start discovery ('.' default)",
)
parser.add_argument(
    "-p",
    "--python-path",
    dest="python_path",
    action="append",
    help="python path inserted into sys.path",
)


def main():
    args = parser.parse_args()
    if args.python_path:
        for i in args.python_path:
            sys.path.insert(0, i)

    argv = [sys.argv[0], "discover", "-s", args.start_directory, "-c"]
    if args.verbose:
        argv.append("-v")
    if args.quiet:
        argv.append("-q")
    if args.pattern:
        argv.append("-k")
        argv.append(args.pattern)
    _show_info()
    try:
        code = unittest.main(module=None, argv=argv)
    except SystemExit as exc:
        code = exc.code

    return code


if __name__ == "__main__":
    sys.exit(main())
