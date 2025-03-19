import sys
import logging
import requests
from kazoo.client import KazooClient
from kazoo.handlers.eventlet import SequentialEventletHandler, TimeoutError


class ZK(KazooClient):                      # pylint: disable=R0903
    def __init__(self, conf):
        self.log = logging.getLogger(self.__class__.__name__)

        # make a connection string
        resp = requests.get(conf.hosts_url)
        if resp.status_code != 200:
            raise RuntimeError('Cannot get zookeeper host list')
        hosts = resp.text.split()
        conn_string = ','.join('{}:{}'.format(x, conf.port) for x in hosts)
        self.log.debug('The conneciton string is %s', format(conn_string))

        self.conf = conf
        super(ZK, self).__init__(hosts=conn_string,
                                 timeout=conf.timeout,
                                 handler=SequentialEventletHandler())

        try:
            self.start(timeout=self.conf.timeout)
        except TimeoutError:
            self.log.fatal('The zookeeper is unreachable')
            sys.exit(-1)
        self.ensure_path(self.conf.prefix)
        self.chroot = self.conf.prefix
