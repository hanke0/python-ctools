import datetime


def py_integer_to_datetime(date):
    return datetime.datetime(date // 10000, date % 10000 // 100, date % 100)
