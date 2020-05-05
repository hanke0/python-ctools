"""
Copyright (c) 2019 ko han

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
"""

__version__ = "0.2.0.dev0"

from ctools import _ctools

build_with_debug = _ctools.build_with_debug
strhash = _ctools.strhash
int8_to_datetime = _ctools.int8_to_datetime
jump_consistent_hash = _ctools.jump_consistent_hash

try:
    from collections.abc import MutableMapping  # noqa
except ImportError:
    from collections import MutableMapping  # noqa

CacheMap = _ctools.CacheMap
TTLCache = _ctools.TTLCache
Channel = _ctools.Channel
SortedMap = _ctools.SortedMap

try:
    MutableMapping.register(CacheMap)
    MutableMapping.register(TTLCache)
    MutableMapping.register(SortedMap)
except Exception:  # noqa
    pass

del _ctools, MutableMapping
