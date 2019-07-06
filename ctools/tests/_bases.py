import unittest
import uuid
import sys
from contextlib import contextmanager


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


class TestCase(unittest.TestCase):
    def assertRefEqual(self, a, b, msg=None):
        self.assertEqual(sys.getrefcount(a), sys.getrefcount(b), msg=msg)


class BaseTestEntry(TestCase):
    def create_entry(self):
        return DefaultEntry(uuid.uuid1())

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


def map_set_random(mp):
    key = str(uuid.uuid1())
    val = str(uuid.uuid1())
    mp[key] = val
    return key, val


class DefaultMap(dict):
    def setnx(self, k, ca):
        if k not in self:
            v = ca()
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


class BaseTestMapLike(TestCase):
    def create_map(self):
        return DefaultMap()

    def test_normal_set_item(self):
        cache = self.create_map()
        mp = DefaultMap()

        ckey, cval = map_set_random(cache)
        dkey, dval = map_set_random(mp)

        self.assertRefEqual(ckey, dkey)
        self.assertRefEqual(cval, dval)

    def test_normal_contains(self):
        cache = self.create_map()
        mp = DefaultMap()

        ckey, cval = map_set_random(cache)
        dkey, dval = map_set_random(mp)
        self.assertIn(ckey, cache)
        a = ckey in cache
        self.assertRefEqual(ckey, dkey)
        self.assertRefEqual(cval, dval)

    def test_normal_del_item(self):
        cache = self.create_map()
        mp = DefaultMap()

        ckey, cval = map_set_random(cache)
        dkey, dval = map_set_random(mp)
        del cache[ckey]
        del mp[dkey]
        self.assertRefEqual(ckey, dkey)
        self.assertRefEqual(cval, dval)

    def test_normal_get_item(self):
        cache = self.create_map()
        mp = DefaultMap()

        ckey, cval = map_set_random(cache)
        dkey, dval = map_set_random(mp)
        a = cache[ckey]
        d = mp[dkey]
        self.assertRefEqual(ckey, dkey)
        self.assertRefEqual(cval, dval)

        with not_raise():
            _ = cache[1]

        self.assertRefEqual(ckey, dkey)
        self.assertRefEqual(cval, dval)

    def test_normal_del(self):
        cache = self.create_map()
        mp = DefaultMap()

        ckey, cval = map_set_random(cache)
        dkey, dval = map_set_random(mp)
        del cache
        del mp
        self.assertRefEqual(ckey, dkey)
        self.assertRefEqual(cval, dval)

    def test_normal_replace(self):
        cache = self.create_map()
        mp = DefaultMap()

        ckey, cval = map_set_random(cache)
        dkey, dval = map_set_random(mp)

        self.assertRefEqual(ckey, dkey)
        self.assertRefEqual(cval, dval)

        cache[ckey] = str(uuid.uuid1())
        mp[dkey] = str(uuid.uuid1())

        self.assertRefEqual(ckey, dkey)
        self.assertRefEqual(cval, dval)
        self.assertRefEqual(cache[ckey], mp[dkey])

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
                self.assertRefEqual(ckey, dkey)
                self.assertRefEqual(cval, dval)

        del cache
        del mp
        for ckey, dkey, cval, dval in store:
            self.assertRefEqual(ckey, dkey)
            self.assertRefEqual(cval, dval)

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
                self.assertRefEqual(ckey, dkey)
                self.assertRefEqual(cval, dval)

        del cit
        del dit
        for ckey, dkey, cval, dval in store:
            if ckey in cache:
                self.assertRefEqual(ckey, dkey)
                self.assertRefEqual(cval, dval)

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
                self.assertRefEqual(ckey, dkey)
                self.assertRefEqual(cval, dval)

        del cit
        del dit
        for ckey, dkey, cval, dval in store:
            if ckey in cache:
                self.assertRefEqual(ckey, dkey)
                self.assertRefEqual(cval, dval)

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
                self.assertRefEqual(ckey, dkey)
                self.assertRefEqual(cval, dval)

        del cit
        del dit
        for ckey, dkey, cval, dval in store:
            if ckey in cache:
                self.assertRefEqual(ckey, dkey)
                self.assertRefEqual(cval, dval)

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
                self.assertRefEqual(ckey, dkey)
                self.assertRefEqual(cval, dval)

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
        self.assertRefEqual(dkey, ckey)
        self.assertRefEqual(dval, cval)
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
        self.assertRefEqual(ckey, dkey)
        self.assertRefEqual(cval, dval)

        cnkey = str(uuid.uuid1())
        dnkey = str(uuid.uuid1())
        dnval = str(uuid.uuid1())
        cnval = str(uuid.uuid1())
        val = cache.setdefault(cnkey, cnval)
        val1 = mp.setdefault(dnkey, dnval)
        self.assertEqual(val, cnval)
        self.assertRefEqual(cnval, dnval)
        self.assertRefEqual(cnkey, dnkey)

    def test_normal_setnx(self):
        c = lambda: str(uuid.uuid1())
        cache = self.create_map()
        mp = DefaultMap()

        ckey, cval = map_set_random(cache)
        dkey, dval = map_set_random(mp)
        cnval = cache.setnx(ckey, c)
        dnval = mp.setnx(dkey, c)
        self.assertEqual(cval, cnval)
        self.assertRefEqual(ckey, dkey)
        self.assertRefEqual(cval, dval)
        self.assertRefEqual(cnval, dnval)

        dnkey = str(uuid.uuid1())
        cnkey = str(uuid.uuid1())
        cnval = cache.setnx(cnkey, c)
        dnval = mp.setnx(dnkey, c)
        self.assertEqual(cnval, cache[cnkey])
        self.assertRefEqual(cnkey, dnkey)
        self.assertRefEqual(cnval, dnval)

    def test_normal_clear(self):
        cache = self.create_map()
        mp = DefaultMap()
        ckey, cval = map_set_random(cache)
        dkey, dval = map_set_random(mp)
        cache.clear()
        mp.clear()
        self.assertEqual(len(cache), 0, msg="cache map is not empty after clear")
        self.assertRefEqual(ckey, dkey)
        self.assertRefEqual(cval, dval)

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

        self.assertRefEqual(ckey, dkey)
        self.assertRefEqual(cval, dval)


if __name__ == "__main__":
    unittest.main()
