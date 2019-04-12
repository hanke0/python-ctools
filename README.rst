A collection of useful functions for python implement in C.
===========================================================

Installation
============

Install and update using `pip`_:

.. code-block:: text

    pip install -U ctools

API
===

* A consistent hash implement. See paper at `jump_consistent_hash`_.

.. code-block:: text

    jump_consistent_hash(key: int, num_buckets: int) -> int:
        """Generate a number in the range [0, num_buckets).

        This function uses C bindings for speed.

        :param key: The key to hash.
        :type key: int
        :param num_buckets: Number of buckets to use.
        :type num_buckets: int
        :return: hash number
        :rtype: int
        """

* A normal hash function for str with consistent value.

.. code-block:: text

    strhash(s: str) -> int:
        """
        hash str with consistent value.

        This function uses C bindings for speed.

        :param s: The string to hash.
        :type s: string
        :return: hash number
        :rtype: int
        """

* Transfer integer like 20170101 (Java style) to python datetime object.

.. code-block:: text

    int8_to_datetime(date_integer: int) -> datetime.datetime:
        """
        Convert int like 20180101 to datetime.datetime(2018, 1, 1)).

        This function uses C bindings for speed.

        :param date_integer: The string to hash.
        :type date_integer: int
        :return: parsed datetime
        :rtype: datetime.datetime
        """

.. _pip: https://pip.pypa.io/en/stable/quickstart/
.. _jump_consistent_hash: https://arxiv.org/abs/1406.2294
