import unittest
import random
import string
import uuid
import sys
import gc
from datetime import datetime, timedelta

import ctools


class T(unittest.TestCase):

    def test_int8_to_datetime(self):
        start = datetime(2000, 1, 1)
        for i in range(1024):
            date = start + timedelta(days=i)
            d_int = date.year * 10000 + date.month * 100 + date.day
            self.assertEqual(date, ctools.int8_to_datetime(d_int))

        with self.assertRaises(ValueError):
            ctools.int8_to_datetime(1)

    def test_jump_consistent_hash(self):
        count = 1024
        bucket = 100
        m = {
            i: ctools.jump_consistent_hash(i, bucket)
            for i in range(count)
        }
        for i in range(count):
            b = ctools.jump_consistent_hash(i, bucket)
            self.assertEqual(m[i], b)
        n = {
            i: ctools.jump_consistent_hash(i, bucket + 1)
            for i in range(count)
        }
        equal_count = 0
        for i in range(count):
            if m[i] == n[i]:
                equal_count += 1

        # At most 1/6 keys is changed
        self.assertTrue(equal_count > (5 / 6 * count))

    def test_strhash(self):
        s = "".join(random.choice(string.printable) for _ in range(1024))
        a = ctools.strhash(s)
        for i in range(1024):
            self.assertEqual(ctools.strhash(s), a)


class LFUTest(unittest.TestCase):
    def test_get_set(self):
        d = ctools.LFUCache(2)
        d['a'] = 1
        d['c'] = 2
        d['e'] = 3
        self.assertEqual(len(d), 2)

        for i in range(10):
            self.assertEqual(d['c'], 2)

        self.assertEqual(d.lfu(), "e")

    def test_len(self):
        for m in range(254, 1024):
            d = ctools.LFUCache(m)
            for i in range(m+1):
                d[str(i)] = str(i)

            self.assertEqual(len(d), m)

    def assert_ref_equal(self, lfu_cache, lkey, d, dkey):
        ltmp = lfu_cache[lkey]
        dtmp = d[dkey]
        self.assertEqual(sys.getrefcount(ltmp), sys.getrefcount(dtmp))

        del lfu_cache[lkey]
        del d[dkey]
        self.assertEqual(sys.getrefcount(ltmp), sys.getrefcount(dtmp))

        del d
        del lfu_cache
        self.assertEqual(sys.getrefcount(ltmp), sys.getrefcount(dtmp))

    def set_random(self, dlike):
        key = str(uuid.uuid1())
        val = str(uuid.uuid1())
        dlike[key] = val
        return key

    def test_one_ref_eq(self):
        lfu_cache = ctools.LFUCache(10)
        d = {}

        lkey = self.set_random(lfu_cache)
        dkey = self.set_random(d)

        self.assert_ref_equal(lfu_cache, lkey, d, dkey)

    def test_many_ref_eq(self):
        lfu_cache = ctools.LFUCache(257)
        d = {}
        for i in range(1024):
            self.set_random(lfu_cache)
            self.set_random(d)

        for i in range(len(lfu_cache)):
            lkey = list(lfu_cache.keys())[i]
            dkey = list(d.keys())[i]
            self.assert_ref_equal(lfu_cache, lkey, d, dkey)


if __name__ == '__main__':
    unittest.main()
