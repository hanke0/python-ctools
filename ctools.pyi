from datetime import datetime
from typing import Any, Mapping, Iterable, Tuple

def jump_consistent_hash(key: int, num_bucket: int) -> int: pass

def strhash(s: str) -> int: ...

def int8_to_datetime(date_integer: int) -> datetime: ...


class LFUCache:

    def __init__(self, capacity: int) -> None: ...

    def get(self, key, default=None) -> Any: ...

    def pop(self, key, default=None) -> Any: ...

    def setdefault(self, key, default=None) -> Any: ...

    def update(self, map: Mapping, **kwargs) -> None: ...

    def keys(self) -> Iterable: ...

    def values(self) -> Iterable: ...

    def items(self) -> Iterable[Tuple]: ...

    def clear(self): ...

    def __contains__(self, *args, **kwargs): # real signature unknown
        """ True if the dictionary has the specified key, else False. """
        pass

    def __delitem__(self, *args, **kwargs): # real signature unknown
        """ Delete self[key]. """
        pass

    def __eq__(self, *args, **kwargs): # real signature unknown
        """ Return self==value. """
        pass

    def __getattribute__(self, *args, **kwargs): # real signature unknown
        """ Return getattr(self, name). """
        pass

    def __getitem__(self, y): # real signature unknown; restored from __doc__
        """ x.__getitem__(y) <==> x[y] """
        pass

    def __ge__(self, *args, **kwargs): # real signature unknown
        """ Return self>=value. """
        pass

    def __gt__(self, *args, **kwargs): # real signature unknown
        """ Return self>value. """
        pass

    def __iter__(self, *args, **kwargs): # real signature unknown
        """ Implement iter(self). """
        pass

    def __len__(self, *args, **kwargs): # real signature unknown
        """ Return len(self). """
        pass

    def __le__(self, *args, **kwargs): # real signature unknown
        """ Return self<=value. """
        pass

    def __lt__(self, *args, **kwargs): # real signature unknown
        """ Return self<value. """
        pass

    def evict(self) -> None: ...

    def set_capacity(self, capacity: int) -> None: ...

    def hints(self) -> (int, int, int): ...

    def lfu(self) -> Any: ...

    def lfu_of(self, key: Any) -> int: ...
