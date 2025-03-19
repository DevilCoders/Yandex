#!/usr/bin/env python
# -*- coding: utf-8 -*-
import os

from shutil import copyfileobj
from .ctl_logging import log
from .command import CommandResult, ResultCode

STDOUT_PATH = '/dev/stdout'


def copy_file(source, destination):
    if destination == '-':
        destination = STDOUT_PATH
    try:
        with open(source, 'rb') as source_fd:
            with open(destination, 'wb') as destination_fd:
                log.debug("write %s -> %s", source_fd, destination_fd)
                copyfileobj(source_fd, destination_fd)
                log.debug("write finished")
    except Exception as ex:
        log.debug('failed to write %s', ex)
        return CommandResult(result_print='failed to write {}'.format(ex), result_code=ResultCode.ERROR)
    return CommandResult()


def get_fs_stat(path):
    return os.statvfs(path)


def get_total_size(path):
    st = get_fs_stat(path)
    return st.f_bsize * st.f_blocks


def get_free_space(path):
    st = get_fs_stat(path)
    return 1.0 * st.f_bavail * st.f_frsize


def get_disk_usage_percentage(path):
    total_size = get_total_size(path)
    free_space = get_free_space(path)
    return 100 * (1 - free_space / total_size)


def is_disk_full(path, disk_limit):
    disk_usage = get_disk_usage_percentage(path)
    return disk_usage >= disk_limit, disk_usage


def get_file_size(path):
    return 1.0 * os.path.getsize(path) if os.path.exists(path) else 0


def get_file_usage_percentage(path):
    total_size = get_total_size(path)
    filesize = get_file_size(path)
    return 100 * (filesize / total_size)


def is_file_large(path, file_limit):
    file_usage = get_file_usage_percentage(path)
    return file_usage >= file_limit, file_usage


def _remove_file(path):
    if not os.path.exists(path):
        return CommandResult(result_print="No {} already".format(path), result_code=ResultCode.SUCCESS)
    size_before = get_file_size(path)

    os.unlink(path)

    if os.path.exists(path):
        size_after = get_file_size(path)
        if size_after >= size_before > 0:
            return CommandResult(
                result_print="File {} is not removed somehow: size before {}, size after {}".format(
                    path, size_before, size_after
                ),
                result_code=ResultCode.ERROR,
            )
    return CommandResult(result_print="File {} successfully removed".format(path), result_code=ResultCode.SUCCESS)


def remove_file(path):
    result = _remove_file(path)
    log.debug(result.result_print)
    return result


def store_to_file(data, path):
    with open(path, 'w') as fobj:
        fobj.write(data)


def get_from_file(path):
    if not os.path.exists(path):
        return
    with open(path) as fobj:
        return fobj.read()
