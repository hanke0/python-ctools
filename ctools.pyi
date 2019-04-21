from datetime import datetime
from typing import Any, Mapping, Iterable, Tuple, Callable, Optional

def jump_consistent_hash(key: int, num_bucket: int) -> int: pass

def strhash(s: str) -> int: ...

def int8_to_datetime(date_integer: int) -> datetime: ...

dict.setdefault()

class LFUCache:

    def __init__(self, capacity: int) -> None: ...

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

    def evict(self) -> None: ...

    def set_capacity(self, capacity: int) -> None: ...

    def hints(self) -> (int, int, int): ...

    def lfu(self) -> Any: ...

    def setnx(self, key, callback: Callable[[], Any]):
        """
        Insert key with a value of callback() if key is not in the dictionary.

        Return the value for key if key is in the dictionary, else callback().
        """
        pass
