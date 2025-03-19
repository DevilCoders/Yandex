#!/usr/bin/env python
"""
Check salt-minion version against expected one
"""

import argparse

from salt.grains.core import saltversion


def _main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-v', '--version', type=str, help='Expected version')
    args = parser.parse_args()
    try:
        cur_ver = saltversion()['saltversion']
        if cur_ver != args.version:
            print('1;Invalid salt-minion version: {current}, expected: {expected}'.format(
                current=cur_ver, expected=args.version))
            return

        print('0;OK')
    except Exception as exc:
        print('1;exception: {exc}'.format(exc=repr(exc)))


if __name__ == '__main__':
    _main()
