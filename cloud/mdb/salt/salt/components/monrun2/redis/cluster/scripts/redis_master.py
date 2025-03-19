#!/usr/bin/env python
"""
Dummy script for Redis Cluster.
"""

import sys


def die(status=0, message='OK'):
    """
    Print status in monrun-compatible format
    """
    print('{status};{message}'.format(status=status, message=message))
    sys.exit(0)


def _main():
    die()


if __name__ == '__main__':
    _main()
