#!/skynet/python/bin/python

import os
import sys
import json
import argparse

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))
import gencfg
from core.argparse.parser import ArgumentParserExt
import core.argparse.types as argparse_types

from utils.saas.saas_host_info import SaaSHostInfo


def get_parser():
    parser = ArgumentParserExt(
        description="Dump host info for hosts")
    parser.add_argument('-g', '--group', type=argparse_types.group, default=None, required=False,
                        help='Dump info for all hosts in group')
    parser.add_argument('-s', '--hosts', type=argparse_types.hosts, default=None, required=False,
                        help='Dump info for specific host. Ignored if group is provided')
    parser.add_argument('-f', '--file', type=argparse.FileType('w'), default=sys.stdout,
                        help='File for result json')
    parser.add_argument('--yt', action='store_true')
    return parser


def dump_host_info(hosts, yt_schema_mode=False):
    if yt_schema_mode:
        hosts_info = map(lambda x: SaaSHostInfo(x).dump_for_yt(), hosts)
    else:
        hosts_info = map(lambda x: SaaSHostInfo(x).to_dict_full('Gb'), hosts)
    return hosts_info


if __name__ == '__main__':
    args = get_parser().parse_cmd()

    if args.group:
        host_list = args.group.getHosts()
    elif args.hosts:
        host_list = args.hosts
    else:
        print('Host list or group is required')
        exit(1)
    result = dump_host_info(host_list, args.yt)
    args.file.write(json.dumps(result))
