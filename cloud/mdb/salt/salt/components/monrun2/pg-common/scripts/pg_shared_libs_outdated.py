#!/usr/bin/env python3
"""
Check for outdated extensions loaded into postmaster
"""

import argparse
import os
import sys

def die(status=0, message='OK'):
    """
    Print status;message and exit
    """
    print('{status};{message}'.format(status=status, message=message))
    sys.exit(0)


def get_deleted_libs(data_path, lib_path):
    """
    Get a set of deleted libs loaded into postmaster
    """
    try:
        with open(os.path.join(data_path, 'postmaster.pid')) as pid_file:
            pid = int(pid_file.readline().strip())
    except Exception as exc:
        die(1, 'Unable to get postmaster pid: {error}'.format(error=exc))

    try:
        with open('/proc/{pid}/maps'.format(pid=pid)) as map_file:
            libs = set()
            for line in map_file:
                if '(deleted)' in line and lib_path in line:
                    libs.add(line.split()[-2].split('/')[-1])
        return libs
    except Exception as exc:
        die(1, 'Unable to parse map file: {error}'.format(error=exc))


def _main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-d', '--data-path', type=str, required=True, help='PostgreSQL data dir path')
    parser.add_argument('-l', '--lib-path', type=str, required=True, help='PostgreSQL lib dir path')
    args = parser.parse_args()

    libs = get_deleted_libs(args.data_path, args.lib_path)
    if not libs:
        die()

    die(2, '{num} shared libs are outdated: {libs}'.format(num=len(libs), libs=', '.join(sorted(libs))))

if __name__ == '__main__':
    _main()
