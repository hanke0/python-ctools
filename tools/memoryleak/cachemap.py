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


def dict_as_cachemap(size):
    class Cache(dict):
        def setnx(self, k, v):
            self[k] = v()

    return Cache()


run_type = 'cachemap'
if len(sys.argv) == 2 and sys.argv[1] == 'dict':
    ctools.CacheMap = dict_as_cachemap
    run_type = 'dict'

print('run type = ', run_type)

try:
    while True:
        start = time.time()
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

        print(run_type, run_times, "loop finish, cost", time.time() - start, flush=True)
        rss_bytes = process.memory_info().rss
        if rss_bytes > limit:
            print(run_type, "memory touch roof", rss_bytes, flush=True)
            sys.exit(1)

        run_times += 1
except KeyboardInterrupt:
    print(run_type, 'exit!')
    sys.exit(0)
