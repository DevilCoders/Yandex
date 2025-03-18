#!/usr/bin/env python

import logging
FORMAT = "%(asctime)s:%(levelname)s:%(name)s:%(message)s"
logging.basicConfig(level=logging.DEBUG, format=FORMAT)

import sys
import time
import random

from app import cache_manager, abc, hosts_anomalies, total_alloc


def main():
    cache_manager.enable_global_forced_refresh_mode()

    period_recalc = int(sys.argv[1])
    n_hosts = int(sys.argv[2])

    while True:
        sleep_time = period_recalc * (random.random()) * n_hosts
        logging.info("Sleeping for %s seconds", sleep_time)
        time.sleep(sleep_time)

        logging.info("Before updating total_alloc")
        total_alloc.get_gencfg_stuff.refresh_cache()
        # total_alloc.get_total_alloc_and_usage_data.refresh_cache()

        logging.info("Before updating abc data")
        abc.get_cached_abc_data.refresh_cache()
        logging.info("Before updating pgh data")
        abc.get_cached_pgh_data.refresh_cache()

        logging.info("Before finding hosts anomalies")
        hosts_anomalies.get_all_anomalies.refresh_cache()


if __name__ == "__main__":
    logging.warning("Starting background_jobs")
    main()
