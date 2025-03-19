#!/usr/bin/python

import os
import re
import time
import sys


def get_disk_usage(dir = None, avg_window = 1):

    if not dir:
        config = '/etc/mongodb.conf'
        if not os.path.isfile(config):
            config = '/etc/mongodb-default.conf'
        if not os.path.isfile(config):
            return (None, None, "cannot find mongodb config")

        cfg = open(config, 'r')
        for line in cfg:
            m = re.search('db[Pp]ath\s*[:=]\s*(.*)$', line)
            if m:
                dir = m.group(1)
                break
        cfg.close()

    #get dir mountpoint
    path = dir
    while not os.path.ismount(path):
        path = os.path.dirname(path)

    mounts = open('/proc/mounts', 'r')
    disk = None
    #get device where mongodb datadir is located
    #a bit hacky and will not work for lvm-based containers
    for m in mounts:
        (device, mount_point) = m.split(' ')[:2]
        if mount_point == path and device[:4] == '/dev':
            if(os.path.islink(device)):					#for disks mounted by UUID (/dev/disk/by-uuid/UUID -> ../../sda1)
                device=os.path.normpath(os.path.join(os.path.dirname(device), os.readlink(device)))
            m = re.match('\/dev\/(md\d+)', device)			#/dev/mdX
            if not m:
                m = re.match('\/dev\/([a-z]+)\d*', device)		#/dev/sda, /dev/vda1, /dev/sda2 etc
                if not m:
                    return (None, None, 'storage type not supported')
            disk = m.group(1)
            break
    mounts.close()

    if disk.find('mapper') >= 0:
        return (None, None, 'storage type not supported')

    try:
        sysfs = open('/sys/block/%s/queue/hw_sector_size' % disk)
    except:
        exception = sys.exc_info()
        error = "Couldn't get device sector size, got %s: %s" % (exception[0].__name__, exception[1])
        return (None, None, error)


    sector_size = int(sysfs.readline().rstrip())
    sysfs.close()

    try:
        sysfs = open('/sys/block/%s/stat' % disk)
    except:
        exception = sys.exc_info()
        error = "Couldn't get device statistic, got %s: %s" % (exception[0].__name__, exception[1])
        return (None, None, error)

    def get_rw_stats(sysfs):
        data = sysfs.readline().rstrip()
        stat_re = r'(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)'
        values = re.search(stat_re, data)
        (sectors_read, sectors_written) = [int(x) for x in (values.group(3), values.group(7))]
        return (sectors_read*sector_size/1024.0/1024.0, sectors_written*sector_size/1024.0/1024.0)

    (r1, w1) = get_rw_stats(sysfs)
    time.sleep(avg_window)
    sysfs.seek(0)
    (r2, w2) = get_rw_stats(sysfs)
    sysfs.close()

    return ((r2 - r1)/avg_window, (w2 - w1)/avg_window, 'ok')
