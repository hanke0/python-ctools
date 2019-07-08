========
CHANGELOG
========

0.1.0
=====
**New Feature**

* New class `ctools.CacheMap`, it's a lfu cache behaving much like dict.
* New class `TTLCache`, a cache mapping storing items for a certain time.
* `strhash` now add a method argument which supported fnv1a(default), fnv1, djb2 and murmur algorithms.

0.0.4
=====
**Improve**

* Decrease strhash collisions, but note that we will get different hash value in this version compare with old version.

0.0.3
=====
* First release.
