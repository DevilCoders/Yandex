#!/usr/bin/env python3
import subprocess as sp
from typing import Union, List
import yaml
from yc_monitoring import JugglerPassiveCheck, JugglerPassiveCheckException


class FS(object):
    def __init__(self, name):
        self.name = name

    def stat(self):
        stat = {}
        cmdline = 'df -l --o {}'.format(self.name)
        proc = sp.Popen(cmdline.split(), stdout=sp.PIPE, stderr=sp.PIPE)
        out, err = proc.communicate()
        if err:
            raise PermissionError(err.decode("utf-8").rstrip('\n'))
        raw = out.decode("utf-8").split("\n")
        if raw:
            tokens = raw[1].split()
            stat['IUsed%'] = 0 if tokens[5][:-1] == '' else int(tokens[5][:-1])
            stat['SAvailBlocks'] = int(tokens[8])
            stat['SUsed%'] = 0 if tokens[9][:-1] == '' else int(tokens[9][:-1])
        return stat


def get_partitions(paths : List[str]) -> List[str]:
    """
    Gets all mounted filesystems from /proc/mounts
    :return part_list: list of mounted partitions or None if everything is broken
    """
    part_list = []
    try:
        with open('/proc/mounts') as mounts:
            for mount in mounts.readlines():
                mount = mount.split()
                device, mounted_on = mount[0], mount[1]
                if mounted_on in paths or device[:4] == '/dev':
                    part_list.append(mount[1])
        return part_list
    except Exception as error:
        raise JugglerPassiveCheckException("Could'n read /proc/mounts: {}".format(error))


def check_limits(
    check: JugglerPassiveCheck,
    item: str,
    soft: Union[int, float] = 0,
    hard: Union[int, float] = 0,
    relative: bool = True,
    ignore: bool = False,
):
    if ignore:
        check.ok('Partition {} is ignored'.format(item))
        return

    item_instance = FS(item)
    try:
        item_stat = item_instance.stat()
    except PermissionError as ex:
        check.warn('An error occured while using {}'.format(ex))
        return
    
    if relative:
        i_used = item_stat['IUsed%']
        s_used = item_stat['SUsed%']
        if hard > s_used >= soft:
            check.warn('Few free space on {}, {}% used'.format(item, s_used))
        elif s_used >= hard:
            check.crit('No free space on {}, {}% used'.format(item, s_used))
        if hard > i_used >= soft:
            check.warn('Few free inodes on {}, {}% used'.format(item, i_used))
        elif i_used >= hard:
            check.crit('No free inodes on {}, {}% used'.format(item, i_used))
    else:
        s_avail = item_stat['SAvailBlocks']
        if s_avail <= hard:
            check.crit('No free space on {}, {}G left'.format(item, convert2gigabytes(s_avail)))
        elif s_avail <= soft:
            check.warn('Few free space on {}, {}G left'.format(item, convert2gigabytes(s_avail)))


def convert2gigabytes(size: Union[int, float]) -> float:
    return size / 1024.0 / 1024.0


def convert2blocks(size: str) -> float:
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


def check_free_space(check: JugglerPassiveCheck):
    with open('freespace.yaml') as fd:
        limits = yaml.safe_load(fd)

    rel_limits = limits['global']
    for partition in get_partitions(rel_limits['mounts_in_addition_to_dev']):
        # snap loop mounts always full no need to check
        if partition.startswith("/snap"):
            continue

        if partition in limits:
            abs_limits = limits[partition]
            if abs_limits.get("ignore", False):
                check_limits(check, partition, ignore=True)
                continue

            check_limits(
                check,
                partition,
                soft=convert2blocks(abs_limits['absolute_warn']),
                hard=convert2blocks(abs_limits['absolute_crit']),
                relative=False,
            )
        # check relative limits
        check_limits(
            check,
            partition,
            soft=int(rel_limits['relative_warn'][:-1]),
            hard=int(rel_limits['relative_crit'][:-1]),
        )


def main():
    check = JugglerPassiveCheck("freespace")
    try:
        check_free_space(check)
    except JugglerPassiveCheckException as ex:
        check.crit(ex.description)
    except Exception as ex:
        check.crit("During check exception raised: ({}): {}".format(ex.__class__.__name__, ex))
        raise
    print(check)


if __name__ == '__main__':
    main()
