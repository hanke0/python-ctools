============================================================
A collection of useful extensions for python implement in C.
============================================================

:Python: >= 3.5
:Version: 0.1.0
:Latest stable: 0.1.0
:Python Implementation: CPython only

.. contents::

Installation
============

Install and update using `pip`_ (python3 only):

.. code-block:: text

    pip install -U ctools

Documents
=========

Latest develop document at `api.md <https://github.com/ko-han/python-ctools/blob/master/doc/api.md>`_.

See wiki for `stable release document  <https://github.com/ko-han/python-ctools/wiki>`_.

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