#!/skynet/python/bin/python

import os
import sys

sys.path.append(
    os.path.abspath(
        os.path.join(os.path.dirname(__file__), '..')
    )
)
import gencfg
from core.db import CURDB
from time import sleep, time

if __name__ == "__main__":
    db = CURDB
    res = db.ping_repo()
    exit(int(not res))
