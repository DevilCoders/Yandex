# -*- coding: UTF-8 -*-

import argparse
import json
import logging
import os
import signal
import sys
import time

from coredump import Minidump, Coredump, CoredumpError
from crash_info import CrashInfoStorage
from sender import Sender, SenderError

logger = logging.getLogger(__name__)


class CrashProcessor(object):
    EMPTY_QUEUE_WAIT_TIME = 5.0

    def __init__(self):
        super(CrashProcessor, self).__init__()
        self._logger = logger.getChild(self.__class__.__name__)
        self.storage = None
        self.stopping = False
        self.sender = None
        self.args = None
        self.config = {}

    def _parse_args(self):
        parser = argparse.ArgumentParser(description="Crash dump processor")
        parser.add_argument("--datadir", type=str, default="/var/tmp/breakpad", metavar="DIR")
        parser.add_argument("--aggregator-url", type=str,
                                default="http://cores.cloud-preprod.yandex.net", metavar="URL")
        parser.add_argument("--prj", type=str, default="nbs", help="Project tag (default: nbs)")
        parser.add_argument("--config", type=str, metavar="PATH")
        parser.add_argument("--verbose", action="store_true")

        # TODO: remove, kept for backward compatibility
        parser.add_argument("--nbs-config", type=str, metavar="PATH")
        self.args = parser.parse_args()

    def set_signals(self):
        signal.signal(signal.SIGINT, self.on_signal)
        signal.signal(signal.SIGTERM, self.on_signal)
        signal.signal(signal.SIGHUP, self.on_signal)

    def on_signal(self, signum, _):
        self._logger.info("signal %s received", signum)
        self.stopping = True

    def process_oom(self, crash_info):
        try:
            self.sender.send(
                timestamp=crash_info.time,
                coredump="Thread 1 (LWP 0):\n#0  0x0000000000000000 in main () at main.cc:0\n",
                info=crash_info.info,
                service_name=crash_info.service,
                crash_type=Sender.CRASH_TYPE_OOM,
                logfile=crash_info.logfile,
            )
        except SenderError:
            pass

    def process_coredump(self, crash_info):
        try:
            if crash_info.is_minidump:
                core = Minidump(crash_info.corefile)
            else:
                core = Coredump(crash_info.corefile)

            backtrace = core.backtrace
            service_name = core.service

            self.sender.send(
                timestamp=crash_info.time,
                coredump=backtrace,
                info=crash_info.info,
                service_name=service_name,
                crash_type=Sender.CRASH_TYPE_CORE,
                logfile=crash_info.logfile,
            )

        except CoredumpError as e:
            self._logger.error("Can't process core %s %r", crash_info.corefile, e)
            self._logger.debug("Exception", exc_info=True)
            return

        except SenderError:
            return

        core.cleanup()

    def load_crash_info(self):
        while not self.stopping:
            crash_info = self.storage.get()
            if crash_info:
                if not crash_info.corefile:
                    self.process_oom(crash_info)
                else:
                    self.process_coredump(crash_info)
            else:
                time.sleep(self.EMPTY_QUEUE_WAIT_TIME)

    def load_config(self):
        if not self.args.nbs_config and not self.args.config:
            return

        try:
            config = self.args.nbs_config if self.args.nbs_config else self.args.config
            self._logger.info("Load config from %s", config)
            with open(config, "r") as fd:
                self.config = json.load(fd)
        except (IOError, OSError) as e:
            self._logger.error("Error reading config %r", e)

    def get_config_emails(self):
        notify_emails = self.config.get("notify_email")
        emails = notify_emails.split(',') if notify_emails else list()

        unique_emails = list(set(emails))
        if unique_emails:
            self._logger.info("Notify emails: %r", unique_emails)

        return unique_emails

    def get_aggregator_url(self):
        url = self.config.get("aggregator_url")
        return url if url else self.args.aggregator_url

    def init(self):
        self.set_signals()
        self.storage = CrashInfoStorage(os.path.join(self.args.datadir, "queue"))

        self.load_config()
        emails = self.get_config_emails()
        url = self.get_aggregator_url()

        self.sender = Sender(url, self.args.prj, emails)

    def run(self):
        logging.basicConfig(format="%(asctime)s - %(levelname)s - %(message)s", level=logging.INFO)
        self._parse_args()
        if self.args.verbose:
            logging.getLogger().setLevel(logging.DEBUG)
        try:
            self.init()
            self.load_crash_info()
        except KeyboardInterrupt:
            return 0
        except Exception as e:
            self._logger.error("%r", e)
            self._logger.debug("Exception", exc_info=True)
            return 1
        return 0


def main():
    sys.exit(CrashProcessor().run())
