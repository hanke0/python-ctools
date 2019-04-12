# -*- coding: utf-8 -*-
import timeit
import sys
import io

from ctools import *


def humanize(t):
    if t > 1:
        return "%.3f s" % t
    t *= 1000
    if t > 1:
        return "%.3f ms" % t
    t *= 1000
    if t > 1:
        return "%.3f µs" % t
    t *= 1000
    return "%.3f ns" % t


def mean_std(seq):
    mean = sum(seq) / len(seq)
    return mean, sum((x - mean) ** 2 for x in seq) ** 0.5


buffer = io.StringIO()


def run_str(s, loop=1000000, repeat=10):
    t_arr = timeit.repeat(s, globals=globals(), number=loop, repeat=repeat)
    t_arr = [t / loop for t in t_arr]
    mean, std = mean_std(t_arr)
    print(
        s, ",\t", humanize(mean), " ± ", humanize(std),
        "each ({:,} runs, {:,} loops)".format(repeat, loop),
        sep="", flush=True, file=sys.stderr
    )


run_str("int8_to_datetime(20170101)")
run_str("jump_consistent_hash(65535, 1024)")
run_str("strhash('zxgfhyxjhjtepqoikns')")
