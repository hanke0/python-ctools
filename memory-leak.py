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
print(pid)

run_times = 1
limit = 1024 * 1024 * 1024  # 1 GB

while True:
    start = time.time()
    cache = ctools.LFUCache(1024)
    # d = {}
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
        break

    print(run_times, "loop finish, cost", time.time() - start, flush=True)
    rss_bytes = process.memory_info().rss
    if rss_bytes > limit:
        print("memory touch roof", rss_bytes, flush=True)
        sys.exit(1)

    run_times += 1
