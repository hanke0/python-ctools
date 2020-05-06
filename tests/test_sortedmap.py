import unittest
import sys
import random

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


class TestSortedMap(unittest.TestCase):
    def assert_ref(self, o1, o2, msg=None):
        self.assertEqual(sys.getrefcount(o1), sys.getrefcount(o2), msg=msg)

    def _create_sorted_map(self, cmp=None):
        return ctools.SortedMap(cmp)

    def _build_v(self, seq):
        sorted_map = ctools.SortedMap()
        mapping = dict()
        keys1 = [A(i) for i in seq]
        keys2 = [A(i) for i in seq]
        for i in range(len(seq)):
            key1 = keys1[i]
            key2 = keys2[i]
            sorted_map[key1] = key1
            mapping[key2] = key2
        return keys1, keys2, sorted_map, mapping

    def _test_setgetitem_list(self, seq):
        keys1, keys2, sorted_map, mapping = self._build_v(seq)
        self.assertEqual(len(mapping), len(sorted_map))

        for i in range(len(seq)):
            key1 = keys1[i]
            key2 = keys2[i]
            self.assert_ref(key1, key2, msg=seq)
            self.assertEqual(len(mapping), len(sorted_map))

        for i in range(len(seq)):
            key1 = keys1[i]
            key2 = keys2[i]
            self.assertEqual(mapping[key2], sorted_map[key1])
            self.assert_ref(key1, key2, msg=seq)
            self.assertEqual(len(mapping), len(sorted_map))

        del sorted_map, mapping
        for i in range(len(seq)):
            key1 = keys1[i]
            key2 = keys2[i]
            self.assert_ref(key1, key2, msg=seq)

    def test_setgetitem(self):
        self._test_setgetitem_list(list(range(1024)))
        self._test_setgetitem_list(list(reversed(list(range(1024)))))
        v = list(range(1024))
        random.shuffle(v)
        self._test_setgetitem_list(v)

    def test_inplace(self):
        seq = list(range(1024))
        random.shuffle(seq)
        keys1, keys2, sorted_map, mapping = self._build_v(seq)
        self.assertEqual(len(mapping), len(sorted_map))
        u1 = [A(i) for i in range(len(seq))]
        u2 = [A(i) for i in range(len(seq))]
        for i in range(len(u1)):
            k1 = u1[i]
            k2 = u2[i]
            sorted_map[k1] = k1
            mapping[k2] = k2
            self.assertEqual(len(mapping), len(sorted_map))

        for i in range(len(seq)):
            key1 = u1[i]
            key2 = u2[i]
            self.assert_ref(key1, key2, msg=seq)

        for i in range(len(seq)):
            key1 = keys1[i]
            key2 = keys2[i]
            self.assert_ref(key1, key2, msg=seq)

        del sorted_map, mapping
        for i in range(len(seq)):
            key1 = u1[i]
            key2 = u2[i]
            self.assert_ref(key1, key2, msg=seq)

        for i in range(len(seq)):
            key1 = keys1[i]
            key2 = keys2[i]
            self.assert_ref(key1, key2, msg=seq)

    def test_contains(self):
        seq = list(range(1024))
        keys1, keys2, sorted_map, mapping = self._build_v(seq)
        for i in range(len(seq)):
            key1 = keys1[i]
            key2 = keys2[i]
            self.assertTrue(key1 in sorted_map)
            self.assertTrue(key2 in mapping)
            self.assert_ref(key2, key1)

        for i in range(2056, 128):
            key = A(i)
            self.assertFalse(key in mapping)
            self.assertFalse(key in sorted_map)

        for i in range(len(seq)):
            key1 = keys1[i]
            key2 = keys2[i]
            self.assertTrue(key1 in sorted_map)
            self.assertTrue(key2 in mapping)
            self.assert_ref(key2, key1)

    def _test_iter(self, name):
        seq = list(range(1024))
        random.shuffle(seq)
        keys1, keys2, sorted_map, mapping = self._build_v(seq)
        it1 = list(getattr(sorted_map, name)())
        it2 = list(getattr(mapping, name)())
        s1 = sorted(it1)
        s2 = sorted(it2)

        self.assertEqual(len(keys1), len(it1))
        for i in range(len(it1)):
            self.assertEqual(s1[i], it1[i], msg=name)

        for i in range(len(keys1)):
            self.assert_ref(s2[i], s1[i], msg=name)

        del it1, it2, s1, s2
        for i in range(len(keys1)):
            self.assert_ref(keys2[i], keys1[i], msg=name)

    def test_keys(self):
        self._test_iter("keys")

    def test_values(self):
        self._test_iter("values")

    def test_items(self):
        self._test_iter("items")

    def test_iter(self):
        seq = list(range(1024))
        random.shuffle(seq)
        keys1, keys2, sorted_map, mapping = self._build_v(seq)
        for i in sorted_map:
            self.assertIn(i, keys1)
        del i
        s1 = sorted(keys1)
        s2 = sorted(keys2)
        for i in range(len(keys1)):
            k1 = s1[i]
            k2 = s2[i]
            self.assert_ref(k1, k2)

        del sorted_map, mapping
        for i in range(len(keys1)):
            k1 = s1[i]
            k2 = s2[i]
            self.assert_ref(k1, k2)

    def test_get(self):
        seq = list(range(1024))
        random.shuffle(seq)
        keys1, keys2, sorted_map, mapping = self._build_v(seq)
        self.assertEqual(len(mapping), len(sorted_map))
        for i in range(2096):
            if i < len(keys1):
                k1 = keys1[i]
                k2 = keys2[i]
            else:
                k1 = A(i)
                k2 = A(i)

            self.assertEqual(len(mapping), len(sorted_map))
            v1 = sorted_map.get(k1, k1)
            v2 = mapping.get(k2, k2)
            self.assertEqual(len(mapping), len(sorted_map))
            self.assertEqual(v2, v1)
            self.assert_ref(v2, v1)

        del sorted_map, mapping
        for i in range(len(seq)):
            key1 = keys1[i]
            key2 = keys2[i]
            self.assert_ref(key1, key2, msg=seq)

    def test_setdefault(self):
        seq = list(range(1024))
        random.shuffle(seq)
        keys1, keys2, sorted_map, mapping = self._build_v(seq)
        self.assertEqual(len(mapping), len(sorted_map))
        for i in range(2096):
            if i < len(keys1):
                k1 = keys1[i]
                k2 = keys2[i]
            else:
                k1 = A(i)
                k2 = A(i)

            self.assertEqual(len(mapping), len(sorted_map))
            v1 = sorted_map.setdefault(k1, k1)
            v2 = mapping.setdefault(k2, k2)
            self.assertEqual(len(mapping), len(sorted_map))
            self.assertEqual(v2, v1)
            self.assert_ref(v2, v1)

        for k in mapping.keys():
            self.assertTrue(k in sorted_map, msg=k)

        del sorted_map, mapping
        for i in range(len(seq)):
            key1 = keys1[i]
            key2 = keys2[i]
            self.assert_ref(key1, key2, msg=seq)

    def test_setnx(self):
        seq = list(range(1024))
        random.shuffle(seq)
        keys1, keys2, sorted_map, mapping = self._build_v(seq)
        self.assertEqual(len(mapping), len(sorted_map))
        for i in range(2096):
            if i < len(keys1):
                k1 = keys1[i]
                k2 = keys2[i]
            else:
                k1 = A(i)
                k2 = A(i)

            self.assertEqual(len(mapping), len(sorted_map))
            v1 = sorted_map.setnx(k1, lambda x: k1)
            v2 = mapping.setdefault(k2, k2)
            self.assertEqual(len(mapping), len(sorted_map))
            self.assertEqual(v2, v1)
            self.assert_ref(v2, v1)

        for k in mapping.keys():
            self.assertTrue(k in sorted_map, msg=k)

        del sorted_map, mapping
        for i in range(len(seq)):
            key1 = keys1[i]
            key2 = keys2[i]
            self.assert_ref(key1, key2, msg=seq)

    def test_update(self):
        seq = list(range(1024))
        random.shuffle(seq)
        keys1, keys2, sorted_map, mapping = self._build_v(seq)
        update1 = {}
        update2 = {}
        for i in range(2096):
            k1 = A(i)
            k2 = A(i)
            update1[k1] = k1
            update2[k2] = k2

        self.assertEqual(len(mapping), len(sorted_map))
        sorted_map.update(update1)
        mapping.update(update2)
        self.assertEqual(len(mapping), len(sorted_map))

        s1 = sorted(update1.keys())
        s2 = sorted(update2.keys())
        for i in range(len(s1)):
            k1 = s1[i]
            k2 = s2[i]
            v1 = sorted_map[k1]
            v2 = mapping[k2]
            self.assert_ref(v2, v1)

        del sorted_map, mapping
        for i in range(len(keys1)):
            k1 = keys1[i]
            k2 = keys2[i]
            self.assert_ref(k2, k1)

        for i in range(len(s1)):
            k1 = s1[i]
            k2 = s2[i]
            self.assert_ref(k2, k1)

    def test_clear(self):
        seq = list(range(1024))
        random.shuffle(seq)
        keys1, keys2, sorted_map, mapping = self._build_v(seq)

        self.assertEqual(len(mapping), len(sorted_map))
        sorted_map.clear()
        mapping.clear()
        self.assertEqual(len(mapping), len(sorted_map))

        for i in range(len(seq)):
            k1 = keys1[i]
            k2 = keys2[i]
            self.assert_ref(k2, k1)
            self.assertNotIn(k1, sorted_map)

        del sorted_map, mapping
        for i in range(len(keys1)):
            k1 = keys1[i]
            k2 = keys2[i]
            self.assert_ref(k2, k1)

    def _test_del(self, seq):
        keys1, keys2, sorted_map, mapping = self._build_v(seq)
        for i in range(len(keys1)):
            k1 = keys1[i]
            k2 = keys2[i]
            del mapping[k2]
            del sorted_map[k1]
            self.assert_ref(k2, k1)
            self.assertNotIn(k1, sorted_map)
            if i + 1 < len(keys1):
                self.assertIn(keys1[i + 1], sorted_map)

            self.assertEqual(len(mapping), len(sorted_map))

        del sorted_map, mapping
        for i in range(len(keys1)):
            k1 = keys1[i]
            k2 = keys2[i]
            self.assert_ref(k2, k1)

    def test_del(self):
        # fmt: off
        seq = [19, 35, 62, 2, 21, 61, 49, 50, 32, 63, 5, 26, 10, 13, 48, 46, 52, 1]
        # fmt: on
        self._test_del(seq)
        seq = list(range(1024))
        random.shuffle(seq)
        self._test_del(seq)

    def test_pop(self):
        seq = list(range(1024))
        random.shuffle(seq)
        keys1, keys2, sorted_map, mapping = self._build_v(seq)

        v1 = sorted_map.pop(A(65526), "")
        v2 = mapping.pop(A(65526), "")
        self.assertEqual(v2, v1)
        self.assertEqual(len(mapping), len(sorted_map))

        for i in range(len(keys1)):
            k1 = keys1[i]
            k2 = keys2[i]
            v1 = sorted_map.pop(k1, "")
            v2 = mapping.pop(k2, "")
            self.assert_ref(k2, k1)
            self.assertNotIn(k1, sorted_map)
            self.assertEqual(v2, v1, msg=(i, k1, k2))
            if i + 1 < len(keys1):
                self.assertIn(keys1[i + 1], sorted_map)
            self.assertEqual(len(mapping), len(sorted_map))

        del sorted_map, mapping
        for i in range(len(keys1)):
            k1 = keys1[i]
            k2 = keys2[i]
            self.assert_ref(k2, k1)

    def test_popitem(self):
        seq = list(range(1024))
        random.shuffle(seq)
        keys1, keys2, sorted_map, mapping = self._build_v(seq)

        s1 = min(keys1)
        s2 = min(keys2)

        k1, v1 = sorted_map.popitem()
        k2, v2 = s2, mapping[s2]
        del mapping[s2]

        self.assert_ref(k2, k1)
        self.assertEqual(s1, k1)
        self.assertNotIn(k1, sorted_map)

        del sorted_map, mapping
        for i in range(len(keys1)):
            k1 = keys1[i]
            k2 = keys2[i]
            self.assert_ref(k2, k1)

    def test_max_min(self):
        seq = list(range(1024))
        random.shuffle(seq)
        keys1, keys2, sorted_map, mapping = self._build_v(seq)

        s1 = min(keys1)
        s2 = min(keys2)

        k1, v1 = sorted_map.min()
        k2, v2 = s2, mapping[s2]
        self.assert_ref(k2, k1)
        self.assertEqual(s1, k1)
        self.assertIn(k1, sorted_map)

        s1 = max(keys1)
        s2 = max(keys2)

        k1, v1 = sorted_map.max()
        k2, v2 = s2, mapping[s2]

        self.assert_ref(k2, k1)
        self.assertEqual(s1, k1)
        self.assertIn(k1, sorted_map)

        del sorted_map, mapping
        for i in range(len(keys1)):
            k1 = keys1[i]
            k2 = keys2[i]
            self.assert_ref(k2, k1)

    def test_len(self):
        seq = list(range(1024))
        s = self._create_sorted_map()
        for i, v in enumerate(seq):
            s[v] = v
            self.assertEqual(i + 1, len(s))

        total = len(seq)
        for i, v in enumerate(seq):
            del s[v]
            self.assertEqual(total - i - 1, len(s))

    def test_sentinel(self):
        seq = list(range(1024))
        s = self._create_sorted_map()
        sentinel = _ctools.SortedMapSentinel
        ref_count = sys.getrefcount(sentinel)
        for i, v in enumerate(seq):
            s[v] = v
        for i, v in enumerate(seq):
            del s[v]
        self.assertEqual(ref_count, sys.getrefcount(sentinel))


if __name__ == "__main__":
    unittest.main()
