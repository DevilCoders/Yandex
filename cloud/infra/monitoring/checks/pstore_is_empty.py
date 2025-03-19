#!/usr/bin/env python3
import os

from yc_monitoring import JugglerPassiveCheck

PSTORE_DIR_PATH = '/sys/fs/pstore/'


def main():
    check = JugglerPassiveCheck('pstore_is_empty')
    try:
        if not os.path.exists(PSTORE_DIR_PATH):
            check.crit('Directory {} does not exist'.format(PSTORE_DIR_PATH))
        if not os.path.isdir(PSTORE_DIR_PATH):
            check.crit('{} is not a directory'.format(PSTORE_DIR_PATH))
        if os.listdir(PSTORE_DIR_PATH):
            check.crit('Directory {} is not empty'.format(PSTORE_DIR_PATH))
    except Exception as e:
        check.crit('An error occurred during check: ({}): {}'.format(e.__class__.__name__, e))
    print(check)


if __name__ == '__main__':
    main()
