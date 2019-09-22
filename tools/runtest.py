#!/usr/bin/env python
import sys
import os
import pytest


def terminal_size():
    try:
        columns, rows = os.get_terminal_size(0)
    except OSError:
        columns, rows = os.get_terminal_size(1)

    return columns, rows


def _show_info(test_args):
    import ctools

    separate = "-" * terminal_size()[0]
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


def main(argv=None):
    argv = argv or sys.argv
    _show_info(argv)
    try:
        code = pytest.main()
    except SystemExit as exc:
        code = exc.code

    return code


if __name__ == "__main__":
    sys.exit(main())
