#!/usr/bin/env python3
# vim: expandtab ts=4 sw=4 sts=4:
import argparse
import json
import logging
import logging.handlers
import urllib.request
import sys
import platform

JUGGLER_STATUS_MAP = {
    "0": "OK", "1": "WARN", "2": "CRIT",
    "OK": "OK", "CRIT": "CRIT", "WARN": "WARN",
}

JUGGLER_URLS = ["http://localhost:31579/events",
                "http://juggler-push.search.yandex.net:80/events",
                ]
logger = logging.getLogger(__name__)

def main():
    host = platform.node()

    events = []
    for line in sys.stdin:
        try:
            [mark, rest] = line.split(":",2)
            if mark != "PASSIVE-CHECK":
                logger.error("Unexpected line, skip it: %s", line)
                continue
        except ValueError as e:
            logger.error("Unexpected line, skip it: %s", line)
            continue

        [service, status, desc] = rest.split(";", 3)
        events.append({
            "description": desc,
            "service": service,
            "status": JUGGLER_STATUS_MAP[status],
            "host": host,
            "instance": "",
        })

    if send(events, "ib-mon"):
        sys.exit(0)
    else:
        sys.exit(1)

def send(data, source):
    errors = []
    logger.debug("send to juggler %s", data)
    for url in JUGGLER_URLS:
        sdata = {"events": data, "source": source}
        try:
            ans = get_url(url, timeout=2, data=json.dumps(sdata))
            ans = json.loads(ans)
        except Exception as e:
            errors.append(Exception("unable to send to juggler %s %r" % (url, e)))
        else:
            logger.debug("sent data to url=%s. prev push errors=%s", url, errors)
            for event in ans["events"]:
                if int(event["code"]) != 200:
                    logger.debug("%s", event)
            break
    if len(errors) == len(JUGGLER_URLS):
        # raise Exception("unable to send to %s: %s" % (JUGGLER_URLS, errors))
        logger.debug("unable to send to %s: %s", JUGGLER_URLS, errors)
        return False
    return True


def get_url(url, timeout=None, data=None, headers=None):
    if headers is None:
        headers = {}

    if data is not None and not isinstance(data, bytes):
        data = str(data).encode()
    req = urllib.request.Request(url=url,
                                 data=data,
                                 headers=headers)
    with urllib.request.urlopen(req, timeout=timeout) as f:
        ret = f.read().decode()

    return ret

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='send passive-checks to juggler-client')
    parser.add_argument('--debug', '-d', dest='debug', action='store_true', help='show debug logging')
    parser.add_argument('--log-file', '-l', dest='log_file', action='store', help='log file')
    args = parser.parse_args()

    if args.debug:
        level = logging.DEBUG
    else:
        level = logging.WARNING

    if args.log_file:
        handler = logging.FileHandler(args.log_file, encoding="utf8")
    else:
        handler = logging.StreamHandler()

    fmt = logging.Formatter("%(asctime)s - %(filename)s:%(lineno)d - %(funcName)s() - %(levelname)s - %(message)s")
    handler.setFormatter(fmt)
    handler.setLevel(level)

    logger.addHandler(handler)
    logger.setLevel(level)

    main()
