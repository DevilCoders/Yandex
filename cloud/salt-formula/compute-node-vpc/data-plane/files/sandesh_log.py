#!/usr/bin/env python3

import argparse
import json
import logging
import os

from datetime import datetime, timedelta
from logging import FileHandler
from lxml import etree
from urllib.request import urlopen

LOGGER_NAME = "SandeshTrace{}"
LOG_NAME = "sandesh-{}.log"
TMP_DIR = "/tmp/sandesh"
URL = "http://0.0.0.0:{port}/Snh_SandeshTraceRequest?x={buf}"
SERVICE_PORT_MAP = {
    "contrail-control": 8083,
    "contrail-api": 8084,
    "contrail-vrouter-agent": 8085,
    "contrail-dns": 8092
}


def saved_logstr_path(sandesh_buffer):
    return os.path.join(TMP_DIR, sandesh_buffer)


def get_saved_logstr(sandesh_buffer):
    tmp_path = saved_logstr_path(sandesh_buffer)
    if not os.path.exists(tmp_path):
        return None, None

    with open(tmp_path, "r") as f:
        saved_dict = json.load(f)

    return int(saved_dict["timestamp"]), saved_dict["line"]


def save_logstr(timestamp, line, sandesh_buffer):
    if not os.path.exists(TMP_DIR):
        os.makedirs(TMP_DIR)

    saved_dict = {"timestamp": timestamp, "line": line}
    with open(saved_logstr_path(sandesh_buffer), "w") as f:
        json.dump(saved_dict, f)


def make_log_path(sandesh_buffer, log_dir):
    return os.path.join(log_dir, LOG_NAME.format(sandesh_buffer.lower()))


def make_url(sandesh_buffer, service_name):
    return URL.format(port=SERVICE_PORT_MAP[service_name], buf=sandesh_buffer)


def make_logger_name(sandesh_buffer):
    return LOGGER_NAME.format(sandesh_buffer)


def make_time(timestamp):
    timestamp_usec = int(timestamp)
    timestamp_sec = timestamp_usec // 10**6
    timestamp_msec = timestamp_usec % 10**6
    str_time = datetime.fromtimestamp(timestamp_sec)
    str_time += timedelta(microseconds=timestamp_msec)
    return str_time.strftime("%Y-%m-%d %H:%M:%S.%f")


def logger_setup(sandesh_buffer, log_dir):
    log = logging.getLogger(make_logger_name(sandesh_buffer))
    fh = FileHandler(make_log_path(sandesh_buffer, log_dir))
    formatter = logging.Formatter("%(message)s")
    fh.setFormatter(formatter)
    log.addHandler(fh)
    log.setLevel(logging.DEBUG)
    return log


def parse_args():
    parser = argparse.ArgumentParser(description="Logs sandesh trace buffers")
    parser.add_argument(
        "--buffer", help="name of sandesh buffer", required=True
    )
    parser.add_argument(
        "--service", help="name of service to get sandesh trace",
        choices=SERVICE_PORT_MAP.keys(), required=True
    )
    parser.add_argument(
        "--dir", help="directory where to store logs", required=True
    )

    return parser.parse_args()


def main():
    args = parse_args()
    log = logger_setup(args.buffer, args.dir)

    last_seen_timestamp, last_seen_line = get_saved_logstr(args.buffer)

    response = urlopen(make_url(args.buffer, args.service))
    parsed_log = [e.text for e in etree.parse(response).xpath("//element")]

    skip_messages = False
    if last_seen_timestamp:
        skip_messages = True
        first_timestamp = int(parsed_log[0].split(" ", 1)[0])
        if last_seen_timestamp < first_timestamp:
            log.warning("Some messages were lost due to high sandesh message rate. Lost: {} - {}".format(make_time(first_timestamp), make_time(last_seen_timestamp)))
            skip_messages = False

    for elem in parsed_log:
        if skip_messages:
            if last_seen_line == elem:
                skip_messages = False
            continue

        line = elem.split(" ", 1)
        line[0] = make_time(line[0])
        log.info(" ".join(line))

    save_logstr(parsed_log[-1].split(" ", 1)[0], parsed_log[-1], args.buffer)


if __name__ == "__main__":
    main()
