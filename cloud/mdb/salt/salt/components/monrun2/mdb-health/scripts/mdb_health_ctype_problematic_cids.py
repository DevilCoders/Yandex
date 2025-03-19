#!/usr/bin/env python
"""
Check problematic ha/non_ha <ctype> clusters
"""

import sys
import argparse
import requests

MAX_EXAMPLES = 300
DEFAULT_CRIT_LIMIT = 1
DEFAULT_AVAIL_TYPE = 'sla'

def die(status=0, message='OK'):
    """
    Print status in monrun-compatible format
    """
    print('{status};{message}'.format(status=status, message=message))
    sys.exit(0)


def _obtain_health_info(url, verify, ctype, agg_type):
    url = "{url}/v1/unhealthyaggregatedinfo?c_type={ctype}&agg_type={agg_type}".format(url=url, ctype=ctype, agg_type=agg_type)
    req = requests.get(url, verify=verify)
    req.raise_for_status()
    return req.json()


def _format_report(info, crit_limit, skip_userfault_broken=False):
    no_write_cids = []
    no_read_cids = []
    report = ""
    die_code = 0

    problematic_cnt = 0

    for elem in info.get('by_availability', []):
        if elem['userfaultBroken'] and skip_userfault_broken:
            continue

        if not elem['writable']:
            no_write_cids += (elem['examples'])
            problematic_cnt += 1
        elif not elem['readable']:
            no_read_cids += (elem['examples'])
            problematic_cnt += 1

    if problematic_cnt != 0:
        if problematic_cnt >= crit_limit:
            die_code = 2
        else:
            die_code = 1

    if no_write_cids or no_read_cids:
        if no_write_cids:
            report += "not writable cids are: {}".format(no_write_cids[:MAX_EXAMPLES])
        if no_read_cids:
            report += "not readable cids are: {}".format(no_read_cids[:MAX_EXAMPLES])
    else:
        report = "OK"

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
        '-g',
        '--agg_type',
        type=str,
        default="clusters",
        help='aggregation type')

    args_parser.add_argument(
        '-c',
        '--crit',
        type=int,
        default=DEFAULT_CRIT_LIMIT,
        help='crit limit')

    args_parser.add_argument(
        '-a',
        '--avail-type',
        type=str,
        default=DEFAULT_AVAIL_TYPE,
        help='sla/no_sla')

    args_parser.add_argument(
        '-s',
        '--skip-userfault-broken',
        type=bool,
        default=False,
        help='include userfault broken clusters in report or not')

    args = args_parser.parse_args()
    url = args.url
    verify = args.verify
    ctype = args.ctype
    agg_type = args.agg_type

    try:
        health_info = _obtain_health_info(url, verify, ctype, agg_type)
    except requests.exceptions.HTTPError as exc:
        die(1, "failed to check aggregated availability info due excpetion: {}".format(exc))

    if args.avail_type == 'sla':
        die_code, sla_report = _format_report(health_info['sla'], args.crit, args.skip_userfault_broken)

        aggr_report = "sla problematic cids: {sla_report}".format(sla_report=sla_report)
    else:
        die_code, no_sla_report = _format_report(health_info['no_sla'], args.crit, args.skip_userfault_broken)

        aggr_report = "no_sla problematic cids: {no_sla_report}".format(no_sla_report=no_sla_report)

    die(die_code, aggr_report)


if __name__ == '__main__':
    _main()

