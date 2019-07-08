#!/usr/bin/env python

import sys
import os
import subprocess
import argparse

sys.path.pop(0)

project_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def install(python, env):
    cmd = [python, "-m", "pip", "install", project_root]
    print("Install: ", " ".join(cmd))
    return subprocess.call(cmd, env=env)


def run_test(argv):
    sys.argv = sys.argv[:1] + argv
    import ctools

    return ctools.test()


def main(argv=None):
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument(
        "--no-install",
        action="store_true",
        default=False,
        help="do not build the project (use system installed version)",
    )
    parser.add_argument("--python", help="Python bin path")

    env = os.environ
    env.setdefault("CTOOLS_DEBUG", "ON")

    args, unknown_args = parser.parse_known_args(argv)

    python = args.python or sys.executable

    if not args.no_install:
        r = install(python, env)
        if r:
            return r
    if os.getenv('__CTOOLS_IN_SUBPROCESS', 'Y') == "Y":
        return run_test(unknown_args)

    env['__CTOOLS_IN_SUBPROCESS'] = "Y"
    return subprocess.call(
        [python, sys.argv[0], "--no-install"] + unknown_args, env=env
    )


if __name__ == "__main__":
    sys.exit(main())
