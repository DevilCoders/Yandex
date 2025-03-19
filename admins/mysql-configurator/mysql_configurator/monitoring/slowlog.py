"""
Slowlog detection module
"""
import logging
import os
import md5
import eventlet
import greenlet
from eventlet.green import subprocess  # pylint: disable=ungrouped-imports
import kazoo.exceptions


class SlowLog(object):  # pylint: disable=too-many-instance-attributes
    """
    Main slowlog detection class
    """
    def __init__(self, conf, zk):
        self.log = logging.getLogger(self.__class__.__name__)
        self.conf = conf

        self.filename = None
        self.stat = None
        self.zk = zk  # pylint: disable=C0103
        self.seen_crit_queries = set()
        self.seen_warn_queries = set()

        self.zk_url = conf.monitoring.slowlog.ignored_key.format(group=conf.group_name)
        self.log.debug('Will be using %s key in zookeeper for the slow log', self.zk_url)

    def set_vars(self, variables):
        """Set the vars of the monitored file

        Run by the poller to start the monitoring. Autoturns on/off

        Arguments:
            variables {dict} -- The latest vars dict
        """
        # ignore file if turned off
        if variables['slow_query_log'] == 'OFF':
            variables['slow_query_log_file'] = None

        if self.filename != variables['slow_query_log_file']:
            self.filename = variables['slow_query_log_file']
            self.log.info('Monitoring file: %s', self.filename)
            self.stat = os.stat(self.filename)

    def normalize_query(self, query):
        """Normalize a query in a percona toolkit compatible way

        Arguments:
            query {str} -- The query to normalize

        Returns:
            str -- The normalized query
        """

        pos = query.find(';')
        query = query[pos + 1:]
        # pylint: disable=no-member
        try:
            proc = subprocess.Popen(['pt-fingerprint'],
                                    stdin=subprocess.PIPE,
                                    stdout=subprocess.PIPE,
                                    stderr=subprocess.STDOUT)
            proc.stdin.write(query)
            result = proc.communicate()
            return result[0].strip()
        except subprocess.CalledProcessError as exc:
            self.log.error('Could not run pt-fingerprint: %s', exc)
        return query.strip()

    @staticmethod
    def query_id(query):
        """Return a query ID in percona toolkit compatible way

        Arguments:
            query {str} -- The string to make a query id of
        """
        return "0x" + md5.new(query).hexdigest()[16:].upper()

    def report_query(self, query):
        """Reports the query to juggler if it is not in banlist"""
        query = self.normalize_query(query)
        digest = self.query_id(query)
        self.log.info('Reporting a slow query: [%s] %s', digest, query)
        try:
            self.zk.get(self.zk_url + '/' + digest)
            self.seen_crit_queries.discard(digest)
            self.seen_warn_queries.add(digest)
            return
        except kazoo.exceptions.NoNodeError:
            pass
        except kazoo.exceptions.ZookeeperError as exc:
            self.log.error('Zookeeper error: %s', exc)
        self.seen_crit_queries.add(digest)

    def check_for_queries(self):
        """Run one loop of a slow query check"""
        stat = os.stat(self.filename)
        if stat.st_mtime > self.stat.st_mtime:
            with open(self.filename) as inf:
                inf.seek(self.stat.st_size)
                lines = inf.readlines()

            query = ''
            for line in lines:
                if line.startswith('#'):
                    if query:
                        self.report_query(query)
                        query = ''
                    continue
                query += line
            if query:
                self.report_query(query)
            self.stat = stat

    def run(self):
        """
        Class main
        :return: None
        """
        try:
            while True:
                if self.filename:
                    self.check_for_queries()

                eventlet.sleep(self.conf.monitoring.slowlog.interval)
        except greenlet.GreenletExit:  # pylint: disable=c-extension-no-member
            pass
