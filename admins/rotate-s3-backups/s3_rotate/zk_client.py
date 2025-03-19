"""
Zk client wrapper
"""

import os
import logging
import time

import requests
from dotmap import DotMap
from kazoo.client import KazooClient
from kazoo.exceptions import NoNodeError, BadVersionError, LockTimeout


class ZkCli(object):
    """Zookeeper related methods"""
    log = logging.getLogger(__name__)

    def __init__(self, conf):
        self.conf = conf
        self.url = "http://c.yandex-team.ru/api-cached/groups2hosts/{}"
        self.client = KazooClient(hosts=','.join(self.get_zk_hosts(conf.zk.group)))
        self.client.start()
        self.client.ensure_path(conf.zk.basepath)
        self.lock = self.client.Lock(
            "{}/{}/lock".format(self.conf.zk.basepath, conf.current_bucket)
        )
        if os.sys.stdin.isatty():
            self.log.debug("Run from console, ignore zk relevant period")
            self.rotate_relevant_zk_period = 1
        else:
            self.rotate_relevant_zk_period = conf.zk.safe_period

    def get_zk_hosts(self, group):
        """Get zk hosts from conductor cache"""
        url = self.url.format(group)
        resp = requests.get(url)
        retry = 3
        while resp.status_code not in (200, 404) and retry > 0:
            retry -= 1
            resp = requests.get(url)
        resp.raise_for_status()
        return resp.text.strip().split()

    def acquire_lock(self):
        "acquire zk lock"
        try:
            if not self.lock.acquire(blocking=True, timeout=self.conf.zk.timeout):
                return False
            stats = DotMap({"version": -1})
            try:
                (lasttime, stats) = self.client.get(self.conf.last_ts_path)
                lasttime = float(lasttime)
            except (ValueError, NoNodeError):
                lasttime = time.time() - self.rotate_relevant_zk_period

            if time.time() - lasttime < self.rotate_relevant_zk_period:
                self.log.debug(
                    "The last rotate was still relevant (rotate period %s)",
                    self.rotate_relevant_zk_period
                )
                return False
            self.client.ensure_path(self.conf.last_ts_path)
            self.client.set(self.conf.last_ts_path, str(time.time()).encode(), stats.version)
        except (BadVersionError, LockTimeout):
            return False
        return True

    def write(self, zk_path, value):
        """Write zk data"""
        if self.client.exists(zk_path):
            self.client.set(zk_path, value)
        else:
            self.client.create(zk_path, value, makepath=True)


    def rotate_mon_file(self, filename):
        """Rotate monitoring file"""
        rotate = self.conf.monitoring.rotate
        if self.client.exists(filename):
            rotate_to = "{}.{}".format(filename, 1)
            for num in range(rotate, 0, -1):
                rotate_from = "{}.{}".format(filename, num-1)
                rotate_to = "{}.{}".format(filename, num)
                if self.client.exists(rotate_from):
                    value, _ = self.client.get(rotate_from)
                    if not value:
                        continue
                    self.log.debug("Rotate monitoring data %s -> %s", rotate_from, rotate_to)
                    if self.conf.args.dry_run:
                        self.log.info(
                            "Dry run: write rotate_to %s, value, %s", rotate_to, value
                        )
                    else:
                        self.write(rotate_to, value)
            value, _ = self.client.get(filename)
            if self.conf.args.dry_run:
                self.log.info("Dry run: write rotate_to %s, value, %s", rotate_to, value)
            else:
                self.write(rotate_to, value)


    def stop(self):
        """Stop zk client"""
        self.lock.release()
        self.client.stop()
