# -*- coding: utf-8 -*-
import timeit
import sys
import io
import uuid
import random

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


def run_str(run, title, loop=1000000, repeat=10, **kwargs):
    t_arr = timeit.repeat(run, globals=globals(), number=loop, repeat=repeat, **kwargs)
    t_arr = [t / loop for t in t_arr]
    mean, std = mean_std(t_arr)
    print(
        title, ",\t", humanize(mean), " ± ", humanize(std),
        "each ({:,} runs, {:,} loops)".format(repeat, loop),
        sep="", flush=True, file=sys.stderr
    )


run_str("int8_to_datetime(20170101)", 'int8_to_datetime')

rad = random.randint(0, 0xffffffff)

run_str(
    "jump_consistent_hash(65535, 1024)",
    "jump_consistent_hash",
    setup="rad = random.randint(0, 0xffffffff)"
)

string = str(uuid.uuid1())
run_str("strhash(string)", "strhash default", setup="string = str(uuid.uuid1())")
run_str("strhash(string, 'fnv1a')", "strhash fnv1a", setup="string = str(uuid.uuid1())")
run_str("strhash(string, 'fnv1')", "strhash fnv1", setup="string = str(uuid.uuid1())")
run_str("strhash(string, 'djb2')", "strhash djb2", setup="string = str(uuid.uuid1())")
run_str("strhash(string, 'murmur')", "strhash murmur", setup="string = str(uuid.uuid1())")
