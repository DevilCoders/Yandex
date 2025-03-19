import datetime

def get_datetime_from_epoch(epoch):
    try:
        return str(datetime.datetime.fromtimestamp(int(epoch)))
    except:
        return None