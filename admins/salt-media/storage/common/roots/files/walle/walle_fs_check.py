#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Provides: walle_fs_check

import os
import json
import logging
import socket
import time
import subprocess
from collections import defaultdict

__author__ = 'Igor Miheenko'
__copyright__ = 'Copyright 2017, Yandex LLC'
__email__ = 'affe@yandex-team.ru'

logging.basicConfig(format='[%(asctime)s] %(levelname)-8s %(filename)s:%(lineno)-3d> (%(funcName)s) %(message)s',
                    # level=logging.DEBUG)
                    )
log = logging.getLogger(__name__)

# will reboot host if error count it greater or equal to this threshold.
EXT4_ERROR_THRESHOLD = 1


DEV_STATUS_FAILED = 'failed'
DEV_STATUS_SUSPECTED = 'suspected'
DEV_STATUS_OK = 'ok'

SYSTEM_MOUNT_POINTS = {"/", "/home", "/place", "/ssd"}


def should_trigger(error_count):
    # temporary solution, see https://st.yandex-team.ru/WALLE-1786#1523520968000
    EXT4_UNCONDITIONAL_TRIGGER_THRESHOLD = 3
    EXT4_TRIGGER_SLICE = 10

    if error_count >= EXT4_UNCONDITIONAL_TRIGGER_THRESHOLD:
        return True
    if hash(socket.gethostname()) % 100 < EXT4_TRIGGER_SLICE:
        return True

    return False


class MountRecord(object):
    def __init__(self, row):
        DEV, MOUNT_POINT, FS_TYPE, OPTIONS = 0, 1, 2, 3

        tokens = row.split()
        self.dev = tokens[DEV]
        self.path_tokens = tokens[DEV].split("/")

        self.mount_point = tokens[MOUNT_POINT]
        self.fs_type = tokens[FS_TYPE]
        self.options = set(tokens[OPTIONS].split(","))

    def is_ext4(self):
        return self.fs_type == "ext4"

    def is_loop_device(self):
        return self.path_tokens[-1].startswith("loop")

    def is_uuid(self):
        return self.path_tokens[-2] == "by-uuid"

    def is_lvm(self):
        return self.path_tokens[-2] == "mapper" or os.path.realpath(self.dev).startswith("/dev/dm-")

    def is_readonly(self):
        return "ro" in self.options

    def is_system(self, system_mount_points=SYSTEM_MOUNT_POINTS):
        return self.mount_point in system_mount_points


class Checker(object):
    def __init__(self):
        super(Checker, self).__init__()

    def get_ext4_partitions(self):
        filename = '/proc/mounts'
        devices = defaultdict(list)

        for record in self.read_records(filename):
            if not record.is_ext4():
                continue

            if record.is_loop_device():
                continue

            if record.is_uuid():
                dev_name = self.resolve_dev_by_uuid(record.dev)

                if dev_name is None:
                    log.debug("Failed to resolve device by uuid: %s", record.dev)
                    continue

            elif record.is_lvm():
                dev_name = self.resolve_dev_by_mapper(record.dev)

                if dev_name is None:
                    log.debug("Failed to resolve device by mapper: %s", record.dev)
                    continue

            else:
                dev_name = record.path_tokens[-1]

            devices[dev_name].append(record)

        return devices

    def resolve_dev_by_uuid(self, path):
        try:
            _raw_part = os.readlink(path)
            return _raw_part.split('/')[-1]
        except OSError as e:
            log.debug(e)
            return self.run_blkid(path.split('/')[-1])

    def resolve_dev_by_mapper(self, path):
        try:
            _raw_part = os.readlink(path)
            return _raw_part.split('/')[-1]
        except OSError as e:
            log.debug(e)
            return self.run_blkid(path.split('/')[-1])

    @staticmethod
    def run_blkid(pattern):
        log.debug('pattern = %s', pattern)
        _part = None
        _raw_output = None
        _command = ['blkid', '-U', pattern]
        try:
            _raw_output = subprocess.check_output(_command)
        except subprocess.CalledProcessError as e:
            log.debug(e)
        if _raw_output:
            _part_full = _raw_output.strip()
            _part = _part_full.split('/')[-1]
        log.debug('_part = %s', _part)
        return _part

    def read_errors(self, ext4_partitions):
        count_dict = {}
        for part in ext4_partitions:
            _path = '/sys/fs/ext4/{}/errors_count'.format(part)
            try:
                raw_data = self.readfile(_path)
            except IOError as e:
                log.debug(e)
                continue
            try:
                count_dict.update({part: int(raw_data)})
            except ValueError:
                log.debug("Invalid error count for partition %s: %s", part, raw_data)
                continue

        log.debug('count_dict = %s', count_dict)
        return count_dict

    def genering_dict(self, partitions, count_dict):
        device_list = []
        crit_errors = 0
        warn_errors = 0
        for part, records in partitions.iteritems():
            error_count = count_dict.get(part, -1)
            device, crit, warn = self.find_error(part, error_count, records)
            crit_errors += crit
            warn_errors += warn
            device_list.append(device)

        self.sort_devices(device_list)
        while True:
            output_msg = json.dumps({'result': {'device_list': device_list}, 'timestamp': time.time()})
            if len(output_msg) > 1000:
                device_list.pop()
                continue
            else:
                break

        if crit_errors:
            exit_num = 2
        elif not crit_errors and warn_errors:
            exit_num = 1
        else:
            exit_num = 0
        return exit_num, output_msg

    @staticmethod
    def sort_devices(device_list):
        # This can be achieved with stable dictionary sort, but it doesn't worth the lines of code.
        status_ordered = [DEV_STATUS_FAILED, DEV_STATUS_SUSPECTED, DEV_STATUS_OK]
        status_enumerated = dict((status, i) for i, status in enumerate(status_ordered))

        device_list.sort(key=lambda dev: status_enumerated[dev['status']])

    @staticmethod
    def find_device_with_status(device_list, status):
        for i, device in enumerate(device_list):
            if device["status"] == status:
                return i

    @staticmethod
    def find_error(partition, error_count, records):
        crit = warn = 0
        device = {'name': partition, 'error_count': error_count, 'threshold': EXT4_ERROR_THRESHOLD}

        if error_count >= EXT4_ERROR_THRESHOLD:
            if should_trigger(error_count):
                crit = 1
            else:
                warn = 1
            message = 'partition {} has {} errors (threshold is {})'.format(
                partition, error_count, EXT4_ERROR_THRESHOLD)
            device.update(status=DEV_STATUS_FAILED, message=message)
        elif any(rec.is_readonly() for rec in records if rec.is_system()):
            crit = 1
            message = 'partition {} mounted readonly'.format(partition)
            device.update(status=DEV_STATUS_FAILED, message=message)
        elif 0 < error_count < EXT4_ERROR_THRESHOLD:
            warn = 1
            message = 'partition {} has {} errors (threshold is {})'.format(
                partition, error_count, EXT4_ERROR_THRESHOLD)
            device.update(status=DEV_STATUS_OK, message=message)
        elif error_count == -1:
            warn = 1
            message = 'cannot read errors_count on {}'.format(partition)
            device.update(status=DEV_STATUS_SUSPECTED, message=message)
        else:
            device.update(status=DEV_STATUS_OK, message='no errors')

        return device, crit, warn

    @staticmethod
    def read_records(filename):
        with open(filename, 'r') as f:
            for row in f:
                yield MountRecord(row)

    @staticmethod
    def readfile(filename):
        with open(filename, 'r') as f:
            return f.read().strip()

    def run(self):
        ext4_partitions = self.get_ext4_partitions()
        count_dict = self.read_errors(ext4_partitions)
        exit_num, output_msg = self.genering_dict(ext4_partitions, count_dict)
        print 'PASSIVE-CHECK:walle_fs_check;{};{}'.format(exit_num, output_msg)


def main():
    log.info('Starting.')
    worker = Checker()
    worker.run()


if __name__ == '__main__':
    main()
