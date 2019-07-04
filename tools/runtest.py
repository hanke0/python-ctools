#!/usr/bin/env python

import sys
import os
import subprocess
import argparse

sys.path.pop(0)

project_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def install(python, env):
    cmd = [python, '-m', "pip", "install", project_root]
    print("Install: ", " ".join(cmd))
    return subprocess.call(cmd, env=env)


def run_test(argv):
    sys.argv = sys.argv[:1] + argv
    import ctools
    return ctools.test()


def main(argv=None):
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument("--no-install", "-n", action="store_true", default=False,
                        help="do not build the project (use system installed version)")
    parser.add_argument('--python', '-p', help="Python bin path")

    env = os.environ
    env.setdefault('CTOOLS_DEBUG', "ON")

    in_subprocess = False
    for i, s in enumerate(sys.argv):
        if s == "--run-test-in-this-python":
            sys.argv.pop(i)
            in_subprocess = True
            break

    args, unknown_args = parser.parse_known_args(argv)
    python = args.python or sys.executable

    if not args.no_install:
        r = install(python, env)
        if r:
            return r

    if not in_subprocess:
        return subprocess.call([python, sys.argv[0], '--no-install', '--run-test-in-this-python'] + unknown_args, env=env)
    else:
        return run_test(unknown_args)


if __name__ == '__main__':
    sys.exit(main())