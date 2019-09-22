.. currentmodule:: ctools

0.1.0
-----
**New Feature**

* New class :class:`CacheMap`, it's a lfu cache behaving much like dict.
* New class :class:`TTLCache`, a cache mapping storing items for a certain time.
* :func:`strhash` now add a method argument which supported fnv1a(default), fnv1, djb2 and murmur algorithms.

0.0.4
-----
**Improve**

* Decrease :func:`strhash` collisions, but note that we will get different hash value in this version compare with old version.

0.0.3
-----
* First release.
