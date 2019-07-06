import uuid
import time
import sys
from contextlib import contextmanager

import ctools


def random_string():
    return str(uuid.uuid1())


@contextmanager
def not_raise(exc=Exception):
    try:
        yield
    except exc:
        pass


class TTLCache(dict):

    def __init__(self, ttl):
        self.ttl = ttl

    def __getitem__(self, item):
        v = dict.__getitem__(self, item)
        now = time.time()
        if v[0] < now:
            dict.__delitem__(self, item)
            raise KeyError(item)
        return v[1]

    def __setitem__(self, key, value):
        dict.__setitem__(self, key, (time.time() + self.ttl, value))

    def __contains__(self, item):
        try:
            self[item]
            return True
        except KeyError:
            return False

    def setnx(self, k, v):
        self.setdefault(k, v())

    def update(self, __m=None, **kwargs) -> None:
        for k, v in kwargs.items():
            self[k] = v
        if not __m:
            return
        for k, v in __m.items():
            self[k] = v


def test():
    cache = ctools.TTLCache(1)

    for c in range(43198):
        key = random_string()
        val = random_string()
        cache[key] = val
        assert cache[key] == val

    with not_raise():
        cache[[]] = uuid.uuid1()

    keys = cache.keys()
    values = cache.values()
    cache.setdefault(random_string(), random_string())
    cache.get(random_string(), random_string())
    cache.setnx(random_string(), lambda: random_string())
    cache.update(a=random_string(), b=random_string())
    s = str(cache)
    for i in cache:
        cache.get(i)


if __name__ == '__main__':
    run_type = 'ttlcache'
    if len(sys.argv) == 2 and sys.argv[1] == 'dict':
        ctools.TTLCache = TTLCache
        run_type = 'dict'

    print('run type = ', run_type)
    try:
        report = ctools.memory_leak_test(test, log_prefix=run_type)
        sys.exit(report.exc_code)
    except KeyboardInterrupt:
        print("exit by Ctrl-C")
        sys.exit(0)
