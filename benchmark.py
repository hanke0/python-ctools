# -*- coding: utf-8 -*-
import timeit

from ctools import int8_to_datetime


def humanize(t):
    if t > 0.1:
        return "%06f s" % t
    t *= 1000
    if t > 0.1:
        return "%06f ms" % t
    t *= 1000
    if t > 0.1:
        return "%06f µs" % t
    t *= 1000
    return "%06f ns" % t


def run_str(s, loop=1000000):
    t = timeit.timeit(s, globals=globals(), number=loop) / loop
    print("avg = ", t, "(", humanize(t), "),\tloop = ", loop, sep="", flush=True)


run_str("int8_to_datetime(20170101)")
