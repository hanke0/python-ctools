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


**CTools is a collection of useful data structures and functions written in C for Python.**

* Author: ko-han
* Documents: https://python-ctools.readthedocs.io


Install:

    Install and update using `pip`_:  ``pip install -U ctools``

It provides:

    * Jump consistent hash. Find description `here <https://arxiv.org/abs/1406.2294>`_ .
    * `fnv1a`_, fnv1, djb2, and `murmur`_ string hash method.
    * A `LFU`_ (least frequently used) cache mapping class.
    * A TTL cache mapping class that all key expire after specific seconds.
    * A channel class support sending and receiving objects.
    * A SortedMap class based on `red-black tree`_ .


How To Test:

    Simple, type ``make test`` and take a sip of coffee.
    Also can use ``make benchmark`` to run the benchmark test.


More:

    What's important is free.


.. _pip: https://pip.pypa.io/en/stable/quickstart/
.. _jump_consistent_hash: https://arxiv.org/abs/1406.2294
.. _pytest: https://docs.pytest.org/en/latest/contents.html
.. _LFU: https://en.wikipedia.org/wiki/Least_frequently_used
.. _red-black tree: https://en.wikipedia.org/wiki/Red%E2%80%93black_tree
.. _fnv1a: https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
.. _murmur: https://en.wikipedia.org/wiki/MurmurHash