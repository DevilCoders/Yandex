"""Various system utilities."""

import os

from yc_common.exceptions import Error


def get_open_fds(ignore_stdio=False, ignore_fds=()):
    fds = []

    if ignore_stdio:
        ignore_fds = tuple(ignore_fds) + (0, 1, 2)

    try:
        for fd in os.listdir("/proc/self/fd"):
            fd = int(fd)
            if fd not in ignore_fds:
                fds.append(fd)
    except Exception as e:
        raise Error("Unable to get a list of opened file descriptors from procfs: {}.", e)

    return fds
