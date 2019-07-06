from time import sleep
import unittest
import uuid
import sys

import ctools
from ctools.tests._bases import BaseTestEntry, BaseTestMapLike


class TestTTLCacheEntry(BaseTestEntry):
    def create_entry(self):
        return ctools.TTLCacheEntry(uuid.uuid1(), 1024)


def set_random(mp):
    key = str(uuid.uuid1())
    val = str(uuid.uuid1())
    mp[key] = val
    return key


class TestTTLCache(BaseTestMapLike):
    def create_map(self):
        return ctools.TTLCache(1024)

    def test_len(self):
        d = ctools.TTLCache(1024)
        for i in range(12):
            d[str(i)] = str(i)

        self.assertEqual(len(d), 12)

    def test_expire(self):
        cache = ctools.TTLCache(1)
        cache[1] = 1
        sleep(2)
        self.assertIsNone(cache.get(1, None))
        cache[1] = 1
        sleep(2)
        self.assertNotIn(1, cache)
        cache[1] = 1
        sleep(2)
        with self.assertRaises(KeyError):
            _ = cache[1]


del BaseTestEntry, BaseTestMapLike

if __name__ == "__main__":
    unittest.main()
