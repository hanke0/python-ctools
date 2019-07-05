# Copyright 2019 ko-han. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from datetime import datetime
from typing import Any, Mapping, Iterable, Tuple, Callable, Optional

##-----start-----##

#- api
#- description: A consistent hash implement. See paper at [jump_consistent_hash](https://arxiv.org/abs/1406.2294).
def jump_consistent_hash(key: int, num_bucket: int) -> int: pass

#- api
#- description: A normal hash function for str with consistent value.
def strhash(s: str, method: str = 'fnv1a') -> int:
    """
    Change Log: start from 0.0.4, support method arguments.Allowed method are fnv1a(default), fnv1, djb2 and murmur.
    """
    pass

#- api
#- description: Transfer integer like 20170101 (Java style) to python datetime object.
def int8_to_datetime(date_integer: int) -> datetime: ...

#- api
#- description: A cache Map behaving like dict
class CacheMap:
    """New in version 0.1.0"""
    def __init__(self, capacity: int) -> None:
        """
        :param capacity: max items cache would hold.
        """
        pass

    def get(self, key, default=None):
        """ Return the value for key if key is in the cache, else default. """
        pass

    def pop(self, key, default=None): # real signature unknown; restored from __doc__
        """
        Cache.pop(k[,default]) -> v, remove specified key and return the corresponding value.
        If key is not found, default is returned
        """
        pass

    def setdefault(self, *args, **kwargs): # real signature unknown
        """
        Insert key with a value of default if key is not in the dictionary.

        Return the value for key if key is in the dictionary, else default.
        """
        pass

    def update(self, mp: Optional[Mapping] = None, **kwargs): # known special case of dict.update
        """
        Cache.update([mp, ]**kwargs) -> None.  Update Cache from dict/iterable mp and kwargs.
        If mp is present then does:  for k in mp: Cache[k] = mp[k]
        In either case, this is followed by: for k in kwargs:  Cache[k] = kwargs[k]
        """
        pass

    def keys(self) -> Iterable:
        """Return key list."""
        pass

    def values(self) -> Iterable:
        """Return value list."""
        pass

    def items(self) -> Iterable[Tuple]:
        """Return (k, v) pairs list."""
        pass

    def clear(self):
        """ Cache.clear() -> None.  Remove all items from Cache. """
        pass

    def __contains__(self, key): # real signature unknown
        """ True if the dictionary has the specified key, else False. """
        pass

    def __delitem__(self, key): # real signature unknown
        """ Delete self[key]. """
        pass

    def __setitem__(self, key, value): # real signature unknown
        """ self[key] = value. """
        pass

    def __getitem__(self, key): # real signature unknown; restored from __doc__
        """ Return self[key] """
        pass

    def __len__(self, *args, **kwargs): # real signature unknown
        """ Return len(self). """
        pass

    def evict(self) -> None:
        """evict a item
        """
        ...

    def set_capacity(self, capacity: int) -> None: ...

    def hints(self) -> (int, int, int):
        """cache statics information.

        :return: capacity, hits, misses
        """

    def next_evict_key(self) -> Any: ...

    def setnx(self, key, callback: Callable[[], Any]):
        """
        Insert key ith a value of callback() if key is not in the dictionary.

        Return the value for key if key is in the dictionary, else callback().
        """
        pass


#- api
#- description: A cache map stores items in certain seconds and behaves much like dict.
class TTLCache:
    """New in version 0.1.0"""
    def __init__(self, ttl: int) -> None:
        """
        :param ttl: max seconds a items stores.
        """
        pass

    def get(self, key, default=None):
        """ Return the value for key if key is in the cache, else default. """
        pass

    def pop(self, key, default=None): # real signature unknown; restored from __doc__
        """
        Cache.pop(k[,default]) -> v, remove specified key and return the corresponding value.
        If key is not found, default is returned
        """
        pass

    def setdefault(self, *args, **kwargs): # real signature unknown
        """
        Insert key with a value of default if key is not in the dictionary.

        Return the value for key if key is in the dictionary, else default.
        """
        pass

    def update(self, mp: Optional[Mapping] = None, **kwargs): # known special case of dict.update
        """
        Cache.update([mp, ]**kwargs) -> None.  Update Cache from dict/iterable mp and kwargs.
        If mp is present then does:  for k in mp: Cache[k] = mp[k]
        In either case, this is followed by: for k in kwargs:  Cache[k] = kwargs[k]
        """
        pass

    def keys(self) -> Iterable:
        """Return key list."""
        pass

    def values(self) -> Iterable:
        """Return value list."""
        pass

    def items(self) -> Iterable[Tuple]:
        """Return (k, v) pairs list."""
        pass

    def clear(self):
        """ Cache.clear() -> None.  Remove all items from Cache. """
        pass

    def __contains__(self, key): # real signature unknown
        """ True if the dictionary has the specified key, else False. """
        pass

    def __delitem__(self, key): # real signature unknown
        """ Delete self[key]. """
        pass

    def __setitem__(self, key, value): # real signature unknown
        """ self[key] = value. """
        pass

    def __getitem__(self, key): # real signature unknown; restored from __doc__
        """ Return self[key] """
        pass

    def __len__(self, *args, **kwargs): # real signature unknown
        """ Return len(self). """
        pass

    def set_default_ttl(self, ttl: int) -> None:
        """reset default ttl"""
        ...

    def get_default_ttl(self) -> int:
        """Return default ttl"""
        pass

    def setnx(self, key, callback: Callable[[], Any]):
        """
        Insert key ith a value of callback() if key is not in the dictionary.

        Return the value for key if key is in the dictionary, else callback().
        """
        pass