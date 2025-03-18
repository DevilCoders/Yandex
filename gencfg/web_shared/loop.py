#!/usr/bin/env python

from time import sleep, time
import os
import sys
import subprocess
import threading
import logging
from argparse import ArgumentParser
import datetime

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))
import gencfg
import gaux.aux_utils
import config
from core.settings import SETTINGS

from exit_msg import ExitMessageFile, ExitCodes

DATA_UPDATE_TIMEOUT = 15
PRECALC_CACHE_TIMEOUT = 15

# shared in multithread variables
IS_RUNNING = True
LOGGER = None


def run_cmd(cmd):
    dev_null_read = open('/dev/null', 'r')
    dev_null_write = open('/dev/null', 'w')
    obj = subprocess.Popen(cmd, stdin=dev_null_read, stdout=dev_null_write, stderr=sys.stdout)
    obj.communicate()
    return obj.returncode


def run_cmd_no_throw(cmd):
    returncode = 1
    try:
        returncode = run_cmd(cmd)
    except:
        pass
    return returncode


def with_pid(s):
    return "[%s] [%s] %s" % (os.getpid(), os.getppid(), s)


class PrecalcThread(threading.Thread):
    def __init__(self, port, db_type):
        threading.Thread.__init__(self)
        self.port = port
        self.db_type = db_type
        assert (db_type in ["trunk", "unstable", "tags"])

    def run(self):
        global IS_RUNNING
        global LOGGER

        LOGGER.info(with_pid("[precalc] Precalc caches looper started"))

        if self.db_type == "trunk":
            prefix = "trunk"
        elif self.db_type == "unstable":
            prefix = "unstable"
        else:
            prefix = "tags/recent"
        url = 'http://localhost:%s/%s/precalc_caches' % (self.port, prefix)

        last_time = time()
        while IS_RUNNING:
            try:
                LOGGER.debug(with_pid("[precalc] Running next update cycle"))

                cur_time = time()
                if cur_time - last_time >= PRECALC_CACHE_TIMEOUT:
                    run_cmd_no_throw(['curl', '-s', '-X', 'GET', url])
                    last_time = cur_time
            except Exception, e:
                LOGGER.error(with_pid("[precalc] Got exception %s" % (e,)))

            sleep(1)

        LOGGER.info(with_pid("[precalc] Precalc caches looper finished"))


def parse_cmd():
    BACKEND_TYPES = ["wbe", "api"]
    DB_TYPES = ["trunk", "unstable", "tags"]

    parser = ArgumentParser(description="Control (wbe|api)/main.py: restart, send autoupdate requests")
    parser.add_argument("-b", "--backend-type", type=str, required=True,
                        choices=BACKEND_TYPES,
                        help="Obligatory. Backend type")
    parser.add_argument("-p", "--port", type=int, required=True,
                        help="Obligatory. Port to start backend on")
    parser.add_argument("--db", type=str, default="trunk",
                        choices=DB_TYPES,
                        help="Optional. Db type (trunk by default)")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options, unknown_options = parser.parse_known_args()
    options.suboptions = unknown_options

    return options


def _update():
    try:
        # Reset main repository to latest revision of the tracking branch
        repo = gaux.aux_utils.get_main_repo()
        repo.update_remote()
        repo.reset_all_changes(ref=repo.remote_ref())

        # DB repository is reset to the latest revision on each service start
    except Exception as e:
        print "Failed to update the repository:", e


def main(options):
    global LOGGER
    global IS_RUNNING

    LOGGER = logging.getLogger('loop_logger')

    fname = os.path.join(SETTINGS.looper.logging.logdir, SETTINGS.looper.logging.logtpl % dict(port=options.port))
    fname = os.path.abspath(fname)
    gaux.aux_utils.setup_logger_logfile(LOGGER, fname, logging.INFO,
                                       log_format="[%(asctime)s] [%(levelname)s] %(message)s")

    fname = os.path.join(SETTINGS.looper.logging.logdir, SETTINGS.looper.logging.dbglogtpl % dict(port=options.port))
    fname = os.path.abspath(fname)
    gaux.aux_utils.setup_logger_logfile(LOGGER, fname, logging.DEBUG,
                                       log_format="[%(asctime)s] [%(levelname)s] %(message)s")

    LOGGER.setLevel(logging.DEBUG)

    LOGGER.info(with_pid("[main] Started"))

    precalcer = None
    updater = None

    try:
        n = 0
        precalcer = PrecalcThread(options.port, options.db)
        precalcer.start()

        while IS_RUNNING:
            LOGGER.info(with_pid("[main] Restarting %s-nth time" % n))
            n += 1

            is_failed_by_exception = False

            ExitMessageFile.reset()
            run_cmd_no_throw(['./%s_backend/main.py' % options.backend_type, "--port", str(options.port), "--db",
                              options.db] + options.suboptions)
            ret = ExitMessageFile.get_code()

            LOGGER.info(with_pid("[main] Backend exited with status %s" % ret))

            if ret == ExitCodes.NORMAL:
                pass
            elif ret == ExitCodes.DIE:
                IS_RUNNING = False
            elif ret == ExitCodes.UPDATE_ALL:
                _update()
            else:
                LOGGER.exception(with_pid("[main] Backend failed by exception"))
                is_failed_by_exception = True

            if is_failed_by_exception:
                # let's make a pause to prevent high resource usage
                sleep(1)
    finally:
        LOGGER.info(with_pid("[main] Finalizing"))

        IS_RUNNING = False
        if updater is not None:
            LOGGER.info(with_pid("[main] Finalizing updater started"))
            updater.join()
            LOGGER.info(with_pid("[main] Finalizing updater finished"))
        if precalcer is not None:
            LOGGER.info(with_pid("[main] Finalizing precalcer started"))
            precalcer.join()
            LOGGER.info(with_pid("[main] Finalizing precalcer finished"))


if __name__ == '__main__':
    options = parse_cmd()
    main(options)
