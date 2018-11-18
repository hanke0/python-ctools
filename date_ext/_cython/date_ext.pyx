import datetime

def cy_interger_to_datetime(int date):
    return datetime.datetime(date // 10000, date % 10000 // 100, date % 100)