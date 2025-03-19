#!/usr/bin/env python3
# pylint: disable=invalid-name
''' Repair cassandra by token range and rebuild by keyspace '''

import argparse
import datetime
import time
import json
import logging.handlers
import os
import random
import re
import shutil
import string
import sys
import fcntl
from multiprocessing import Pool
from subprocess import Popen, PIPE, check_output, TimeoutExpired

PID = str(os.getpid())
PIDFILE = "/var/run/cass-repair-tool.pid"
LOCKFILE = "/var/run/cass-repair-tool.lock"

CONFIG = "/etc/cassandra-repair.json"
LOG = "/var/log/cassandra/repair.log"
MON_FILE = "/var/log/cassandra/repair-monitoring.log"
MAX_REPAIR_TIME = 32 * 60 * 60  # 32 hours
LOG100MB = 100 * 1024 * 1024

LOGGER = logging.getLogger(__name__)
LOGGER.setLevel(logging.DEBUG)
HANDLER = logging.handlers.RotatingFileHandler(LOG, maxBytes=LOG100MB, backupCount=5)
HANDLER.setLevel(logging.DEBUG)
FORMATTER = logging.Formatter('%(asctime)s - %(levelname)s - %(message)s')
HANDLER.setFormatter(FORMATTER)
LOGGER.addHandler(HANDLER)

CONSOLE = logging.StreamHandler()
CONSOLE.setLevel(logging.INFO)
CONSOLE_FORMATTER = logging.Formatter('%(message)s')
CONSOLE.setFormatter(CONSOLE_FORMATTER)
LOGGER.addHandler(CONSOLE)


def config(conf):
    '''
    Parse the config file
    '''

    try:
        with open(conf, 'r') as conf_file:
            configs = json.load(conf_file)
            LOGGER.info("Got a list of databases: %s", configs['databases'])
            return configs['databases']
    except Exception as ex:  # pylint: disable=broad-except
        LOGGER.info("Can't import config: %s %s ", conf, ex)
        sys.exit()


def rotate_monfile(path):
    '''
     Rotate monitoring file if not empty
    '''
    if os.path.isfile(path) and os.path.getsize(path) > 0:
        for i in range(4, 0, -1):
            if os.path.isfile(path + "." + str(i)):
                shutil.move(path + "." + str(i), path + "." + str(i + 1))
        shutil.move(path, path + "." + str(1))


def monrun_output(path, pidfile, max_repair_time):
    '''
    Output monrun error
    '''
    if os.path.isfile(pidfile):
        with open(pidfile) as pid_file:
            pid = pid_file.read()
        since_epoch = check_output(["stat", "--format=%Y", "/proc/"+pid])
        etimes = time.time() - int(since_epoch)
        if int(etimes) > max_repair_time:
            hours = int(etimes) / 60 / 60
            print("2; Repair is running for: {:.2f} hours".format(hours))
            sys.exit()
    else:
        # max_repair_time is not exceeded
        pass

    if not os.path.isfile(path):
        print("0; Ok")
    else:
        error = 0
        with open(path, 'r') as mon_file:
            for _ in mon_file:
                error += 1
        print("1; Repairs failed: {}".format(error))


def generate_random_key(length):
    '''
    Generate random sting
    '''
    return ''.join(random.choice(string.ascii_lowercase + string.digits) for _ in range(length))


def repair_wrapper(keyspace, start_token, end_token):
    '''
    A repair wrapper
    '''
    one_hour = 3600
    req_id = generate_random_key(5)
    LOGGER.info("Keyspace: %s Start token: %s End token: %s",
                keyspace, start_token, end_token)
    repair_proc = Popen(['nodetool', 'repair', keyspace, '-full', '-j', '4',
                         '-seq', '-st', start_token, '-et',
                         end_token], stdout=PIPE, stderr=PIPE)
    # python3.4 has args in Popen
    # pylint is mistaken here
    LOGGER.info(req_id + " " + ' '.join(repair_proc.args))  # pylint: disable=no-member
    try:
        std_out, std_err = repair_proc.communicate(timeout=one_hour)
        LOGGER.debug(req_id + " " + str(std_out.decode()))
        if std_err:
            LOGGER.error(req_id + " " + str(std_err.decode()))
        error = re.search('Repair session .* failed with error', str(std_out.decode()))
        if error:
            with open(MON_FILE, 'w') as mon_file:
                mon_file.write("%s Keyspace: %s Error: %s\n" %
                               (datetime.datetime.now(), keyspace, error.group(0)))
    except TimeoutExpired as ex:
        LOGGER.error("%s timed out: %s", req_id, ex)


def repair(ranges):
    '''
    Do a repair on a token range
    '''
    for keyspace in ranges:
        LOGGER.info("Starting repair for keyspace %s", keyspace)
        with Pool(1) as pool:
            pool.starmap(repair_wrapper, ranges[keyspace])
    LOGGER.info("Repair by range finished")


def compact():
    '''
    A compact wrapper
    '''
    LOGGER.info("Start global compact")
    proc = Popen(['nodetool', 'compact'], stdout=PIPE, stderr=PIPE)
    std_out, std_err = proc.communicate()
    LOGGER.info("stdout: " + str(std_out.decode()))
    if std_err:
        LOGGER.error("stderr: " + str(std_err.decode()))


def rebuild(keyspaces):
    '''
    A rebuild wrapper
    '''
    for keyspace in keyspaces:
        LOGGER.info("Start keyspace: %s", keyspace)
        rebuild_proc = Popen(['nodetool', 'rebuild', '--keyspace', keyspace],
                             stdout=PIPE, stderr=PIPE)
        std_out, std_err = rebuild_proc.communicate()
        LOGGER.info("stdout: " + str(std_out.decode()))
        if std_err:
            LOGGER.error("stderr: " + str(std_err.decode()))


def get_ranges(keyspaces_list):
    '''
    Get ranges for specified keyspaces.
    Return dict of keyspaces and their start range, end range pairs
    '''
    ranges = {}
    for keyspace in keyspaces_list:
        LOGGER.info("Getting ranges for keyspace %s", keyspace)
        command = ["nodetool", "describering", keyspace]
        process = Popen(command, stdout=PIPE, stderr=PIPE)
        ring, err = process.communicate()
        if err:
            LOGGER.fatal(err.decode())
            sys.exit()
        for line in ring.decode().split("\n"):
            token = re.search('start_token:((-)?[0-9]+), end_token:((-)?[0-9]+)', line)
            if token:
                try:
                    ranges[keyspace].append((keyspace, token.group(1), token.group(3)))
                except KeyError:
                    ranges[keyspace] = [(keyspace, token.group(1), token.group(3))]
    return ranges

def lock_instance(lock_fd):
    fcntl.flock(lock_fd, fcntl.LOCK_NB|fcntl.LOCK_EX)

def create_pid(pidfile, pid):
    '''
    Check for pidfile, exit if exists, else create.
    '''
    if os.path.isfile(pidfile):
        LOGGER.fatal("%s already exists, exiting", pidfile)
        sys.exit()
    with open(pidfile, 'w') as pid_file:
        pid_file.write(pid)


def parse_args(print_help=False):
    '''
    Parse args
    '''
    parser = argparse.ArgumentParser(description="Repair a range of token on a \
                                                    host or rebuild a host.")
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("-r", "--repair", dest="repair", required=False,
                       action="store_true", help="Repair the host")
    group.add_argument("-c", "--compact", dest="compact", required=False,
                       action="store_true", help="Compact the host")
    group.add_argument("-b", "--rebuild", dest="rebuild", required=False,
                       action="store_true", help="Rebuild the host")
    group.add_argument("-m", "--monrun", dest="monrun", required=False,
                       action="store_true", help="Display a monrun check")
    group.add_argument("--rotate", dest="rotate", required=False,
                       action="store_true", help="Rotate monitoring log")
    parser.add_argument("-q", "--quite", dest="quite", required=False,
                        action="store_true", help="Don't output to stdin")
    if print_help:
        return parser.print_help()
    results = parser.parse_args()
    return results


if __name__ == '__main__':
    args = parse_args()  # pylint: disable=invalid-name
    if args.rotate:
        rotate_monfile(MON_FILE)
        sys.exit()
    if args.monrun:
        monrun_output(MON_FILE, PIDFILE, MAX_REPAIR_TIME)
        sys.exit()
    if args.quite:
        LOGGER.removeHandler(CONSOLE)
    rotate_monfile(MON_FILE)
    KEYSPACES = config(CONFIG)
    if args.repair:
        create_pid(PIDFILE, PID)
        lock_fd = open(LOCKFILE, 'w')
        lock_instance(lock_fd)
        try:
            RANGES = get_ranges(KEYSPACES)
            repair(RANGES)
        finally:
            os.unlink(PIDFILE)
            os.unlink(LOCKFILE)
    elif args.compact:
        create_pid(PIDFILE, PID)
        lock_fd = open(LOCKFILE, 'w')
        lock_instance(lock_fd)
        try:
            compact()
        finally:
            os.unlink(PIDFILE)
            os.unlink(LOCKFILE)
    else:
        lock_fd = open(LOCKFILE, 'w')
        lock_instance(lock_fd)
        try:
            rebuild(KEYSPACES)
        finally:
            os.unlink(PIDFILE)
            os.unlink(LOCKFILE)
