============================================================
A collection of useful extensions for python implement in C.
============================================================

:Python: >= 3.4
:Version: 0.0.4

Installation
============

Install and update using `pip`_ (python3 only):

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


Benchmark
=========
.. code-block:: text

    $ make install && make benchmark
      int8_to_datetime,       63.481 ns ± 4.179 nseach (10 runs, 1,000,000 loops)
      jump_consistent_hash,   156.082 ns ± 6.489 nseach (10 runs, 1,000,000 loops)
      strhash,                139.542 ns ± 4.268 nseach (10 runs, 1,000,000 loops)



How To Test
===========
.. code-block:: text

    $ make install && make test


More
====
.. code-block:: text

    $ make help


What's important is free.

.. _pip: https://pip.pypa.io/en/stable/quickstart/
.. _jump_consistent_hash: https://arxiv.org/abs/1406.2294
