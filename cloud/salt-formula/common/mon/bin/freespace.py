#!/usr/bin/env python
from __future__ import print_function

import subprocess as sp
import re
import ConfigParser

class FS(object):
    def __init__(self, name):
        self.name = name

    def stat(self):
        stat = {}
        cmdline = 'df -l --o {}'.format(self.name)
        raw = sp.Popen(cmdline.split(), stdout=sp.PIPE).stdout.readlines()
        if raw:
            tokens = raw[-1].split()
            stat['IUsed%'] = int(tokens[5][:-1])
            stat['SAvailBlocks'] = int(tokens[8])
            stat['SUsed%'] = int(tokens[9][:-1])
        return stat


class CheckResult(object):
    STATUS_OK = 0
    STATUS_WARN = 1
    STATUS_CRIT = 2

    def __init__(self):
        self.statuses = []
        self.messages = []

    def add_result(self, status, msg):
        self.statuses.append(status)
        self.messages.append(msg)

    def ok(self, msg='OK'):
        self.add_result(self.STATUS_OK, msg)

    def warn(self, msg):
        self.add_result(self.STATUS_WARN, msg)

    def crit(self, msg):
        self.add_result(self.STATUS_CRIT, msg)



def m_exit(s, m):
    """
    Simple monitoring wrapper
    :param s: list, status code 0 for OK, 1 for warning, 2 for critical
    :param m: list, description
    :return str, joined status and message
    """
    print("PASSIVE-CHECK:freespace;{};{}".format(max(s), ', '.join(m)))
    exit()


def get_partitions():
    """
    Gets all mounted filesystems from /proc/mounts
    :return part_list: list of mounted partitions or None if everything is broken
    """
    part_pattern = re.compile(r'(^/dev/.*)\s+', re.MULTILINE)
    part_list = None
    try:
        with open('/proc/mounts') as mounts:
            part_list = [''.join(el.split()[1:2]) for el in re.findall(part_pattern, mounts.read())]
    except Exception as error:
        m_exit([2], "Could'n read /proc/mounts: {}".format(error))
    return part_list


def check_limits(item, soft=0, hard=0, relative=True, ignore=False):
    item_instance = FS(item)
    item_stat = item_instance.stat()
    result = CheckResult()
    if ignore:
       result.ok('Partition {} is ignored'.format(item))
    elif relative:
        i_used = item_stat['IUsed%']
        s_used = item_stat['SUsed%']
        if s_used < soft:
            result.ok()
        elif hard > s_used >= soft:
            result.warn('Few free space on {}, {}% used'.format(item, s_used))
        elif s_used >= hard:
            result.crit('No free space on {}, {}% used'.format(item, s_used))
        if i_used < soft:
            result.ok()
        elif hard > i_used >= soft:
            result.warn('Few free inodes on {}, {}% used'.format(item, i_used))
        elif i_used >= hard:
            result.crit('No free inodes on {}, {}% used'.format(item, i_used))
    else:
        s_avail = item_stat['SAvailBlocks']
        if s_avail <= hard:
            result.crit('No free space on {}, {}G left'.format(item, convert2gigabytes(s_avail)))
        elif s_avail <= soft:
            result.warn('Few free space on {}, {}G left'.format(item, convert2gigabytes(s_avail)))
        else:
            result.ok()

    return result


def convert2gigabytes(size):
    return size / 1024.0 / 1024.0


def convert2blocks(size):
    """
    Converts human readable size to number of blocks
    :param size: str, human readable size of partition
    """
    if size[-1] == "M":
        return float(size[:-1].replace(',', '.')) * 1024
    if size[-1] == "G":
        return float(size[:-1].replace(',', '.')) * 1024 * 1024
    if size[-1] == "T":
        return float(size[:-1].replace(',', '.')) * 1024 * 1024 * 1024


def main():
    status, message = [], []
    cfgpath = '/home/monitor/agents/etc/freespace.conf'
    config = ConfigParser.ConfigParser()
    config.read(cfgpath)
    limits = dict((section, dict((option, config.get(section, option))
                                 for option in config.options(section)))
                  for section in config.sections())
    rel_limits = limits['global']

    for partition in get_partitions():
        if partition in limits:
            abs_limits = limits[partition]
            if abs_limits.get("ignore", False):
                ignore_check = check_limits(partition, ignore=True)
                status.extend(ignore_check.statuses)
                message.extend(ignore_check.messages)
                continue

            absolute_check = check_limits(partition,
                                          convert2blocks(abs_limits['absolute_warn']),
                                          convert2blocks(abs_limits['absolute_crit']), False)
            status.extend(absolute_check.statuses)
            message.extend(absolute_check.messages)
        # check relative limits
        relative_check = check_limits(partition,
                                      int(rel_limits['relative_warn'][:-1]),
                                      int(rel_limits['relative_crit'][:-1]))
        status.extend(relative_check.statuses)
        message.extend(relative_check.messages)

    if max(status) != 0:
        message = filter(lambda x: True if x != 'OK' else False, message)
    else:
        message = list(set(message))

    m_exit(status, message)


if __name__ == '__main__':
    main()
