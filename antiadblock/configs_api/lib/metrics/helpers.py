import datetime
from time import mktime


def percentage(part, whole):
    """
    Calculate percentage of two floats
    :return: percentage as integer
    """
    if part == 0:
        return 0
    # unnormal situation when we have part only. So we use 110% to notify user that something went wrong
    if whole == 0:
        return 110
    return int(100 * float(part) / float(whole))


def date_params_from_range(date_range):
    """
    :param date_range: date_range in minutes
    :return: two params: timestamp in millis with date_range time shifting and current time group_by param for es query
    """
    from_date_ts = timestamp_millis_with_shifting(date_range)
    if 1 <= date_range <= 60:
        group_by = '1m'
    elif 61 <= date_range <= 720:  # 1-11 hours
        group_by = '5m'
    elif 721 <= date_range <= 1080:  # 12-18 hours
        group_by = '10m'
    elif 1081 <= date_range <= 1440:  # 18-24 hours
        group_by = '30m'
    else:
        group_by = '60m'

    return from_date_ts, group_by


def timestamp_millis_with_shifting(minutes):
    """
    Helping method to calculate timestamp shifted to `minutes`
    :return: unit timestamp in milliseconds format (for elasticsearch requests)
    """
    return int(mktime((datetime.datetime.now() - datetime.timedelta(minutes=minutes)).timetuple())) * 1000
