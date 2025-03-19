#!/usr/bin/env python3

import argparse
import json
import os
import sys
import time

from datetime import timedelta


def parse_args():
    arg = argparse.ArgumentParser(description='WAL-G check backups monrun script')
    arg.add_argument(
        '-c',
        '--clusters',
        type=str,
        required=True,
        metavar='<clusters>',
        default='',
        help='comma separated list of clusters')
    arg.add_argument(
        '-r',
        '--recover-dir',
        type=str,
        required=False,
        metavar='<dir>',
        default='/backups/recover',
        help='Folder to check backups')
    arg.add_argument(
        '-d',
        '--delay',
        type=int,
        required=False,
        metavar='<delay>',
        default=172800,
        help='Critical delay for check (seconds)')
    return arg.parse_args()


def die(status=0, message='OK'):
    """
    Print status;message and exit
    """
    print('{status};{message}'.format(status=status, message=message))
    sys.exit(0)


def _main():
    try:
        args = parse_args()
        warnings = list()
        errors = list()
        for cluster in args.clusters.split(','):
            if not cluster:
                continue
            status_file_path = os.path.join(args.recover_dir, f'.{cluster}.status')
            if not os.path.exists(status_file_path):
                warnings.append(f'{cluster} no data')
                continue
            with open(status_file_path, 'r') as sfile:
                try:
                    state = json.loads(sfile.read())
                    if 'ts' not in state or 'status' not in state:
                        warnings.append(f'{cluster} malformed status file')
                        continue
                    delta_seconds = int(time.time() - state['ts'])
                    if delta_seconds > args.delay:
                        diff = timedelta(seconds=delta_seconds)
                        errors.append(f'{cluster} check stale for {diff.days} days')
                    if state.get('status') != 'OK':
                        errors.append('{cluster} {status} {err}'.format(
                            cluster=cluster, status=state.get('status'), err=state.get('message')))
                except json.decoder.JSONDecodeError:
                    warnings.append(f'{cluster} malformed status file')
                    continue
        if errors:
            die(2, '; '.join(errors + warnings))
        if warnings:
            die(1, '; '.join(warnings)) 
        die()
    except Exception as exc:
        die(1, 'Unable to check backup status: {err}'.format(err=repr(exc)))


if __name__ == '__main__':
    _main()
