#!/skynet/python/bin/python

import os
import sys

D = os.path.abspath(__file__)
for i in range(3):
    D = os.path.dirname(D)
sys.path.append(D)

import inspect

import tools.analyzer.functions
import gaux.aux_utils
import utils.mongo.mongopack

HEAD = """#!/skynet/python/bin/python

from __future__ import absolute_import, print_function

import re
import argparse
from pprint import pprint
import os
import socket
from multiprocessing import Pool
import subprocess
import json
import time
import urllib2
import signal
import fcntl

import msgpack

"""

TAIL = """
# ===============================================================================
# main stuff
# ===============================================================================

def generate(arg):
    signal.alarm(300)
    (host, port), instance_env = arg

    try:
        return {
            'port' : port,
            'major_tag' : instance_major_tag(instance_env, {}),
            'minor_tag' : instance_minor_tag(instance_env, {}),
            'instance_cpu_usage' : instance_cpu_usage(instance_env, {}),
            'instance_mem_usage' : instance_mem_usage(instance_env, {}),
        }
    except:
        return None

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('-f', '--format', type=str, default='pretty',
                        choices=('pretty', 'binary'),
                        help='Output in serialized or pretty format')
    parser.add_argument('-d', '--deserialize-file', type=str, default=None,
                        help='Deserialize from file rather than serialize')
    parser.add_argument('--search', action='store_true', default=False,
                        help='Runs in yandex-search environment with bsconfig, iss, and stuff.')
    return parser.parse_args()

def main():
    args = parse_args()

    if args.deserialize_file is not None:
        data = open(args.deserialize_file).read()
        pprint(Serializer.deserialize(msgpack.unpackb(data)))
    else:
        if args.search:
            p = Pool(10)
            instances_data = p.map(generate, get_all_instances_env().items())
            instances_data = filter(lambda x: x is not None, instances_data)
        else:
            instances_data = [generate(get_fake_instances_env().items()[0])]

        if args.format == 'pretty':
            pprint(instances_data)
        elif args.format == 'binary':
            print(msgpack.packb(Serializer.serialize(instances_data)))

if __name__ == '__main__':
    main()
"""


def _pp(obj):
    return ''.join(inspect.getsourcelines(obj)[0]) + '\n'


def main():
    result = HEAD

    result += """
ISS_ILIST_URL = "%s"
ISS_CGROUP_PATH = "%s"

""" % (tools.analyzer.instance_env.ISS_ILIST_URL, tools.analyzer.functions.ISS_CGROUP_PATH)

    result += """
# =====================================================================================
# Auxiliarily functions
# =====================================================================================

"""
    result += _pp(tools.analyzer.functions._host_ncpu)
    result += _pp(tools.analyzer.functions._detect_child_pids_recurse)
    result += _pp(tools.analyzer.functions._detect_pids)
    result += _pp(tools.analyzer.functions._detect_cgroup)
    result += _pp(tools.analyzer.functions._detect_porto)

    result += _pp(gaux.aux_utils.run_command)

    result += """
# =====================================================================================
# Instances env
# =====================================================================================

"""
    result += _pp(tools.analyzer.instance_env.EInstanceTypes)
    result += _pp(tools.analyzer.instance_env._update_bsconfig_instance_env)
    result += _pp(tools.analyzer.instance_env.get_bsconfig_instances_env)
    result += _pp(tools.analyzer.instance_env._update_iss_instance_env)
    result += _pp(tools.analyzer.instance_env.get_iss_instances_env)
    result += _pp(tools.analyzer.instance_env.get_fake_instances_env)
    result += _pp(tools.analyzer.instance_env.get_all_instances_env)

    result += """
# =====================================================================================
# FUNCTIONS
# =====================================================================================

"""
    result += _pp(tools.analyzer.functions.instance_major_tag)
    result += _pp(tools.analyzer.functions.instance_minor_tag)
    result += _pp(tools.analyzer.functions.instance_cpu_usage)
    result += _pp(tools.analyzer.functions.instance_mem_usage)

    result += """
# ========================================================================================
# SERIALIZER
# ========================================================================================

"""
    result += _pp(utils.mongo.mongopack.PackBuffer)
    result += _pp(utils.mongo.mongopack.UnpackBuffer)
    result += _pp(utils.mongo.mongopack.Serializer)

    result += TAIL

    return result
