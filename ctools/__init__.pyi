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

from datetime import datetime
from typing import Any, Mapping, Iterable, Tuple, Callable, Optional

__version__: str


def jump_consistent_hash(key: int, num_bucket: int) -> int: ...


def strhash(s: str, method: str = 'fnv1a') -> int: ...


def int8_to_datetime(date_integer: int) -> datetime: ...


MAX_INT32 = 1 << 31 - 1


class CacheMap:
    def __init__(self, capacity: int = MAX_INT32) -> None: ...

    def __getitem__(self, item): ...

    def __setitem__(self, key, value): ...

    def __delitem__(self, key): ...

    def __contains__(self, item): ...

    def __len__(self): ...

    def get(self, key, default=None): ...

    def pop(self, key, default=None): ...

    def popitem(self) -> Tuple[Any, Any]: ...

    def setdefault(self, *args, **kwargs): ...

    def update(self, mp: Optional[Mapping] = None, **kwargs) -> None: ...

    def keys(self) -> Iterable: ...

    def values(self) -> Iterable: ...

    def items(self) -> Iterable[Tuple]: ...

    def clear(self): ...

    def evict(self) -> None: ...

    def set_capacity(self, capacity: int) -> None: ...

    def hints(self) -> (int, int, int): ...

    def next_evict_key(self) -> Any: ...

    def setnx(self, key, fn: Callable[[], Any]): ...


class TTLCache:
    def __init__(self, ttl: int = MAX_INT32) -> None: ...

    def __getitem__(self, item): ...

    def __setitem__(self, key, value): ...

    def __delitem__(self, key): ...

    def __contains__(self, item): ...

    def __len__(self): ...

    def get(self, key, default=None): ...

    def pop(self, key, default=None): ...

    def popitem(self) -> Tuple[Any, Any]: ...

    def setdefault(self, key, default=None): ...

    def update(self, mp: Optional[Mapping] = None) -> None: ...

    def keys(self) -> Iterable: ...

    def values(self) -> Iterable: ...

    def items(self) -> Iterable[Tuple]: ...

    def clear(self): ...

    def set_default_ttl(self, ttl: int) -> None: ...

    def get_default_ttl(self) -> int: ...

    def setnx(self, key, fn: Callable[[], Any]): ...


class Channel:
    def __init__(self, size: int = MAX_INT32) -> None: ...

    def clear(self) -> None: ...

    def close(self, send: bool = True, recv: bool = True) -> None: ...

    def recv(self) -> Tuple[Any, bool]: ...

    def recvable(self) -> bool: ...

    def safe_consume(self, fn) -> bool: ...

    def send(self, o: Any) -> bool: ...

    def sendable(self) -> bool: ...

    def size(self) -> int: ...
