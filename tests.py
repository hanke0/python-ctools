import unittest
import random
import string
import os
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

    def test_lfu(self):
        d = ctools.LFU(2)
        d['a'] = 1
        d['c'] = 2
        d['e'] = 3
        self.assertEqual(len(d), 2)

        with open(os.devnull, "w") as null:
            for i in range(1024):
                self.assertEqual(d['c'], 2)

        self.assertEqual(d.should_removed(), "e")

        for i in range(0xffffff00, 0xffffffff):
            d[i] = random.random()

        self.assertEqual(len(d), 2)


if __name__ == '__main__':
    unittest.main()
