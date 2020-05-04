import unittest
import uuid
import sys
from contextlib import contextmanager

import ctools
from ctools import _ctools


class A:
    def __init__(self, a):
        self.a = a

    def __lt__(self, other):
        if isinstance(other, A):
            return self.a < other.a
        return self.a < other

    def __eq__(self, other):
        if isinstance(other, A):
            return self.a == other.a
        return self.a == other

    def __hash__(self):
        return hash(self.a)

    def __repr__(self):
        return str(self.a)

    def __str__(self):
        return self.__repr__()


class DefaultEntry:
    def __init__(self, o):
        self.o = o

    def get_value(self):
        return self.o


@contextmanager
def not_raise(exc=Exception):
    try:
        yield
    except exc:
        pass


class BaseTestEntry(unittest.TestCase):
    def assertRefEqual(self, a, b, msg=None):
        self.assertEqual(sys.getrefcount(a), sys.getrefcount(b), msg=msg)

    def test_normal_ref(self):
        entry = self.create_entry()
        a = DefaultEntry(uuid.uuid1())
        self.assertRefEqual(entry, a)

        v1 = entry.get_value()
        v2 = entry.get_value()
        self.assertRefEqual(v1, v2)

        del entry
        del a
        self.assertRefEqual(v1, v2)

    def create_entry(self):
        return _ctools.CacheMapEntry(uuid.uuid1())


def map_set_random(mp):
    key = str(uuid.uuid1())
    val = str(uuid.uuid1())
    mp[key] = val
    return key, val


class DefaultMap(dict):
    def setnx(self, k, ca):
        if k not in self:
            v = ca(k)
            self[k] = v
            return v
        return self[k]

    def _storage(self):
        return self

    def keys(self):
        return list(super().keys())

    def values(self):
        return list(super().values())

    def items(self):
        return list(super().items())

    def __iter__(self):
        return self.keys()


class TestCacheMap(unittest.TestCase):
    def create_map(self, maxsize=257):
        return ctools.CacheMap(maxsize)

    def assert_ref(self, a, b, msg=None):
        self.assertEqual(sys.getrefcount(a), sys.getrefcount(b), msg=msg)

    def test_normal_set_item(self):
        cache = self.create_map()
        mp = DefaultMap()

        ckey, cval = map_set_random(cache)
        dkey, dval = map_set_random(mp)

        self.assert_ref(ckey, dkey)
        self.assert_ref(cval, dval)

    def test_normal_contains(self):
        cache = self.create_map()
        mp = DefaultMap()

        ckey, cval = map_set_random(cache)
        dkey, dval = map_set_random(mp)
        self.assertIn(ckey, cache)
        a = ckey in cache
        self.assert_ref(ckey, dkey)
        self.assert_ref(cval, dval)

    def test_normal_del_item(self):
        cache = self.create_map()
        mp = DefaultMap()

        ckey, cval = map_set_random(cache)
        dkey, dval = map_set_random(mp)
        del cache[ckey]
        del mp[dkey]
        self.assert_ref(ckey, dkey)
        self.assert_ref(cval, dval)

    def test_normal_get_item(self):
        cache = self.create_map()
        mp = DefaultMap()

        ckey, cval = map_set_random(cache)
        dkey, dval = map_set_random(mp)
        a = cache[ckey]
        d = mp[dkey]
        self.assert_ref(ckey, dkey)
        self.assert_ref(cval, dval)

        with not_raise():
            _ = cache[1]

        self.assert_ref(ckey, dkey)
        self.assert_ref(cval, dval)

    def test_normal_del(self):
        cache = self.create_map()
        mp = DefaultMap()

        ckey, cval = map_set_random(cache)
        dkey, dval = map_set_random(mp)
        del cache
        del mp
        self.assert_ref(ckey, dkey)
        self.assert_ref(cval, dval)

    def test_normal_replace(self):
        cache = self.create_map()
        mp = DefaultMap()

        ckey, cval = map_set_random(cache)
        dkey, dval = map_set_random(mp)

        self.assert_ref(ckey, dkey)
        self.assert_ref(cval, dval)

        cache[ckey] = str(uuid.uuid1())
        mp[dkey] = str(uuid.uuid1())

        self.assert_ref(ckey, dkey)
        self.assert_ref(cval, dval)
        self.assert_ref(cache[ckey], mp[dkey])

    def test_normal_set_many(self):
        cache = self.create_map()
        mp = DefaultMap()
        store = []
        for i in range(1024):
            ckey, cval = map_set_random(cache)
            dkey, dval = map_set_random(mp)
            store.append((ckey, dkey, cval, dval))

        for ckey, dkey, cval, dval in store:
            if ckey in cache:
                self.assert_ref(ckey, dkey)
                self.assert_ref(cval, dval)

        del cache
        del mp
        for ckey, dkey, cval, dval in store:
            self.assert_ref(ckey, dkey)
            self.assert_ref(cval, dval)

    def test_normal_keys(self):
        cache = self.create_map()
        mp = DefaultMap()
        store = []
        for m in range(1024):
            ckey, cval = map_set_random(cache)
            dkey, dval = map_set_random(mp)
            store.append((ckey, dkey, cval, dval))

        cit = cache.keys()
        dit = mp.keys()

        for ckey, dkey, cval, dval in store:
            if ckey in cache:
                self.assert_ref(ckey, dkey)
                self.assert_ref(cval, dval)

        del cit
        del dit
        for ckey, dkey, cval, dval in store:
            if ckey in cache:
                self.assert_ref(ckey, dkey)
                self.assert_ref(cval, dval)

    def test_normal_values(self):
        cache = self.create_map()
        mp = DefaultMap()
        store = []
        for m in range(1024):
            ckey, cval = map_set_random(cache)
            dkey, dval = map_set_random(mp)
            store.append((ckey, dkey, cval, dval))

        cit = cache.values()
        dit = mp.values()

        for ckey, dkey, cval, dval in store:
            if ckey in cache:
                self.assert_ref(ckey, dkey)
                self.assert_ref(cval, dval)

        del cit
        del dit
        for ckey, dkey, cval, dval in store:
            if ckey in cache:
                self.assert_ref(ckey, dkey)
                self.assert_ref(cval, dval)

    def test_normal_items(self):
        cache = self.create_map()
        mp = DefaultMap()
        store = []
        for m in range(1024):
            ckey, cval = map_set_random(cache)
            dkey, dval = map_set_random(mp)
            store.append((ckey, dkey, cval, dval))

        cit = cache.items()
        dit = mp.items()

        for ckey, dkey, cval, dval in store:
            if ckey in cache:
                self.assert_ref(ckey, dkey)
                self.assert_ref(cval, dval)

        del cit
        del dit
        for ckey, dkey, cval, dval in store:
            if ckey in cache:
                self.assert_ref(ckey, dkey)
                self.assert_ref(cval, dval)

    def test_normal_iter(self):
        cache = self.create_map()
        mp = DefaultMap()
        store = []
        for m in range(1024):
            ckey, cval = map_set_random(cache)
            dkey, dval = map_set_random(mp)
            store.append((ckey, dkey, cval, dval))

        ckeys = [i[0] for i in store]
        dkys = [i[1] for i in store]
        for i in cache:
            self.assertIn(i, ckeys)
            del i

        for ckey, dkey, cval, dval in store:
            if ckey in cache:
                self.assert_ref(ckey, dkey)
                self.assert_ref(cval, dval)

    def test_normal_update(self):
        d = {}
        c = {}
        dkey, dval = map_set_random(d)
        ckey, cval = map_set_random(c)
        cache = self.create_map()
        cache.update(c)
        cache.update(a=1)
        mp = DefaultMap()
        mp.update(d)
        mp.update(a=1)

        for k in c:
            self.assertEqual(c[k], cache[k])
            del k
        self.assert_ref(dkey, ckey)
        self.assert_ref(dval, cval)
        self.assertEqual(len(cache), len(mp))
        self.assertIn("a", cache)

    def test_normal_setdefault(self):
        cache = self.create_map()
        mp = DefaultMap()

        ckey, cval = map_set_random(cache)
        dkey, dval = map_set_random(mp)
        val = cache.setdefault(ckey, 1)
        val1 = mp.setdefault(dkey, 1)
        self.assertEqual(cval, val)
        self.assert_ref(ckey, dkey)
        self.assert_ref(cval, dval)

        cnkey = str(uuid.uuid1())
        dnkey = str(uuid.uuid1())
        dnval = str(uuid.uuid1())
        cnval = str(uuid.uuid1())
        val = cache.setdefault(cnkey, cnval)
        val1 = mp.setdefault(dnkey, dnval)
        self.assertEqual(val, cnval)
        self.assert_ref(cnval, dnval)
        self.assert_ref(cnkey, dnkey)

    def test_normal_setnx(self):
        fn = lambda k: str(uuid.uuid1()) + str(k)
        cache = self.create_map()
        mp = DefaultMap()

        ckey, cval = map_set_random(cache)
        dkey, dval = map_set_random(mp)
        cnval = cache.setnx(ckey, fn)
        dnval = mp.setnx(dkey, fn)
        self.assertEqual(cval, cnval)
        self.assert_ref(ckey, dkey)
        self.assert_ref(cval, dval)
        self.assert_ref(cnval, dnval)

        dnkey = str(uuid.uuid1())
        cnkey = str(uuid.uuid1())
        cnval = cache.setnx(cnkey, fn)
        dnval = mp.setnx(dnkey, fn)
        self.assertEqual(cnval, cache[cnkey])
        self.assert_ref(cnkey, dnkey)
        self.assert_ref(cnval, dnval)

    def test_normal_clear(self):
        cache = self.create_map()
        mp = DefaultMap()
        ckey, cval = map_set_random(cache)
        dkey, dval = map_set_random(mp)
        cache.clear()
        mp.clear()
        self.assertEqual(len(cache), 0, msg="cache map is not empty after clear")
        self.assert_ref(ckey, dkey)
        self.assert_ref(cval, dval)

    def test_error_set(self):
        ckey = dict()
        map_set_random(ckey)
        dkey = dict()
        map_set_random(dkey)
        cval = dict()
        map_set_random(cval)
        dval = dict()
        map_set_random(dval)
        cache = self.create_map()
        mp = DefaultMap()
        with not_raise():
            cache[ckey] = cval

        with not_raise():
            mp[dkey] = dval

        self.assert_ref(ckey, dkey)
        self.assert_ref(cval, dval)

    def test_get_set(self):
        d = self.create_map(2)
        d["a"] = 1
        d["c"] = 2
        # py35 dict is not ordered, add priority of key 'c' here
        self.assertEqual(d["c"], 2)
        d["e"] = 3
        self.assertEqual(len(d), 2)

        for i in range(10):
            self.assertEqual(d["c"], 2)

    def test_len(self):
        for m in range(254, 1024):
            d = self.create_map(m)
            for i in range(m + 1):
                d[str(i)] = str(i)
                if i < m and len(d) != i + 1:
                    self.assertEqual(len(d), i)

            self.assertEqual(len(d), m)

    def test_popitem(self):
        cache = self.create_map()
        mapping = {}
        key1 = A(1)
        key2 = A(1)
        cache[key1] = key1
        mapping[key2] = key2

        self.assert_ref(key2, key1)

        k1, v1 = cache.popitem()
        k2, v2 = mapping.popitem()

        self.assertEqual(k2, k1)
        self.assertEqual(v2, v1)

        self.assert_ref(key2, key1)
        del cache, mapping
        self.assert_ref(key2, key1)


if __name__ == "__main__":
    unittest.main()
