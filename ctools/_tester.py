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
from collections import namedtuple

here = os.path.dirname(os.path.abspath(__file__))


def _show_info(test_args):
    import ctools

    title = "Testing Information"
    print("\n-" * len(title))
    print(title)
    print("-" * len(title))
    print("Python: ", sys.executable)
    print(
        "Ctools:",
        ctools.__version__,
        "(debug build)" if ctools.build_with_debug() else "",
    )
    print("Arguments:", " ".join(test_args), "\n")


class Tester(object):
    def __init__(self, module_name):
        self.module_name = module_name

    def __call__(
        self,
        extra_argv=None,
        coverage=False,
        include_sys_argv=True,
        tests=None,
        verbose=True,
    ):
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
        test_args += ["--deselect", os.path.join(here, "tests", "_bases.py")]

        if verbose:
            for i in test_args:
                if i.startswith("-v"):
                    break
            else:
                test_args.append("-vvv")

        _show_info(test_args)

        try:
            code = pytest.main(test_args)
        except SystemExit as exc:
            code = exc.code

        return code


_memory_report = namedtuple(
    "MemoryReport", "exc_code,cycles,max_rss,last_rss,max_incr,cum_incr,escaped"
)


def memory_leak_test(
    test, max_rss=None, max_incr=None, cycles=None, multi=10, log_prefix="", log=True
):
    import time
    import psutil

    if log:
        print_ = print
    else:
        print_ = lambda *_, **__: None

    pid = os.getpid()
    process = psutil.Process(pid)
    print_("PID =", pid)

    cycle_count = 1
    last_rss = None
    cum_incr = 0

    rss_limit = max_rss
    incr_limit = max_incr
    exc_code = 0
    escaped = 0.0

    try:
        while True:
            start = time.time()
            test()
            spend = round(time.time() - start, 5)
            escaped += spend

            rss_bytes = process.memory_info().rss

            if last_rss is None:
                last_rss = rss_bytes
                if rss_limit is None:
                    rss_limit = multi * last_rss
                incr = 0.0
            else:
                incr = rss_bytes - last_rss
                cum_incr += incr
                last_rss = rss_bytes
                if incr_limit is None:
                    if cum_incr > 0:
                        incr_limit = multi * cum_incr

            print_(log_prefix, end=" ")
            print_(
                f"loop={cycle_count}, escaped={spend}, rss={rss_bytes:,}, increase={incr:,}"
            )

            if rss_limit and rss_bytes > rss_limit:
                print_(f"rss {rss_bytes:,} touch roof {rss_limit:,}")
                exc_code = 1
                break

            if incr_limit and cum_incr > incr_limit:
                print_(f"rss increase {cum_incr:,} touch roof {incr_limit:,}")
                exc_code = 1
                break

            cycle_count += 1

            if cycles is None:
                continue

            if cycle_count >= cycles:
                break

    except KeyboardInterrupt:
        print_(
            _memory_report(
                exc_code, cycle_count, max_rss, last_rss, max_incr, cum_incr, escaped
            )
        )
        raise
    report = _memory_report(
        exc_code, cycle_count, max_rss, last_rss, max_incr, cum_incr, escaped
    )
    print_(report)
    return report
