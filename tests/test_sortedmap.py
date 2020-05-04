import unittest
import sys
import random
import ctools


class A:
    def __init__(self, a):
        self.a = a

    def __lt__(self, other):
        return self.a < other.a

    def __eq__(self, other):
        return self.a == other.a

    def __hash__(self):
        return id(self)

    def __repr__(self):
        return str(self.a)

    def __str__(self):
        return self.__repr__()


class TestSortedMap(unittest.TestCase):
    def assert_ref(self, o1, o2, msg=None):
        self.assertEqual(sys.getrefcount(o1), sys.getrefcount(o2), msg=msg)

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
        for i in range(len(seq)):
            key1 = keys1[i]
            key2 = keys2[i]
            self.assert_ref(key1, key2, msg=seq)

        for i in range(len(seq)):
            key1 = keys1[i]
            key2 = keys2[i]
            self.assertEqual(mapping[key2], sorted_map[key1])
            self.assert_ref(key1, key2, msg=seq)

    def test_setgetitem(self):
        self._test_setgetitem_list(list(range(1024)))
        self._test_setgetitem_list(list(reversed(list(range(1024)))))
        v = list(range(1024))
        random.shuffle(v)
        self._test_setgetitem_list(v)

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


if __name__ == "__main__":
    unittest.main()
