#!/usr/bin/env python
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


parser = argparse.ArgumentParser()
parser.add_argument(
    "-k", dest="pattern", help="Only run tests which match the given substring"
)
parser.add_argument("-v", "--verbose", dest="verbose", help="Verbose output")
parser.add_argument("-q", "--quiet", dest="quiet", help="Quiet output")
parser.add_argument(
    "-s",
    "--start-directory",
    default=".",
    dest="start_directory",
    help="Directory to start discovery ('.' default)",
)
parser.add_argument(
    "-p",
    "--python-path",
    dest="python_path",
    action="append",
    help="Include python path",
)


def main():
    args = parser.parse_args()
    if args.python_path:
        for i in args.python_path:
            sys.path.insert(0, i)

    argv = [sys.argv[0], "discover", "-s", args.start_directory]
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
