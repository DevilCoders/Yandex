# -*- coding: utf-8 -*-
# pylint: disable=missing-docstring,invalid-name,too-many-locals,line-too-long,too-many-branches

from __future__ import absolute_import, unicode_literals, print_function

import argparse
import json
import logging
import os
import sys
import time

from juggler_sdk import CheckFilter
from juggler_sdk import DowntimeSelector
from juggler_sdk import JugglerApi
from juggler_sdk import Check
from juggler_sdk import const
from juggler_sdk import errors


def _get_oauth_token():
    return os.environ.get(const.OAUTH_TOKEN_ENV_VAR)


def _write_dump(output_path, checks_list):
    def write(stream):
        json.dump([x.to_dict(minimize=True) for x in checks_list], stream, sort_keys=True, indent=4)

    if output_path == '-':
        write(sys.stdout)
    else:
        with open(output_path, "w") as stream:
            write(stream)
        print("{0} checks written to {1}".format(len(checks_list), output_path))


def _format_dump_name(desired_path, query):
    if desired_path:
        return desired_path
    return ".".join([_f for _f in ["juggler_dump", query.host, query.service] + list(query.tags) + ["json"] if _f])


def _process_load(args):
    check_list = json.load(args.source)

    client = JugglerApi(args.api_url, args.mark, args.dry_run, oauth_token=_get_oauth_token())
    for check_dict in check_list:
        check = Check.from_dict(check_dict)
        result = client.upsert_check(check, force=args.force)
        if result.changed or args.verbose_level > 0:
            print(json.dumps({
                "check": check.to_dict(minimize=True),
                "changed": result.changed,
                "diff": result.diff.to_dict()
            }, indent=4, sort_keys=True))
    removed_checks = client.cleanup().removed
    if removed_checks:
        print("Removing checks")
        print('\n'.join('  {}:{}'.format(h, s) for h, s in removed_checks))


def _process_dump(args):
    query = CheckFilter(host=args.host, service=args.service, tags=args.tags, project=args.project)
    with JugglerApi(args.source_url, oauth_token=_get_oauth_token()) as client:
        checks = client.get_checks(filters=[query]).checks
    _write_dump(_format_dump_name(args.dump_path, query), checks)


def _process_downtime(args):

    filters = [DowntimeSelector(host=args.host, service=args.service, project=args.project)]
    end = time.time() + args.duration

    client = JugglerApi(args.api_url, oauth_token=_get_oauth_token())
    downtime = client.set_downtimes(filters, end_time=end, description=args.description)
    print("Downtime created: downtime_id={}".format(downtime.downtime_id))


def main():
    logging.basicConfig(level=logging.INFO)

    parser = argparse.ArgumentParser(description='Client for Juggler API.')

    subparsers = parser.add_subparsers()
    parser_load = subparsers.add_parser('load', help='upsert checks from JSON file')
    parser_load.add_argument('--api', dest='api_url', help='url that points to juggler api, %(default)s by default',
                             default='http://juggler-api.search.yandex.net')
    parser_load.add_argument('-s', '--source', type=argparse.FileType('r'), default='-',
                             help='path to file to load JSON from, stdin by default')
    parser_load.add_argument('-m', '--mark', help='mark created check with this id')
    parser_load.add_argument('-c', '--dry-run', action='store_true', help='don\'t modify anything')
    parser_load.add_argument("-v", "--verbose", action="count", dest="verbose_level", default=0,
                             help="Print more stuff")

    parser_load.add_argument('--force', action='store_true', help='force changes')
    parser_load.set_defaults(target=_process_load)

    parser_dump = subparsers.add_parser('dump', help='dump checks to a JSON file')
    parser_dump.add_argument('--api', dest='source_url', help='source API url',
                             default='http://juggler-api.search.yandex.net')
    parser_dump.add_argument('--tags', help="tag name to search for", nargs="*")
    parser_dump.add_argument('--host', help="host name to search for")
    parser_dump.add_argument('--service', help="service name to search for")
    parser_dump.add_argument('--project', help="project name to search for")
    parser_dump.add_argument('--dump-path', dest='dump_path', default='-', help="output file path")
    parser_dump.set_defaults(target=_process_dump)

    parser_downtime = subparsers.add_parser('downtime', help='set downtime for host')
    parser_downtime.add_argument('--api', dest='api_url', help='source API url',
                                 default='http://juggler-api.search.yandex.net')
    parser_downtime.add_argument('--host', help="host for downtime")
    parser_downtime.add_argument('--service', help="service for downtime")
    parser_downtime.add_argument('--project', help="project for downtime")
    parser_downtime.add_argument('--duration', help="downtime duration in seconds", default=1800)
    parser_downtime.add_argument('--description', help="downtime description", default="downtime from cli")
    parser_downtime.set_defaults(target=_process_downtime)

    args = parser.parse_args()
    try:
        args.target(args)
    except errors.JugglerError as exc:
        print(exc, file=sys.stderr)
        sys.exit(1)
