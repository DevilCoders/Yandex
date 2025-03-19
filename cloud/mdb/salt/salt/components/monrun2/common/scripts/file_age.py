#!/usr/bin/env python3

import argparse
import sys
import os
import time

parser = argparse.ArgumentParser()
parser.add_argument('-w', '--warn', type=int, default=2, help='Warning used/total percent')
parser.add_argument('-c', '--crit', type=int, default=4, help='Critical used/total percent')
parser.add_argument('path', help='Path to file')
args = parser.parse_args()


def die(code=0, comment='OK'):
    print('{code};{comment}'.format(code=code, comment=comment))
    sys.exit(0)


try:
    st = os.stat(args.path)
except FileNotFoundError:
    die(0, "OK")
except Exception:
    die(1, "failed to check {} file age".format(args.path))

age = (time.time() - st.st_mtime) / (3600*24)

if age > args.crit:
    die(2, "{} file is {}d old".format(os.path.basename(args.path), int(age)))
elif age > args.warn:
    die(1, "{} file is {}d old".format(os.path.basename(args.path), int(age)))
else:
    die(0, "OK")
