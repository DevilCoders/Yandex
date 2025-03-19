#!/usr/bin/env python
# -*- coding: utf-8 -*-
import logging
import sys


def get_log(filename='/var/log/redis/mdb-redisctl.log'):
    kwargs = dict(
        format='%(asctime)s [%(levelname)s] %(process)d %(module)s:\t%(message)s',
        level=logging.DEBUG,
    )
    if filename:
        kwargs['filename'] = filename
    logging.basicConfig(**kwargs)
    log = logging.getLogger("redisctl")

    syslogHandler = logging.StreamHandler()
    syslogHandler.setLevel(logging.WARNING)
    syslogHandler.setFormatter(logging.Formatter('[%(levelname)s] %(module)s:\t%(message)s'))
    log.addHandler(syslogHandler)
    return log


try:
    log = get_log()
except Exception as ex:
    sys.stderr.write("Failed to init log: {}\n".format(ex))
    # sys.exit(1)
    log = get_log("")
