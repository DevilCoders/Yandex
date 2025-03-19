#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""Visualize backups steps"""

import argparse
from pprint import pprint
from datetime import timedelta, datetime
import dotmap
from s3_rotate import BackupItem, BackupRotate, load_config

TPL = "test-s3://music/backup/mysql/test-base/%F"

def main():
    """Helper for view rotate process"""

    parser = argparse.ArgumentParser(description='Visualize "rotate-s3-backups"')
    parser.add_argument(
        '--start', metavar='YYYMMDD',
        help="Start from time (default 20170903)", default="20170903"
    )
    parser.add_argument(
        '--che', metavar='YYYYMMDD', nargs="+", help="Time check points", required=True
    )

    parser.add_argument(
        '-p', '--period', help="Backup period in days (def=365 days)", default=365, type=int
    )
    parser.add_argument('-d', '--daily', help="Policy daily (def=5)", default=5, type=int)
    parser.add_argument('-w', '--weekly', help="Policy weekly (def=1)", default=1, type=int)
    parser.add_argument('-m', '--monthly', help="Policy monthly (def=1)", default=1, type=int)
    args = parser.parse_args()

    valid_backups = []
    time_start = datetime.strptime(args.start, "%Y%m%d")
    che_times = []
    for che_time in args.che:
        che_times.append(datetime.strptime(che_time, "%Y%m%d"))
    print("Start time", time_start.strftime("%F"))
    print("Time check points:", [x.strftime("%F") for x in che_times])

    conf_args = dotmap.DotMap(check="mysql", bucket="music", debug=False)
    worker = BackupRotate(load_config(conf_args))
    worker.conf.save_from_last = True

    for itr in range(args.period):
        current_date = BackupItem(path=(time_start + timedelta(days=itr)).strftime(TPL))
        valid_backups.append(current_date)
        worker.backups_list = valid_backups

        backup_groups = worker.group_by_frequency()
        policy = dotmap.DotMap({
            "daily": args.daily,
            "weekly": args.weekly,
            "monthly": args.monthly
        })
        worker.select_valid(backup_groups, policy)

        preserve_backups = []
        for bkp in valid_backups:
            if not bkp.valid:
                continue
            preserve_backups.append(BackupItem(path=bkp.timestamp.strftime(TPL)))
        valid_backups = preserve_backups
        if current_date.timestamp in che_times:
            print("Backups for", current_date.timestamp.strftime("%F"))
            pprint([x.timestamp.strftime("%F") for x in valid_backups])

if __name__ == "__main__":
    main()
