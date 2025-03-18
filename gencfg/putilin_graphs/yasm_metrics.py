import json
import os
import time
import logging
import sys

import requests

from core.utils import get_ch_formatted_date_from_timestamp, DAY
from core.query import run_query, get_roughly_last_event_date_from_table
from app import cache_manager
from newcachelib import SimpleRedisLease

FORMAT = "%(asctime)s:%(levelname)s:%(name)s:%(message)s"
logging.basicConfig(level=logging.DEBUG, format=FORMAT)


def run_query_single_int_or_default(query, default=-1):
    data = run_query(query)

    if data and data[0]:
        res = int(data[0][0])
    else:
        res = default

    return res


def get_last_timestamp(table):
    query = """
        SELECT max(ts) from {table}
        WHERE eventDate >= '{eventDate}'
    """.format(
        table=table,
        eventDate=(
            get_roughly_last_event_date_from_table(table, days_to_watch=2)
            or get_ch_formatted_date_from_timestamp(time.time() - 2 * DAY)
        )
    )

    return run_query_single_int_or_default(query)


def get_number_of_last_records(table='instanceusage', minutes_in_the_past_from=10, minutes_in_the_past_to=5):
    now = time.time()
    from_ts = now - minutes_in_the_past_from * 60
    to_ts = now - minutes_in_the_past_to * 60
    query = """
        SELECT count() from {table}
        WHERE eventDate >= '{eventDate}' AND ts >= {from_ts} AND ts <= {to_ts}
    """.format(
        table=table,
        eventDate=get_ch_formatted_date_from_timestamp(from_ts),
        from_ts=from_ts,
        to_ts=to_ts
    )

    return run_query_single_int_or_default(query)


def get_yasm_signals(ttl):
    last_records = get_number_of_last_records()

    yasm_signals = [{
        "name": "gencfggraphs_instanceusage_record_count_last_5m_max",
        "ttl": ttl,
        "val": last_records
    }]

    for table_suffix in ["2m", "15m", "1h", "1d"]:
        last_ts = get_last_timestamp('instanceusage_aggregated_{}'.format(table_suffix))

        yasm_signal = {
            "name": "gencfggraphs_instanceusage_aggregated_{}_lag_max".format(table_suffix),
            "ttl": ttl,
            "val": int(time.time()) - last_ts
        }
        yasm_signals.append(yasm_signal)

    return yasm_signals


def main():
    instance_id = "{}:{}".format(os.environ['BSCONFIG_IHOST'], os.environ['BSCONFIG_IPORT'])
    ttl = int(sys.argv[1])

    redis_yasm_metrics_worker_instance_key = "chgraphs/yasm_metrics_worker"

    while True:
        master = cache_manager.get_redis_master()
        with SimpleRedisLease(master, redis_yasm_metrics_worker_instance_key, ttl / 2, instance_id):
            logging.info("Getting signals")
            yasm_signals = get_yasm_signals(ttl)
            logging.info("Sending signals")
            try:
                r = requests.post('http://localhost:11005/', data=json.dumps(yasm_signals))
                r.raise_for_status()
            except requests.RequestException:
                logging.exception("While sending data to yasm")

        sleep_time = ttl / 4
        logging.info("Sleeping for %s seconds", sleep_time)
        time.sleep(sleep_time)


if __name__ == "__main__":
    logging.warning("Starting yasm_metrics")
    main()
