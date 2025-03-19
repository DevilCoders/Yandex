#!/usr/bin/python
"""
Small keepalived-like daemon
Pingunoque in keepalived-like daemon used to check hosts availability
and block disappeared IPs using ipset to minimize all network timeouts
"""

import threading
import signal
from pingunoque import log, ipset, watcher

def daemon():
    """Main thread"""

    ipset.flush_all()
    exit_event = threading.Event()
    watchers = watcher.WatchersPool()

    signal.signal(signal.SIGINT, lambda s, f: exit_event.set())
    signal.signal(signal.SIGTERM, lambda s, f: exit_event.set())
    signal.signal(signal.SIGHUP, watchers.notify_reload)

    log.info("Pingunoque starting")
    while True:
        watchers.reload()
        if exit_event.wait(1):
            log.info("Exiting")
            break

    watchers.stop()

    log.info("Main thread exiting")
