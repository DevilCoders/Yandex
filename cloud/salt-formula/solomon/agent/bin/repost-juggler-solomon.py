#!/usr/bin/python3

import json
import logging
import requests
import subprocess
import time

SOLOMON_LOCAL_ENDPOINT = "http://[::1]:10050/juggler_events"
SOLOMON_TIMEOUT = 3
VALUES = {
  "OK": 0,
  "WARN": 1,
  "CRIT": 2
}

log = logging.getLogger(__name__)


def setup_logging():
    handler = logging.StreamHandler()
    stdout_formatter = logging.Formatter('%(asctime)s - %(levelname)s - %(message)s')
    handler.setFormatter(stdout_formatter)
    log.addHandler(handler)
    log.setLevel(logging.INFO)


def iterload_json(stream):
    buf = ""
    dec = json.JSONDecoder()
    max_err_count = 10
    err_count = 0

    while True:
        line = stream.readline().decode()
        buf += line.strip()
        if buf and not buf.startswith('{'):
            log.warn("Discarding message %r", buf)
            buf = ""
            continue
        if buf.endswith("}"):
            try:
                yield dec.raw_decode(buf)
                buf = ""
                err_count = 0
            except json.JSONDecodeError as e:
                err_count += 1
                if err_count > max_err_count:
                    log.error("Discarding message %r, error: %r", buf, e)
                    err_count = 0
                    buf = ""
                continue


def main():
    setup_logging()
    p = subprocess.Popen(["/usr/bin/jclient-api", "monitor-events"], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    for j, size in iterload_json(p.stdout):
        now = time.time()
        value = VALUES[j["status"]]
        log.info("Processing event %r", j)

        sensor = {
            "kind": "GAUGE",
            "labels": {
                "juggler_service": j["service"],
                "description": j["description"],
            },
            "timeseries": [{
                "ts": int(now),
                "value": value
            }]
        }
        try:
            r = requests.post(SOLOMON_LOCAL_ENDPOINT, json={"sensors": [sensor]}, timeout=SOLOMON_TIMEOUT)
            r.raise_for_status()
        except requests.RequestException as e:
            log.exception("Could not post sensor %r to solomon-agent: %r", sensor, type(e).__name__)


if __name__ == "__main__":
    main()
