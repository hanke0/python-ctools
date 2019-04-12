#include <Python.h>

PyDoc_STRVAR(ctools__doc__, "Python C extension tools");

PyDoc_STRVAR(jump_consistent_hash__doc__, "jump_consistent_hash(key: int, num_buckets: int) -> int:\n\
\n\
    Generate a number in the range [0, num_buckets).\n\
\n\
    This function uses C bindings for speed.\n\
\n\
    :param key: The key to hash.\n\
    :type key: int\n\
    :param num_buckets: Number of buckets to use.\n\
    :type num_buckets: int\n\
    :return: hash number\n\
    :rtype: int\n");

PyDoc_STRVAR(strhash__doc__, "strhash(s) -> int:\n\
\n\
    hash str with consistent value.\n\
\n\
    This function uses C bindings for speed.\n\
\n\
    :param s: The string to hash.\n\
    :type s: string\n\
    :return: hash number\n\
    :rtype: int\n");

PyDoc_STRVAR(int8_to_datetime__doc__, "int8_to_datetime(date_integer) -> datetime.datetime:\n\
\n\
    Convert int like 20180101 to datetime.datetime(2018, 1, 1)).\n\
\n\
    This function uses C bindings for speed.\n\
\n\
    :param date_integer: The string to hash.\n\
    :type date_integer: int\n\
    :return: parsed datetime\n\
    :rtype: datetime.datetime\n");
