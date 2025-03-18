#!/skynet/python/bin/python

import os
import sys
import time

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from collections import defaultdict

import gencfg
from core.db import CURDB
from gaux.aux_decorators import gcdisable


@gcdisable
def main():
    CURDB.groups.check_group_intersections()


if __name__ == '__main__':
    main()
