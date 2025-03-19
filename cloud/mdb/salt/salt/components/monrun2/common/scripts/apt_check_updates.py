#!/usr/bin/env python
"""
Check if the updates dir is dirty
"""

import os
import sys

import apt_pkg

UPDATES_DIR = '/var/lib/dpkg/updates'


def die(status=0, message='OK'):
    """
    Print status;message and exit
    """
    print('{status};{message}'.format(status=status, message=message))
    sys.exit(0)


def _main():
    broken = False
    try:
        if not os.path.exists(UPDATES_DIR):
            die()
        for file_name in os.listdir(UPDATES_DIR):
            if file_name.isdigit():
                broken = True
                break
        if not broken:
            die()
        apt_pkg.init()
        if apt_pkg.pkgsystem_lock_inner():
            die(status=1, message='dpkg was interrupted')
    except getattr(apt_pkg, 'Error', Exception):
        die(message='dpkg is running')
    except OSError:
        # updates dir vanished before listdir but after exists check
        die()
    except Exception as exc:
        die(status=1, message=repr(exc))


if __name__ == '__main__':
    _main()
