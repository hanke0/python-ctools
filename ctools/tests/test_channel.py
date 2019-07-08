from ctools.tests._bases import TestCase

import ctools


class TestChannel(TestCase):

    def test_size1(self):
        ch = ctools.Channel(1)

        self.assertTrue(ch.sendable())
        self.assertEqual(ch.size(), 1)
        self.assertFalse(ch.recvable())

        self.assertTrue(ch.send(1))
        self.assertFalse(ch.sendable())
        self.assertEqual(ch.size(), 1)
        self.assertTrue(ch.recvable())
        self.assertFalse(ch.send(2))

        a, ok = ch.recv()
        self.assertTrue(ok)
        self.assertEqual(a, 1)
        self.assertFalse(ch.recvable())
        self.assertTrue(ch.sendable())


if __name__ == '__main__':
    import unittest
    unittest.main()