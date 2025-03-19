#!/usr/bin/env python
# -*- coding: utf-8 -*-

'''
This module get info about disks
It is fixed copy-paste from https://a.yandex-team.ru/arc/adm/saltstack/salt/search_runtime/_grains/points_info.py
Some details could be found in https://st.yandex-team.ru/RUNTIMECLOUD-1708
'''
import os
import subprocess
import json
import re

default_dir = '/data'
sata_dir = '/slow'

def _check_disk_type(disk):
    name = disk.split('/')[-1]
    with open('/sys/block/{0}/queue/rotational'.format(name)) as f:
        check_ssd = int(f.read())
    if check_ssd == 0:
        if name.startswith('nvme'):
            return "nvme"
        else:
            return "ssd"
    else:
        return "hdd"

def _get_mount_point(path):
    path = os.path.abspath(path)
    while not os.path.ismount(path):
        path = os.path.dirname(path)
    return path

def _get_device(path):
    info = []
    devices = []
    with open('/proc/mounts') as f:
       info = f.readlines()
    for line in info:
        line_arr = line.split()
        if path == line_arr[1]:
            devices.append(line_arr[0])

    if devices[0] == 'rootfs':
        return os.path.realpath(devices[1])
    else:
        return os.path.realpath(devices[0])

def _get_raid_info(device):
    # disks = list_disks()
    type = None
    disks = []
    mddev = device.split('/')[-1]
    with open('/proc/mdstat') as mdstat:
        for line in mdstat.readlines():
            if not line.startswith('md'):
                continue
            line_arr = line.split()
            if line_arr[0] != mddev:
                continue
            # md_name = line_arr[0]
            md_type = line_arr[3]
            md_partisipant = []
            md_disks_type = []
            for disk in line_arr[4:]:
                # disk is sdh3[7], nvme0n1p1[0] remove [0]
                md_partisipant.append('/dev/' + re.sub('\[[0-9]+\]', '', disk))
                # if disks[disk[:3]]['ssd'] not in md_disks_type:
                #     md_disks_type.append(disks[disk[:3]]['ssd'])
        return (md_type, md_partisipant)

def _get_path_info(path):
    (mount_point, dev_point, raid_type, disks_info) = (None, None, None, [])
    mount_point = _get_mount_point(path)
    dev_point = _get_device(mount_point)
    if dev_point.split('/')[-1].startswith('md'):
        (raid_type, disks) = _get_raid_info(dev_point)
        for disk in disks:
            if disk.startswith('/dev/sd'):
                disk = re.search('sd[a-z]*', disk).group(0)
            if disk.startswith('/dev/nvme'):
                disk = disk[5:12]
            disk_type = _check_disk_type(disk)
            disks_info.append({disk: {'disk_type': disk_type}})
    # elif dev_point.split('/')[-1].startswith('sd'):
    else:
        disk = None
        if dev_point.startswith('/dev/sd'):
            disk = dev_point[5:8]
        if dev_point.startswith('/dev/nvme'):
            disk = dev_point[5:12]
        if disk:
            disk_type = _check_disk_type(disk)
            disks_info.append({disk: {'disk_type': disk_type}})

    return (mount_point, dev_point, raid_type, disks_info)


def path_info():
    '''
    Return info about disks in special points
    '''

    points_info = {}

    if os.path.exists(default_dir):
        (mount_point, dev_point, raid_type, disks_info) = _get_path_info(default_dir)
        points_info[default_dir] = {
                      'mount_point': mount_point,
                      'device': dev_point,
                      'raid': raid_type,
                      'disks_info': disks_info
                    }

    if os.path.exists(sata_dir):
        (mount_point, dev_point, raid_type, disks_info) = _get_path_info(sata_dir)
        points_info[sata_dir] = {
                      'mount_point': mount_point,
                      'device': dev_point,
                      'raid': raid_type,
                      'disks_info': disks_info }

    return {'points_info': points_info}
