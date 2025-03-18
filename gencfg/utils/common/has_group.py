#!/skynet/python/bin/python
import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg
from core.db import CURDB

def main(args):
    if len(args) != 2:
        print('Usage: has_group.py "MAN_SOMETHING, VLA_SOMETHING"')
        sys.exit(-1)

    groups = [x.strip() for x in args[1].split(',')]
    results = [CURDB.groups.has_group(x) for x in groups]

    for result in results:
        print result,
    print

if __name__ == '__main__':
    main(sys.argv)

