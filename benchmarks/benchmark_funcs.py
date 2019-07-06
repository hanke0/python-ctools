import random

import ctools


def benchmark_int8_to_datetime():
    ctools.int8_to_datetime(20170101)


def rand_int():
    return random.randint(0, 0xFFFFFFFF)


@benchmark_setup(i=rand_int)
def benchmark_jump_consistent_hash(i):
    ctools.jump_consistent_hash(i, 1024)
