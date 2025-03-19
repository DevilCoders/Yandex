#!/usr/bin/env python3
"""
Ugly workaround for KERNEL-249, see MDB-5106 for details
"""

import logging
import subprocess
import time


def execute(cmd):
    """
    Wrapper for subprocess with continious stdout output
    """
    popen = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=open('/dev/null', 'w'),
        shell=True,
        universal_newlines=True)
    for stdout_line in iter(popen.stdout.readline, ""):
        yield stdout_line
    popen.stdout.close()
    return_code = popen.wait()
    if return_code:
        raise subprocess.CalledProcessError(return_code, cmd)


def get_free_mem_in_mb():
    """
    Function to get free mem in megabytes
    """
    with open('/proc/meminfo', 'r') as meminfo:
        for line in meminfo.readlines():
            if line.startswith('MemFree:'):
                free_mem_kb = int(line.split()[1])

    return round(free_mem_kb / 1024)


def log_dentry_info():
    """
    Function to log information about dentry
    """
    with open('/proc/slabinfo', 'r') as slabinfo:
        dentry_line = [
            line for line in slabinfo.readlines() if line.startswith('dentry')
        ][0] or None
        if dentry_line is None:
            raise Exception('Could not find info about dentry cache size')

    active_objs = int(dentry_line.split()[1])
    num_objs = int(dentry_line.split()[2])
    ratio = active_objs / num_objs if active_objs != 0 else 0
    logging.info('Dentry objects [active/total]: %d/%d (%.2f)', active_objs,
                 num_objs, ratio * 100)


def fill_cache():
    """
    Function to fill page cache with trash data from /data
    """
    cmd = "find /data -size +1M"
    logging.info(cmd)
    devnull = open('/dev/null', 'w')
    started_at = int(time.time())
    for path in execute(cmd):
        command = "dd if={path} of=/dev/null".format(path=path)
        logging.debug(command)
        subprocess.call(command.split(), stdout=devnull, stderr=devnull)

        now = int(time.time())
        if now - started_at > 10:
            started_at = now
            free_mem = get_free_mem_in_mb()
            logging.info("%d MiB of free memory", free_mem)
            if free_mem <= 1024:
                break


if __name__ == '__main__':
    logging.basicConfig(level=logging.INFO, format='%(message)s')
    log_dentry_info()
    fill_cache()
    log_dentry_info()
