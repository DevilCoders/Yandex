#!/usr/bin/pymds

import re
import os
import glob
import json
from mds.admin.library.python.sa_scripts import utils


def marked_space(group_dir):
    DEVICECONF_RE = re.compile("blob_size_limit ?= ?(\d+)G")
    size_alloc = 0
    device_conf_files = glob.glob("{0}/kdb/device.conf".format(group_dir))

    # mr_proper.py
    device_conf_files += glob.glob("{0}/removed/device.conf".format(group_dir))

    # http://cloud02h.mds.yandex.net/job/4b88a07d5aea425ebd64ff3f9e0b9156/
    # migrate_dst delete last task
    if not device_conf_files:
            device_conf_files += glob.glob("{0}/migrate_dst/device.conf".format(group_dir))
    device_conf_files += glob.glob("{0}/migrate/device.conf".format(group_dir))
    for conf in device_conf_files:
        with open(conf, 'r') as device_conf:
            for line in device_conf:
                m = re.match(DEVICECONF_RE, line)
                if m is None:
                    continue
                if len(glob.glob(conf.replace('device.conf', 'group?id*'))) == 0:
                    continue
                size_alloc += int(m.group(1)) * 1024 * 1024
                break
    return size_alloc


def find_unused_groups(root_dir):
    unused = []
    overcommit = []

    # Max unit size = 916Gb by agodin@
    MAX_UNIT_SIZE = 961322250  # kbytes

    dirs = os.listdir(root_dir)
    dirs.sort()

    for d in dirs:
        is_cache = d == 'cache'
        if not d.isdigit() and not is_cache:
            continue

        disk_dir = os.path.join(root_dir, d)
        if not os.path.isdir(disk_dir):
            continue

        if is_cache:
            if os.path.isdir(disk_dir+"/1"):
                diskstat = os.statvfs(disk_dir+"/1")
            else:
                continue
        else:
            diskstat = os.statvfs(disk_dir)
        total_kbytes = int((diskstat.f_bsize * diskstat.f_blocks) / 1024)

        std_unit_count = total_kbytes / MAX_UNIT_SIZE

        sp = 0
        try:
            group_dirs = os.listdir(disk_dir)
        except OSError:
            continue
        for g_dir in group_dirs:
            if not g_dir.isdigit():
                continue
            group_root_dir = os.path.join(disk_dir, g_dir)
            sp += marked_space(group_root_dir)

        if total_kbytes < sp:
            overcommit.append(disk_dir)
            continue

        free_space = total_kbytes - sp
        if free_space >= MAX_UNIT_SIZE:
            for free_unit_id in range(1, std_unit_count):
                group_root_dir = os.path.join(disk_dir, str(free_unit_id))
                if group_root_dir not in unused and marked_space(group_root_dir) == 0:
                    unused.append(group_root_dir)
                    free_space -= MAX_UNIT_SIZE
                if (free_space < MAX_UNIT_SIZE):
                    break

    return (unused, overcommit)


def hw_watcher_disk_failed():
    failed = False
    try:
        cache_file = '/var/cache/hw_watcher/disk.db'
        with open(cache_file, 'r') as f:
            cache = json.load(f)

        if cache['status'] == "FAILED":
            failed = True
    except Exception:
        pass

    return failed


def main():
    root_dir = utils.get_elliptics_root_dir()

    unused, overcommit = find_unused_groups(root_dir)
    code = 0
    msg = []
    msg_text = "OK"
    if unused:
        code = 1
        msg.append("Unused disks found: {0}".format(unused))
    if overcommit:
        code = 2
        msg.append("Overcommit on disks: {0}".format(overcommit))
    if msg:
        msg_text = "; ".join(msg)
    if hw_watcher_disk_failed():
        code = 0

    print "{0};{1}".format(code, msg_text)


if __name__ == '__main__':
    main()
