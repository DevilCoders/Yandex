"""Poller module"""
import logging
import time
import re
import os
from collections import defaultdict, deque
import eventlet
import greenlet
from ..mysql import MySQL, MySQLError

class Poller(object):  # pylint: disable=too-many-instance-attributes
    """
    Connects to MySQL and maintains regular polls.
    Pushes all the data via the callback to the caller
    """
    def __init__(self, conf):
        self.log = logging.getLogger(self.__class__.__name__)
        self.conf = conf

        self.converted_prefixes = False

        self.conn = MySQL()
        self.server = None
        self.monitoring = None
        self.graphite = None
        self.slowlog = None
        self.prev = {}

        self.status = defaultdict(lambda: deque([], conf.live.maxPoints))
        self.others = defaultdict(lambda: deque([], conf.live.maxPoints))

    def convert_prefixes(self, status, type_):
        """
        Converts the config from prefixes to real fields
        on the first launch
        """
        prefixes = []
        for line in self.conf.metrics[type_]:
            if line.endswith('*'):
                prefixes.append(line[:-1])
        for prefix in prefixes:
            for name in status:
                if name.startswith(prefix):
                    self.conf.metrics[type_].append(name)
        self.conf.metrics[type_] = \
            [x for x in self.conf.metrics[type_] if not x.endswith('*')]

    def fetch_data(self):
        """
        Fetch data from the connection, convert it and report
        to the caller
        """
        try:
            status = self.conn.query("SHOW GLOBAL STATUS", as_dict=True)
            slave_status = self.conn.query("SHOW SLAVE STATUS")

            if not self.converted_prefixes:
                self.convert_prefixes(status, 'direct')
                self.convert_prefixes(status, 'diff')
                self.converted_prefixes = True

            data = {
                'timestamp': time.time(),
            }

            for name in self.conf.metrics.direct:
                data[name] = int(status[name])

            for name in self.conf.metrics.diff:
                if name in self.prev:
                    data[name] = int(status[name]) - self.prev[name]
                else:
                    data[name] = 0
                self.prev[name] = int(status[name])

            if slave_status:
                for name in self.conf.metrics.replication:
                    val = slave_status[0][name]
                    # get Seconds behind master var can be long or not digital
                    try:
                        data[name] = int(val) if val is not None else None
                    except ValueError:
                        data[name] = None

            for name in data:
                self.status[name].append(data[name])
            self.server.send_updates(data)

            # and variables
            self.others['variables'].append(self.conn.query("SHOW GLOBAL VARIABLES", as_dict=True))
            self.others['processlist'].append(self.conn.query("SHOW FULL PROCESSLIST"))
            self.others['innodb_trx'].append(
                self.conn.query("SELECT * FROM INFORMATION_SCHEMA.INNODB_TRX"))
            self.others['innodb_status'].append(self.conn.query("SHOW ENGINE INNODB STATUS")[0])
            self.others['slave'].append(slave_status)

            self.slowlog.set_vars(self.others['variables'][-1])
            self.monitoring.check(self.status,
                                  self.others,
                                  self.slowlog.seen_crit_queries,
                                  self.slowlog.seen_warn_queries)
            self.graphite.send(data)

        except MySQLError as err:
            self.log.warning('Error getting data: %s', err)
            self.log.info('Disconnected from MySQL')
            self.conn = None
            self.monitoring.set_explicit_dead()

    @staticmethod
    def tail_file(path, lines):
        """read file like a tail util"""
        with open(path) as logfile:
            size = os.stat(path).st_size
            processed_lines = 0
            for cur_byte in xrange(size):
                logfile.seek(size - cur_byte - 1)
                value = logfile.read(1)
                code = ord(value)
                if code == 10:
                    processed_lines += 1
                if processed_lines > lines:
                    break
            while True:
                line = logfile.readline()
                if not line:
                    break
                yield line

    def check_error_log(self, path):
        """check mysql error log"""
        try:
            for line in self.tail_file(path, 100):
                if re.search(r'The table \S+ is full', line):
                    self.others['error_log'].append(line)
                if re.search(r'Too many connections', line):
                    self.others['error_log'].append(line)
        except IOError:
            return False
        return True

    def run(self, server, monitoring, graphite, slowlog):
        """ The main routine """
        self.log.info('Starting the poller')
        self.server = server
        self.monitoring = monitoring
        self.graphite = graphite
        self.slowlog = slowlog

        try:
            while True:
                if not self.conn.is_connected():
                    self.conn.connect()

                if self.conn.is_connected():
                    self.fetch_data()

                error_log_path = "/var/log/mysql/mysql-error.log"
                if os.path.isfile(error_log_path):
                    self.check_error_log(error_log_path)

                eventlet.sleep(self.conf.live.interval)
        except greenlet.GreenletExit:  # pylint: disable=c-extension-no-member
            pass

        self.log.info('Stopped the poller')
