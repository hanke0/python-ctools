import unittest
import uuid
import sys

import ctools
from ctools.tests._bases import BaseTestMapLike, BaseTestEntry


class TestCacheMapEntry(BaseTestEntry):
    def create_entry(self):
        return ctools.CacheMapEntry(uuid.uuid1())


def set_random(mp):
    key = str(uuid.uuid1())
    val = str(uuid.uuid1())
    mp[key] = val
    return key


class TestCacheMap(BaseTestMapLike):
    def create_map(self):
        return ctools.CacheMap(257)

    def test_get_set(self):
        d = ctools.CacheMap(2)
        d["a"] = 1
        d["c"] = 2
        d["e"] = 3
        self.assertEqual(len(d), 2)

        for i in range(10):
            self.assertEqual(d["c"], 2)

    def test_len(self):
        for m in range(254, 1024):
            d = ctools.CacheMap(m)
            for i in range(m + 1):
                d[str(i)] = str(i)
                if i < m and len(d) != i + 1:
                    self.assertEqual(len(d), i)

            self.assertEqual(len(d), m)


del BaseTestEntry, BaseTestMapLike

if __name__ == "__main__":
    unittest.main()
