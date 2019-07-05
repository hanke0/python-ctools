import uuid
import sys

import ctools


def random():
    return str(uuid.uuid1())


def dict_as_cachemap(size):
    class Cache(dict):
        def setnx(self, k, v):
            self[k] = v()

    return Cache()


def test():
    cache = ctools.CacheMap(1024)

    for c in range(0x0000efff, 0x000efff0):
        key = random()
        val = random()
        cache[key] = val
        assert cache[key] == val

    keys = cache.keys()
    values = cache.values()
    cache.setdefault(random(), random())
    cache.get(random(), random())
    cache.setnx(random(), lambda: random())
    cache.update(a=random(), b=random())
    s = str(cache)
    for i in cache:
        pass


if __name__ == '__main__':
    run_type = 'cachemap'
    if len(sys.argv) == 2 and sys.argv[1] == 'dict':
        ctools.CacheMap = dict_as_cachemap
        run_type = 'dict'

    print('run type = ', run_type)
    try:
        sys.exit(ctools.memory_leak_test(test, prefix=run_type))
    except KeyboardInterrupt:
        print("exit by Ctrl-C")
        sys.exit(0)
