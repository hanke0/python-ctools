#!/usr/bin/env python
import sys
import os
import shutil
import pytest


def _show_info():
    import ctools

    separate = "-" * shutil.get_terminal_size()[0]
    print(separate)
    print("Testing Information")
    print("\t", "Python: ", sys.executable)
    print(
        "\t",
        "Ctools:",
        ctools.__version__,
        "(debug build)" if ctools.build_with_debug() else "",
        "at %s" % ctools.__file__,
    )
    print(separate)


def _setup_project_root():
    project_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    sys.path.pop(0)
    sys.path.insert(0, project_root)


def main(argv=None):
    if argv is not None and "--no-project-root" in argv:
        argv.remove("--no-project-root")
    else:
        _setup_project_root()

    _show_info()
    try:
        code = pytest.main(argv)
    except SystemExit as exc:
        code = exc.code

    return code


if __name__ == "__main__":
    sys.exit(main())
