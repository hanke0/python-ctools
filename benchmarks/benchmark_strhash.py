import uuid
import ctools


def new_str():
    return str(uuid.uuid1())


@benchmark_setup(string=new_str)
def benchmark_strhash_default_method(string):
    ctools.strhash(string)


@benchmark_setup(string=new_str)
def benchmark_strhash_fnv1a(string):
    ctools.strhash(string, "fnv1a")


@benchmark_setup(string=new_str)
def benchmark_strhash_fnv1(string):
    ctools.strhash(string, "fnv1")


@benchmark_setup(string=new_str)
def benchmark_strhash_djb2(string):
    ctools.strhash(string, "djb2")


@benchmark_setup(string=new_str)
def benchmark_strhash_murmur(string):
    ctools.strhash(string, "murmur")
