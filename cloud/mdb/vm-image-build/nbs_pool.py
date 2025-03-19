#!/usr/bin/env python
"""
NBS Pool helper
"""

import argparse
import json

from yc_common.clients.compute import ComputeClient


def _get_client(url, config):
    with open(config) as inp:
        parsed = json.load(inp)

    token = parsed.get(url)

    return ComputeClient(
        '{base}/compute/external'.format(base=url), iam_token=token)


def create(args):
    """
    Create new nbs pool (or modify existing)
    """
    client = _get_client(args.api, args.config)
    operation = client.update_disk_pooling(
        image_id=args.image, disk_count=args.num, type_id='network-hdd')
    client.wait_operation(operation, wait_timeout=1800)


def delete(args):
    """
    Delete disk pooling (if exists)
    """
    client = _get_client(args.api, args.config)
    operation = client.delete_disk_pooling(image_id=args.image)
    client.wait_operation(operation, wait_timeout=1800)


COMMANDS = {
    'create': create,
    'delete': delete,
}


def _main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        'action', type=str, help='Operation', choices=COMMANDS.keys())
    parser.add_argument(
        '-c',
        '--config',
        type=str,
        help='IAM tokens config',
        default='/etc/yc/sa-tokens.json')
    parser.add_argument('-a', '--api', type=str, help='Compute api url')
    parser.add_argument('-i', '--image', type=str, help='Image id')
    parser.add_argument('-n', '--num', type=int, help='Pool size')
    args = parser.parse_args()

    COMMANDS[args.action](args)


if __name__ == '__main__':
    _main()
