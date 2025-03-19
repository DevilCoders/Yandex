#!/usr/bin/env python2
#
# Provides: walle_reboots
#

import subprocess
import glob
import sys
import json
from datetime import date, datetime, timedelta

REBOOTS_LIMIT = 50  # select value slightly greather than median across problematic hosts
REBOOTS_PERIOD = 7  # in days

parse_date = lambda parts: datetime.strptime(
    "%s %s %s %d" % tuple(parts), "%a %b %d %Y").date()

now = date.today()


def die(exit_num, exit_str):
    print("PASSIVE-CHECK:walle_reboots;%d;%s" % (exit_num, exit_str))
    sys.exit(0)


def parse_last_logs():
    log_lines = []

    list_of_files = ['/var/log/wtmp'] + glob.glob("/var/log/wtmp.?")
    for log in list_of_files:
        stdout = subprocess.check_output(["last", "-x", "-f", log])
        for line in stdout.splitlines():
            if line.startswith(("reboot", "shutdown")):
                log_lines.append(line.strip())
    return log_lines


def find_reboot_dates():
    current_year = now.year
    prev_date = None
    reboot_log = []
    for line in parse_last_logs():
        parts = filter(None, line.split(" "))
        if parts[0] == "reboot":
            current_date = parse_date(parts[4:7] + [current_year])
            if prev_date is not None and current_date > prev_date:
                current_year -= 1
                current_date = parse_date(parts[4:7] + [current_year])
            reboot_log.append(current_date)
            prev_date = current_date
        elif parts[0] == "shutdown" and reboot_log:
            reboot_log.pop()
    return reboot_log


def main():
    day_dt = now - timedelta(days=REBOOTS_PERIOD)

    reboot_count = 0
    for reboot_dt in find_reboot_dates():
        if reboot_dt >= day_dt:
            reboot_count += 1

    status = 0
    if reboot_count >= REBOOTS_LIMIT:
        status = 2

    result = {'result': {'count': reboot_count}}
    die(status, json.dumps(result))


if __name__ == '__main__':
    main()
