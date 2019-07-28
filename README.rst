============================================================
A collection of useful extensions for python implement in C.
============================================================
.. image:: https://travis-ci.org/ko-han/python-ctools.svg?branch=master
    :alt: Ctools build status on Travis CI
    :target: https://travis-ci.org/ko-han/python-ctools
.. image:: https://img.shields.io/github/license/ko-han/python-ctools
    :alt: license
    :target: https://github.com/ko-han/python-ctools/blob/master/LICENSE
.. image:: https://img.shields.io/pypi/v/ctools
    :alt: version
    :target: https://pypi.org/project/ctools/
.. image:: https://img.shields.io/pypi/implementation/ctools
    :alt: implementation
    :target: https://pypi.org/project/ctools/
.. image:: https://img.shields.io/pypi/pyversions/ctools
    :alt: python version
    :target: https://pypi.org/project/ctools/
.. image:: https://img.shields.io/pypi/wheel/ctools
    :alt: wheel
    :target: https://pypi.org/project/ctools/

Installation
============

Install and update using `pip`_:

.. code-block:: text

    pip install -U ctools

Example
=========

Cache things.

.. code-block:: python

    cache = ctools.CacheMap(2)
    cache['a'] = 'a'
    cache['b'] = 'b'
    a = cache['a']
    assert len(a) == 2
    cache['c'] = 'c'
    assert 'a' not in cache

    cache = ctools.TTLCache(10)
    cache['a'] = 'a'
    time.sleep(11)
    assert 'a' not in a

An implementation of golang chan like channel.

.. code-block::  python

    ch = ctools.Channel(1)
    ok = ch.send(1)
    assert ok
    assert not ch.send(2)
    d, ok = ch.recv()
    assert ok
    assert d == 1

Document
=========
`https://github.com/ko-han/python-ctools/wiki  <https://github.com/ko-han/python-ctools/wiki>`_.

Benchmark
=========
.. code-block:: text

    $ make benchmark
      int8_to_datetime,       63.481 ns ± 4.179 nseach (10 runs, 1,000,000 loops)
      jump_consistent_hash,   156.082 ns ± 6.489 nseach (10 runs, 1,000,000 loops)
      strhash,                139.542 ns ± 4.268 nseach (10 runs, 1,000,000 loops)


How To Test
===========
`pytest`_ needed. Simple use ``make test``. If package is installed, you can run ``python -c 'import ctools;ctools.test()'``
for testing.


More
====
What's important is free.

.. _pip: https://pip.pypa.io/en/stable/quickstart/
.. _jump_consistent_hash: https://arxiv.org/abs/1406.2294
.. _pytest: https://docs.pytest.org/en/latest/contents.html
