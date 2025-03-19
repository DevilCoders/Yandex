#!/usr/bin/env python

import argparse
import logging

from client import ConductorClient, get_config


def _main():
    parser = argparse.ArgumentParser()
    parser.add_argument('cmd', choices=['add', 'del'], type=str, help='Operation')
    parser.add_argument('-d', '--dc', type=str, help='Short datacenter name (e.g. vla)')
    parser.add_argument('-g', '--group', type=str, help='Group for hosts')
    parser.add_argument('hosts', metavar='fqdn', type=str, nargs='+', help='target fqdns')
    args = parser.parse_args()

    config = get_config()
    client = ConductorClient(config)

    logging.basicConfig(format=logging.BASIC_FORMAT, level=logging.DEBUG)
    logger = logging.getLogger('main')

    if args.cmd == 'add':
        logger.info('Start add host(s)')
        if not getattr(args, 'group', None):
            logger.info('group is required for host creation')
            parser.print_help()
            return
        for host in args.hosts:
            try:
                client.ensure_host_create(host, args.group, args.dc)
                logger.info(f'{host} created')
            except Exception as exc:
                logger.error(f'{host} error: {repr(exc)}')
    else:
        for host in args.hosts:
            try:
                client.ensure_host_delete(host)
                logger.info(f'{host} deleted')
            except Exception as exc:
                logger.error(f'{host} error: {repr(exc)}')


if __name__ == '__main__':
    _main()
