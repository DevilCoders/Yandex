#!/usr/bin/env python2

import argparse
import os
import re
import time

CRASH_DIR = "/var/lib/systemd/coredump"
CRASH_RE = re.compile("^core.(?P<process>[0-9a-zA-Z_\.\-]+)\.(?P<pid>[0-9]+)\.[a-z0-9\-\.]+\.[0-9]+\.xz$")
INTERVAL = 12 * 30 * 60

def parse_coredump_name(dump_filename):
    re_result = CRASH_RE.match(dump_filename)
    if not re_result:
        return None
    return (re_result.group(attr) for attr in ("process", "pid"))

def scan(crashes_dir, newer_than):
    for entry in os.listdir(crashes_dir):
        path = os.path.join(crashes_dir, entry)
        if os.path.isfile(path) and os.stat(path).st_ctime > newer_than:
            parsed = parse_coredump_name(entry)
            if parsed is None:
                continue
            yield parsed

def report_state(dumps):
    check_status = 0
    check_message = "OK"
    if len(dumps) > 0:
        check_status = 2
        dumped_procs = ", ".join("{} ({})".format(*d) for d in dumps)
        check_message = "{} fresh dump(s): {}".format(len(dumps), dumped_procs)
    return format_output(check_status, check_message)

def format_output(status, message):
    check_type = "PASSIVE-CHECK"
    check_name = "coredumps"
    return "{}:{};{};{}".format(check_type, check_name, status, message)

def parse():
    parser = argparse.ArgumentParser()
    parser.add_argument("--path", required=False, default=CRASH_DIR, help="Path to crash dir")
    parser.add_argument("--interval", required=False, type=int, default=INTERVAL, help="Scan interval in sec")
    return parser.parse_args()

def main():
    args = parse()
    if not os.path.isdir(args.path):
        print(format_output(1, "No such dir {!r}".format(args.path)))
        return
    newer_than = time.time() - args.interval
    dumps = [dump for dump in scan(args.path, newer_than)]
    print(report_state(dumps))

if __name__ == "__main__":
    main()
