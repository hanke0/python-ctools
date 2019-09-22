import os
import time
from collections import namedtuple

import psutil

_memory_report = namedtuple(
    "MemoryReport", "exc_code,cycles,max_rss,last_rss,max_incr,cum_incr,escaped"
)


def memory_leak_test(
        test, max_rss=None, max_incr=None, cycles=None, multi=10, log_prefix="", log=True
):
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
                "loop={}, escaped={}, rss={:,}, increase={:,}".format(
                    cycle_count, spend, rss_bytes, incr
                )
            )

            if rss_limit and rss_bytes > rss_limit:
                print_("rss {:,} touch roof {:,}".format(rss_bytes, rss_limit))
                exc_code = 1
                break

            if incr_limit and cum_incr > incr_limit:
                print_("rss increase {:,} touch roof {:,}".format(cum_incr, incr_limit))
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
