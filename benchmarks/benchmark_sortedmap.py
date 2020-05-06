import random

import ctools

max_item = 65536


def get_seq():
    seq = list(range(max_item))
    random.shuffle(seq)
    return seq


def get_map():
    return {i: i for i in get_seq()}


def get_sorted_map():
    s = ctools.SortedMap()
    s.update(get_map())
    return s


@benchmark_setup(seq=get_seq)
def benchmark_sorted_map_set(seq):
    s = ctools.SortedMap()
    for i in seq:
        s[i] = i


@benchmark_setup(seq=get_seq)
def benchmark_dict_set(seq):
    d = {}
    for i in seq:
        d[i] = i


@benchmark_setup(v=get_sorted_map)
def benchmark_sorted_map_get(v):
    for i in get_seq():
        k = v[i]


@benchmark_setup(v=get_map)
def benchmark_dict_get(v):
    for i in get_seq():
        k = v[i]


@benchmark_setup(v=get_map)
def benchmark_dict_del(v):
    for i in get_seq():
        del v[i]


@benchmark_setup(v=get_sorted_map)
def benchmark_sorted_map_del(v):
    for i in get_seq():
        del v[i]
