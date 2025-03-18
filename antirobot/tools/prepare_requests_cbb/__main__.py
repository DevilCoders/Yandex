import argparse
import logging
import multiprocessing
import sys
import os
import json
import re

import yt.wrapper as yt

from antirobot.tools.prepare_requests_cbb.lib import service_identifier


BAD_PATH_PREFIXES = [
    "/checkcaptcha",
    "/xcheckcaptcha",
]


def check_prefixes(path, prefixes):
    for prefix in prefixes:
        if path.startswith(prefix):
            return True

    return False


class Mapper:
    def __init__(self, service_identifier):
        self.service_identifier = service_identifier

    def __call__(self, rec):
        request = rec[b"request"]

        host = None
        path = None

        host_header = "Host: "
        path_header = "GET "

        try:
            for line in request.decode("utf-8").splitlines():
                line = line.strip()
                if line.startswith(host_header):
                    host = line[len(host_header):]

                if line.startswith(path_header):
                    path = line[len(path_header):].rsplit(" ", 1)[0]  # Get rid HTTP/1.1

            if host is not None and path is not None and not check_prefixes(path, BAD_PATH_PREFIXES):
                url = host + path
                yield {
                    b"request": request,
                    b"service": self.service_identifier.get_service(url).encode("utf-8"),
                    b"url": url.encode("utf-8"),
                }
        except UnicodeDecodeError:
            logging.warning("Skipping request %s", request)


class Reducer:
    def __init__(self, limit):
        self.limit = limit

    def __call__(self, key, recs):
        service = key[b"service"]

        result = []
        for rec in recs:
            result.append(rec[b"request"])

            if len(result) == self.limit:
                break

        if result:
            yield {
                b"service": service,
                b"requests": result,
                b"requests_num": len(result),
            }


def prepare_requests(args):
    client = yt.YtClient(proxy=args.proxy, token=os.getenv("YT_TOKEN") or yt.http_helpers.get_token())

    client.run_map_reduce(
        Mapper(service_identifier.ServiceIdentifier()),
        Reducer(args.limit),
        args.input,
        args.output,
        reduce_by="service",
        format=yt.YsonFormat(encoding=None),
    )
    client.run_sort(args.output, sort_by="service")


def parse_args():
    parser = argparse.ArgumentParser(
        description="Prepare requests for cbb",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )
    parser.add_argument("--input", help="Input table")
    parser.add_argument("--proxy", help="Input and output YT proxy", default="hahn")
    parser.add_argument("--limit", type=int, default=100, help="Number of requests to keep per service")
    parser.add_argument("--output", help="Output table", default="//home/antirobot/log-viewer/cbb_requests_services")
    parser.add_argument("--verbose", action="store_true", help="Verbose logging")

    parser.set_defaults(func=prepare_requests)

    return parser.parse_args()


def main():
    args = parse_args()

    logging.basicConfig(
        level=logging.DEBUG if args.verbose else logging.INFO,
        format="%(levelname)-8s [%(asctime)s] %(message)s",
        stream=sys.stderr,
        datefmt="%Y-%m-%d %H:%M:%S",
    )

    args.func(args)


if __name__ == "__main__":
    main()
