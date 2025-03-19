#!/usr/bin/env python

# Provides: walle_tainted_kernel

import subprocess
import sys
import json

name = 'walle_tainted_kernel'

err_map = {
    0b1: 'P',
    0b10: 'F',
    0b100: 'S',
    0b1000: 'R',
    0b10000: 'M',
    0b100000: 'B',
    0b1000000: 'U',
    0b10000000: 'D',
    0b100000000: 'A',
    0b1000000000: 'W',
    0b10000000000: 'C',
    0b100000000000: 'I',
    0b1000000000000: 'O',
    0b10000000000000: 'E',
    0b100000000000000: 'L'
}

crit_errors = ['F', 'S', 'M', 'B', 'D']

def die(name, status, msg):
    res = {"result": {"reason": [msg]}}
    print("PASSIVE-CHECK:%s;%s;%s" % (name, status, json.dumps(res)))
    sys.exit(0)


def main():
    err_code = 0
    msg = "Ok"
    die(name, err_code, msg)


if __name__ == '__main__':
    main()
