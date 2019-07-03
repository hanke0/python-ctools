import uuid
import os
import time
import sys

import psutil
import ctools


def random():
    return str(uuid.uuid1())


pid = os.getpid()

process = psutil.Process(pid)
print('PID =', pid)

run_times = 1
limit = 1024 * 1024 * 1024  # 1 GB

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


run_type = 'cachemap'
if len(sys.argv) == 2 and sys.argv[1] == 'dict':
    ctools.TTLCache = TTLCache
    run_type = 'dict'

print('run type = ', run_type)

try:
    while True:
        start = time.time()
        cache = ctools.TTLCache(1)

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
        for i in cache:
            pass
        for i in cache:
            cache.get(i)

        print(run_type, run_times, "loop finish, cost", time.time() - start, flush=True)
        rss_bytes = process.memory_info().rss
        if rss_bytes > limit:
            print("memory touch roof", rss_bytes, flush=True)
            sys.exit(1)
        run_times += 1

except KeyboardInterrupt:
    print(run_type, 'exit!')
    sys.exit(0)
