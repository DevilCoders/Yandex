#!/usr/bin/pymds

import logging
import time
import psutil
import sys

from mds.admin.library.python.sa_scripts import utils


    # TODO: move to
    # https://github.yandex-team.ru/mds-storage/yandex-storage-sa-scripts ?
def check_dnet_ioserv():
    process = None
    for proc in psutil.process_iter():
        try:
            if proc.name() == "dnet_ioserv":
                process = proc
                if int(time.time()) - process.create_time() < 60 * 5:
                    print "0;dnet_ioserv start <5 minuts ago"
                    exit()
            # ['perl', '/usr/bin/ubic', 'status', 'elliptics']
            if ' '.join(proc.cmdline()[1:]) in ["/usr/bin/ubic stop elliptics", "/usr/bin/ubic restart elliptics"]:
                print "0;dnet_ioserv stops or restarts"
                exit()
        except psutil.NoSuchProcess:
            continue

    if process is None:
        print "0;dnet_ioserv pid file doesn't exist - probably elliptics is stopped"
        exit()


log_file = '/var/log/gandalf/monrun.log'
logger = logging.getLogger('mr_proper')
MAX_AGE_SEC = 10

_format = logging.Formatter("[%(asctime)s] [%(name)s] %(levelname)s: %(message)s")
_handler = logging.FileHandler(log_file)
_handler.setFormatter(_format)
logging.getLogger().setLevel(logging.DEBUG)
logging.getLogger().addHandler(_handler)

try:
    check_dnet_ioserv()

    data = utils.http_req_try('http://localhost:7323/unistat')
    # [[u'gandalf.node_stat.age_ms_ahhh', 526], [u'gandalf.node_stat.err_ammt', 0]]
    data = data.json()
    status = 0
    for d in data:
        if d[0] == 'gandalf.node_stat.err_ammt' and d[1] > 0:
            print "2;node_stat err count {}".format(d[1])
            sys.exit()
        if d[0] == 'gandalf.node_stat.age_ms_ahhh' and d[1] > MAX_AGE_SEC * 1000:
            print "2;node_stat age s {} (limit {} s)".format(round(d[1] / 1000.0, 2), MAX_AGE_SEC)
            sys.exit()
    print "0;Ok"
except Exception:
    logger.exception("Error")
    print "2;Error: check {} for details".format(log_file)
