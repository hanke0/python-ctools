import unittest
import random
import string
import uuid
import sys
from datetime import datetime, timedelta

from ctools import *


class T(unittest.TestCase):

    def test_int8_to_datetime(self):
        start = datetime(2000, 1, 1)
        for i in range(1024):
            date = start + timedelta(days=i)
            d_int = date.year * 10000 + date.month * 100 + date.day
            self.assertEqual(date, int8_to_datetime(d_int))

        with self.assertRaises(ValueError):
            int8_to_datetime(1)

    def test_jump_consistent_hash(self):
        count = 1024
        bucket = 100
        m = {
            i: jump_consistent_hash(i, bucket)
            for i in range(count)
        }
        for i in range(count):
            b = jump_consistent_hash(i, bucket)
            self.assertEqual(m[i], b)
        n = {
            i: jump_consistent_hash(i, bucket + 1)
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
        us = "文字テキスト텍스트كتابة"
        a = strhash(s)
        for i in range(1024):
            self.assertEqual(strhash(s), a)

        for meth in ("fnv1a", "fnv1", "djb2", "murmur"):
            a = strhash(s, meth)
            b = strhash(us, meth)
            self.assertEqual(a, strhash(s, meth))
            self.assertEqual(b, strhash(us, meth))


def set_random(mp):
    key = str(uuid.uuid1())
    val = str(uuid.uuid1())
    mp[key] = val
    return key


class LFUMemoryLeakTest(unittest.TestCase):

    def assertRefEqual(self, v1, v2, msg=None):
        self.assertEqual(sys.getrefcount(v1), sys.getrefcount(v2), msg)

    def assert_ref_equal(self, lfu_cache, lkey, d, dkey):
        lval = lfu_cache[lkey]
        dval = d[dkey]
        lval = lfu_cache[lkey]
        dval = d[dkey]
        self.assertEqual(sys.getrefcount(lval), sys.getrefcount(dval))

        del lfu_cache[lkey]
        del d[dkey]
        self.assertEqual(sys.getrefcount(lval), sys.getrefcount(dval))

        del d
        del lfu_cache
        self.assertEqual(sys.getrefcount(lval), sys.getrefcount(dval))

    def test_one_ref_eq(self):
        cache = LFUCache(10)
        mp = {}

        lkey = set_random(cache)
        dkey = set_random(mp)

        self.assert_ref_equal(cache, lkey, mp, dkey)

        lkey = set_random(cache)
        dkey = set_random(mp)

        lval = cache[lkey]
        dval = mp[dkey]

        del cache
        del mp
        self.assertRefEqual(lval, dval)

    def test_replace_ref_eq(self):
        cache = LFUCache(10)
        mp = {}

        lkey = set_random(cache)
        dkey = set_random(mp)

        lval = cache[lkey]
        dval = mp[dkey]
        lval = cache[lkey]
        dval = mp[dkey]
        self.assertRefEqual(lval, dval)

        cache[lkey] = str(uuid.uuid1())
        mp[dkey] = str(uuid.uuid1())

        self.assertRefEqual(lval, dval)
        self.assertRefEqual(cache[lkey], mp[dkey])
        self.assertRefEqual(lkey, dkey)

    def test_many_ref_eq(self):

        lfu_cache = LFUCache(257)
        d = {}
        for i in range(1024):
            set_random(lfu_cache)
            set_random(d)

        while True:
            try:
                lkey = list(lfu_cache.keys())[0]
            except IndexError:
                break
            dkey = list(d.keys())[0]
            self.assert_ref_equal(lfu_cache, lkey, d, dkey)

    def test_raw_ref_eq(self):
        cache = LFUCache(10)
        store = cache._store()
        d = {}

        lkey = set_random(cache)
        dkey = set_random(d)

        lval = cache[lkey]
        lval = store[lkey]
        lval = cache[lkey]
        dval = d[dkey]

        self.assertRefEqual(lval, dval)

        del cache[lkey]
        del d[dkey]
        self.assertRefEqual(lval, dval)

        del cache
        del d
        self.assertRefEqual(lval, dval)


class LFUTest(unittest.TestCase):
    def test_get_set(self):
        d = LFUCache(2)
        d['a'] = 1
        d['c'] = 2
        d['e'] = 3
        self.assertEqual(len(d), 2)

        for i in range(10):
            self.assertEqual(d['c'], 2)

    def test_len(self):
        for m in range(254, 1024):
            d = LFUCache(m)
            for i in range(m + 1):
                d[str(i)] = str(i)

            self.assertEqual(len(d), m)

    def test_keys(self):
        cache = LFUCache(257)
        keys = []
        for m in range(1024):
            keys.append(set_random(cache))

        for k in cache.keys():
            self.assertIn(k, keys)

    def test_values(self):
        cache = LFUCache(257)
        values = []
        for m in range(1024):
            values.append(cache[set_random(cache)])

        for v in cache.values():
            self.assertIn(v, values)

    def test_items(self):
        cache = LFUCache(257)
        values = []
        keys = []
        for m in range(1024):
            k = set_random(cache)
            keys.append(k)
            values.append(cache[k])

        for k, v in cache.items():
            self.assertIn(k, keys)
            self.assertIn(v, values)

    def test_contains(self):
        cache = LFUCache(2)
        keys = []
        for m in range(2):
            keys.append(set_random(cache))

        for k in keys:
            self.assertIn(k, cache)

    def test_setdefault(self):
        cache = LFUCache(2)
        for m in range(2):
            set_random(cache)
        k = str(uuid.uuid1())
        v = str(uuid.uuid1())
        val = cache.setdefault(k, v)
        self.assertEqual(v, val)
        self.assertIn(k, cache)
        self.assertEqual(cache[k], v)

    def test_get(self):
        cache = LFUCache(2)
        for m in range(2):
            set_random(cache)
        k = str(uuid.uuid1())
        v = str(uuid.uuid1())
        self.assertEqual(v, cache.get(k, v))
        self.assertNotIn(k, cache)
        cache[k] = v
        self.assertEqual(v, cache.get(k, 1))

    def test_update(self):
        d = {'1': '1'}
        cache = LFUCache(len(d))
        cache.update(d)
        for k in d:
            self.assertIn(k, cache)
        self.assertEqual(len(cache), len(d))

        d = {'abc': '1'}
        cache = LFUCache(len(d))
        cache.update(**d)
        for k in d:
            self.assertIn(k, cache)
        self.assertEqual(len(cache), len(d))

    def test_setnx(self):
        cache = LFUCache(10)
        key = str(uuid.uuid1())
        val = str(uuid.uuid1())

        with self.assertRaises(TypeError):
            cache.setnx(1, 1)

        v = cache.setnx(key, lambda: val)
        self.assertEqual(val, v)

        v = cache.setnx(key, lambda: 1)
        self.assertEqual(v, val)

    def test_iter(self):
        cache = LFUCache(257)
        keys = []
        for m in range(1024):
            k = set_random(cache)
            keys.append(k)

        for k in cache:
            self.assertIn(k, keys)


if __name__ == '__main__':
    unittest.main()
