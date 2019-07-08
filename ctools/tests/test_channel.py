import uuid

import ctools
from ctools.tests._bases import TestCase



class TestChannel(TestCase):

    def test_size1(self):
        ch = ctools.Channel(1)

        item = uuid.uuid1()
        self.assertTrue(ch.sendable())
        self.assertEqual(ch.size(), 1)
        self.assertFalse(ch.recvable())

        self.assertTrue(ch.send(item))
        self.assertFalse(ch.sendable())
        self.assertEqual(ch.size(), 1)
        self.assertTrue(ch.recvable())
        self.assertFalse(ch.send(uuid.uuid1()))

        a, ok = ch.recv()
        s = d = uuid.uuid1()
        self.assertTrue(ok)
        self.assertEqual(a, item)
        self.assertFalse(ch.recvable())
        self.assertTrue(ch.sendable())
        self.assertRefEqual(item, s)

    def test_size32(self):
        ch = ctools.Channel(32)

        self.assertTrue(ch.sendable())
        self.assertFalse(ch.recvable())

        for i in range(32):
           self.assertTrue(ch.send(i))
        self.assertFalse(ch.sendable())
        self.assertTrue(ch.recvable())
        self.assertFalse(ch.send(2))

        for i in range(32):
            a, ok = ch.recv()
            self.assertTrue(ok)
            self.assertEqual(a, i)
        self.assertFalse(ch.recvable())
        self.assertTrue(ch.sendable())

    def test_size31(self):
        ch = ctools.Channel(31)

        self.assertTrue(ch.sendable())
        self.assertFalse(ch.recvable())

        for i in range(31):
            self.assertTrue(ch.send(i))
        self.assertFalse(ch.sendable())
        self.assertTrue(ch.recvable())
        self.assertFalse(ch.send(2))

        for i in range(31):
            a, ok = ch.recv()
            self.assertTrue(ok)
            self.assertEqual(a, i)
        self.assertFalse(ch.recvable())
        self.assertTrue(ch.sendable())

    def test_safe_consume(self):
        ch = ctools.Channel(1)
        ev = uuid.uuid1()
        ch.send(ev)
        a = uuid.uuid1()

        def f(item):
            self.assertEqual(item, ev)
            raise ValueError(item)

        with self.assertRaises(ValueError):
            ch.safe_consume(f)

        self.assertTrue(ch.recvable())
        self.assertFalse(ch.sendable())
        self.assertEqual(ch.recv()[0], ev)
        self.assertRefEqual(ev, a)

        def g(item):
            return False

        ch.send(ev)
        self.assertFalse(ch.safe_consume(g))
        self.assertTrue(ch.recvable())
        self.assertFalse(ch.sendable())
        self.assertEqual(ch.recv()[0], ev)
        self.assertRefEqual(ev, a)

        def s(item):
            return item

        ch.send(ev)
        self.assertEqual(ch.safe_consume(s), ev)
        self.assertFalse(ch.recvable())
        self.assertTrue(ch.sendable())
        self.assertIsNone(ch.recv()[0])
        self.assertRefEqual(ev, a)



if __name__ == '__main__':
    import unittest
    unittest.main()