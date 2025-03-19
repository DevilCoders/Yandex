#!/usr/bin/env python
"""
Fix dpkg if the updates dir is dirty
"""

import os
import subprocess

import apt_pkg

UPDATES_DIR = '/var/lib/dpkg/updates'


def is_apt_updates_dirty():
    """
    Check if apt updates dir dirty and no dpkg is running
    """
    broken = False
    try:
        if not os.path.exists(UPDATES_DIR):
            return False
        for file_name in os.listdir(UPDATES_DIR):
            if file_name.isdigit():
                broken = True
                break
        if not broken:
            return False
        apt_pkg.init()
        if apt_pkg.pkgsystem_lock_inner():
            return True
    except getattr(apt_pkg, 'Error', Exception):
        return False
    except OSError:
        # updates dir vanished before listdir but after exists check
        return False
    except Exception as exc:
        return False


def _main():
    dirty = is_apt_updates_dirty()

    if dirty:
        apt_pkg.pkgsystem_unlock_inner()
        subprocess.check_call(['dpkg', '--configure', '-a', '--force-confdef', '--force-confold'])


if __name__ == '__main__':
    _main()
