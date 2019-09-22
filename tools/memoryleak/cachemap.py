import uuid
import sys
from contextlib import contextmanager

import ctools


def random():
    return str(uuid.uuid1())


def dict_as_cachemap(size):
    class Cache(dict):
        def setnx(self, k, v):
            self[k] = v()

    return Cache()


@contextmanager
def not_raise(exc=Exception):
    try:
        yield
    except exc:
        pass


def test():
    cache = ctools.CacheMap(1024)

    for c in range(42657):
        key = random()
        val = random()
        cache[key] = val
        assert cache[key] == val

    with not_raise():
        cache[[]] = uuid.uuid1()

    keys = cache.keys()
    values = cache.values()
    cache.setdefault(random(), random())
    cache.get(random(), random())
    cache.setnx(random(), lambda: random())
    cache.update(a=random(), b=random())
    s = str(cache)
    for i in cache:
        pass


if __name__ == "__main__":
    run_type = "cachemap"
    if len(sys.argv) == 2 and sys.argv[1] == "dict":
        ctools.CacheMap = dict_as_cachemap
        run_type = "dict"

    print("run type = ", run_type)
    from memleaktest import memory_leak_test
    try:
        report = ctools.memory_leak_test(test, log_prefix=run_type)
        sys.exit(report.exc_code)
    except KeyboardInterrupt:
        print("exit by Ctrl-C")
        sys.exit(0)
