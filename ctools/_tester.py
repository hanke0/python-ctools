# Copyright 2019 ko-han. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import sys
import os


def _show_info(test_args):
    import ctools
    title = "Testing Information"
    print("\n-" * len(title))
    print(title)
    print("-" * len(title))
    print("Python: ", sys.executable)
    print('Ctools:', ctools.__version__, "(debug build)" if ctools.build_with_debug() else "")
    print("Arguments:", " ".join(test_args), '\n')


class Tester(object):
    def __init__(self, module_name):
        self.module_name = module_name

    def __call__(self, extra_argv=None, coverage=False, include_sys_argv=True, tests=None,
                 verbose=True):
        import pytest

        module = sys.modules[self.module_name]
        module_path = os.path.abspath(module.__path__[0])
        test_args = []

        if extra_argv:
            test_args.extend(extra_argv)

        if coverage:
            test_args.append("--cov=" + module_path)

        if tests is None:
            tests = [self.module_name]

        if include_sys_argv and len(sys.argv) > 1:
            test_args.extend(sys.argv[1:])

        test_args += ["--pyargs"] + list(tests)

        if verbose:
            for i in test_args:
                if i.startswith('-v'):
                    break
            else:
                test_args.append("-vvv")

        _show_info(test_args)

        try:
            code = pytest.main(test_args)
        except SystemExit as exc:
            code = exc.code

        return code


def memory_leak_test(test, max_rss=None, max_incr=None, circle=None, multi=10, prefix=""):
    import time
    import psutil
    pid = os.getpid()

    process = psutil.Process(pid)
    print('PID =', pid)

    run_times = 1
    last = None
    cum_incr = 0

    rss_limit = max_rss
    incr_limit = max_incr

    while True:
        start = time.time()
        test()
        spend = round(time.time() - start, 5)

        rss_bytes = process.memory_info().rss

        if last is None:
            last = rss_bytes
            if rss_limit is None:
                rss_limit = multi * last
            incr = 0.0
        else:
            incr = rss_bytes - last
            cum_incr += incr
            last = rss_bytes
            if incr_limit is None:
                if cum_incr > 0:
                    incr_limit = multi * cum_incr
            else:
                if cum_incr > incr_limit:
                    print(f'rss increase {cum_incr:,} touch roof {incr_limit:,}')
                    return 1

        if rss_limit and rss_bytes > rss_limit:
            print(f"rss {rss_bytes:,} touch roof {rss_limit:,}")
            return 1

        print(prefix, f"loop {run_times} finish, cost {spend}, ", end="")
        print(f"rss={rss_bytes:,}, increase={incr:,}")
        run_times += 1

        if circle is None:
            continue

        if run_times >= circle:
            return 0

    return 0
