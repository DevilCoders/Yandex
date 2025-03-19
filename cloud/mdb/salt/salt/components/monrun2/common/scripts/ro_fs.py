#!/usr/bin/env python
"""
Check if any filesystem in mounted read-only
"""

import sys

FS_FILTER = {'ext4'}


def die(status=0, message='OK'):
    """
    Print status;message and exit
    """
    print('{status};{message}'.format(status=status, message=message))
    sys.exit(0)


def _main():
    try:
        read_only = []
        with open('/proc/mounts') as mounts:
            for line in mounts:
                splitted = line.split()
                point = splitted[1]
                fs_type = splitted[2]
                options = splitted[3]
                if fs_type not in FS_FILTER:
                    continue
                if 'rw' not in options:
                    read_only.append(point)
        if read_only:
            die(2, 'Read-only fs: {fs}'.format(fs=', '.join(read_only)))
        die()
    except Exception as exc:
        die(1, 'Unable to check for ro fs: {err}'.format(err=repr(exc)))


if __name__ == '__main__':
    _main()
