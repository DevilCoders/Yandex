#!/skynet/python/bin/python

import inspect

import gaux.aux_utils
import gaux.aux_multithread
import utils.pregen.check_alive_machines as subutil

HEAD = """#!/skynet/python/bin/python

import os
import fcntl
import time
import socket
import subprocess
import argparse
from pprint import pprint
import msgpack
import re

TIMEOUT_STATUSES = [None, -9, 255]

"""

TAIL = """
# ===============================================================================
# main stuff
# ===============================================================================
def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('-f', '--format', choices=('pretty', 'binary'), default='pretty')
    return parser.parse_args()

def main():
    args = parse_args()

    result = detect_unworking()

    if args.format == 'pretty':
        pprint(result)
    elif args.format == 'binary':
        print(msgpack.packb(result))

if __name__ == '__main__':
    main()
"""


def _sl(obj):
    return ''.join(inspect.getsourcelines(obj)[0]) + '\n'


def main():
    result = HEAD

    result += _sl(gaux.aux_utils.run_command)
    result += _sl(gaux.aux_multithread.ERunInMulti)
    result += _sl(subutil._gen_check_command)
    result += _sl(subutil._check_via_proto)
    result += _sl(subutil._get_raids_info)

    avail_funcs = filter(lambda x: x.startswith('check_'), dir(subutil))
    avail_funcs = filter(lambda x: x not in ['check_xfastbone', 'lowfreq_unsafe'], avail_funcs)
    for funcname in avail_funcs:
        result += _sl(getattr(subutil, funcname))

    result += _sl(subutil.get_checkers)

    result += """def detect_unworking():
    try:
        hname = socket.gethostbyaddr(socket.gethostbyname(socket.gethostname()))[0]
    except:
        hname = socket.gethostname()
    params = { 'mode' : ERunInMulti.SKYNET }

    result = {}
    for checker in get_checkers():
        try:
            result[checker] = get_checkers()[checker](hname, params)
        except:
            result[checker] = True

    return result
"""
    result += TAIL

    return result
