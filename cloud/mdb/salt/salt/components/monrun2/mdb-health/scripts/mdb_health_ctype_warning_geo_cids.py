#!/usr/bin/env python
"""
Check geos demanding attention <ctype> clusters
"""

import sys
import argparse
import requests

MAX_EXAMPLES = 300
DEFAULT_CRIT_LIMIT = 2


def die(status=0, message='OK'):
    """
    Print status in monrun-compatible format
    """
    print('{status};{message}'.format(status=status, message=message))
    sys.exit(0)


def _obtain_health_info(url, verify, ctype):
    url = "{url}/v1/unhealthyaggregatedinfo?c_type={ctype}&agg_type=clusters".format(url=url, ctype=ctype)
    req = requests.get(url, verify=verify)
    req.raise_for_status()
    return req.json()


def _format_report(info, crit_limit):
    die_code = 1
    problematic_cnt = 0
    report = ""

    for elem in info.get('by_warning_geo', []):
        problematic_cnt = max(problematic_cnt, elem["count"])
        report += "{geo}: {count} clusters [{examples}] ".format(geo=elem["geo"], count=elem["count"],
                                                                 examples=','.join(elem["examples"][:MAX_EXAMPLES]))
    else:
        die_code = 0
        report = "NO"

    if problematic_cnt > crit_limit:
        die_code = 2

    return die_code, report


def _main():
    args_parser = argparse.ArgumentParser()
    args_parser.add_argument(
        '-u',
        '--url',
        type=str,
        help='mdb-health url')

    args_parser.add_argument(
        '-v',
        '--verify',
        type=str,
        help='url TLS ceritificate (do not set for default)')

    args_parser.add_argument(
        '-t',
        '--ctype',
        type=str,
        default="postgresql_cluster",
        help='cluster type')

    args_parser.add_argument(
        '-c',
        '--crit',
        type=int,
        default=DEFAULT_CRIT_LIMIT,
        help='crit limit')

    args = args_parser.parse_args()
    url = args.url
    verify = args.verify
    ctype = args.ctype

    try:
        health_info = _obtain_health_info(url, verify, ctype)
    except requests.exceptions.HTTPError as exc:
        die(1, "failed to check aggregated availability info due excpetion: {}".format(exc))

    die_code, report = _format_report(health_info['sla'], args.crit)
    aggr_report = "geos demanding attention: {sla_report}".format(sla_report=report)

    die(die_code, aggr_report)


if __name__ == '__main__':
    _main()

