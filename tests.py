import unittest
from datetime import datetime

import ctools


class T(unittest.TestCase):

    def test_int8_to_datetime(self):
        """
        :param:
        :type
        :return:
        :rtype
        :raises
        """
        date = ctools.int8_to_datetime(20170101)
        self.assertEqual(date, datetime(2017, 1, 1))

        with self.assertRaises(ValueError):
            ctools.int8_to_datetime(1)


if __name__ == '__main__':
    unittest.main()
