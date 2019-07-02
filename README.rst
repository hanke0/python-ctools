============================================================
A collection of useful extensions for python implement in C.
============================================================

:Python: >= 3.5
:Version: 0.0.5
:Python Implementation: CPython only

.. contents::

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
        :param num_buckets: Number of buckets to use.
        :return: hash number
        """

* A normal hash function for str with consistent value.

.. code-block:: text

    strhash(s: str) -> int:
        """
        hash str with consistent value.

        This function uses C bindings for speed.

        :param s: The string to hash.
        :return: hash number
        """

* Transfer integer like 20170101 (Java style) to python datetime object.

.. code-block:: text

    int8_to_datetime(date_integer: int) -> datetime.datetime:
        """
        Convert int like 20180101 to datetime.datetime(2018, 1, 1)).

        This function uses C bindings for speed.

        :param date_integer: The string to hash.
        :return: parsed datetime
        """

See more at `api.md <https://github.com/ko-han/python-ctools/blob/master/doc/api.md>` -.

Benchmark
=========
.. code-block:: text

    $ make benchmark
      int8_to_datetime,       63.481 ns ± 4.179 nseach (10 runs, 1,000,000 loops)
      jump_consistent_hash,   156.082 ns ± 6.489 nseach (10 runs, 1,000,000 loops)
      strhash,                139.542 ns ± 4.268 nseach (10 runs, 1,000,000 loops)


How To Test
===========
`pytest`_ needed. Simple use ``make test``. If package is installed, you can run ``ctools.test()``
for testing.


More
====
What's important is free.

.. _pip: https://pip.pypa.io/en/stable/quickstart/
.. _jump_consistent_hash: https://arxiv.org/abs/1406.2294
.. _pytest: https://docs.pytest.org/en/latest/contents.html