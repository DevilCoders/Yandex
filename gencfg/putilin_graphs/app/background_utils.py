import time

from core.query import run_query
from core.utils import get_ch_formatted_date_from_timestamp, WEEK

# Lower number means higher priority
BACKGROUND_PRIORITY = 10


def get_all_groups(ed1, ed2):
    query = """
        SELECT DISTINCT group FROM instanceusage_aggregated_1h
        WHERE eventDate >= '{ed1}' AND eventDate <= '{ed2}'
    """.format(ed1=ed1, ed2=ed2)
    return [row[0] for row in run_query(query) if row[0]]


def get_last_week_groups():
    end_ts = int(time.time())
    start_ts = end_ts - WEEK

    start_date = get_ch_formatted_date_from_timestamp(start_ts)
    end_date = get_ch_formatted_date_from_timestamp(end_ts)

    return get_all_groups(start_date, end_date)
