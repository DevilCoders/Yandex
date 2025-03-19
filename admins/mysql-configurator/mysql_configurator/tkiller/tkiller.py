"""Transaction killer"""

import re
import logging
import eventlet
import greenlet
from ..mysql import MySQL, MySQLError


class TKiller(object):
    """
    input conf must be yaml
    """
    def __init__(self, conf):
        self.log = logging.getLogger(self.__class__.__name__)
        self.conn = MySQL()
        self.conf = set_defaults_in_config(conf)
        self.log.setLevel(self.conf.tkiller.log.level)
        self.log.debug("Tkiller config %s", conf.tkiller.toDict())


    def get_data(self):
        """only get data about active transactions from mysql and returns it"""
        if not self.conn.is_connected():
            self.conn.connect()

        if self.conn.is_connected():

            query = """
            SELECT id, user, trx_id, command, info,
                UNIX_TIMESTAMP(NOW()) - UNIX_TIMESTAMP(trx_started) AS time
            FROM INFORMATION_SCHEMA.PROCESSLIST p
            JOIN INFORMATION_SCHEMA.INNODB_TRX t
            ON t.trx_mysql_thread_id = p.id;"""

            return self.conn.query(query)

        return None

    def killtrx(self, thread_id):
        """kill transaction"""
        if not self.conn.is_connected():
            self.conn.connect()

        try:
            self.conn.query('KILL %s;'%thread_id)
        except MySQLError as err:
            self.log.error('Failed to kill trx with thread id %s', thread_id)
            self.log.debug('Got MySQL error: %s', err)


    def process(self):
        """kills transactions which exec time is too long"""
        data = self.get_data()

        if not data:
            return None

        for trn in data:
            self.log.debug("Process transaction: %s", trn)

            info = trn.get('info', None)
            if trn['time'] > self.conf.tkiller.processor.threshold:

                if self.conf.tkiller.processor.ignore_user.match(trn['user']):
                    self.log.debug('Skip transaction %s from user %s', trn['id'], trn['user'])
                    continue
                if info and self.conf.tkiller.processor.ignore_query.match(info):
                    self.log.debug('Skip transaction %s with query: %s', trn['id'], info)
                    continue

                self.log.info(
                    'KILL transaction with Thread ID %s, exec time %s',
                    trn['id'], trn['time']
                )
                self.log.debug('Transaction %s query: %s', trn['id'], info)
                self.killtrx(trn['id'])
        return None

    def run(self):
        """process data while process will not be ended"""
        self.log.info('Start transaction killer...')

        self.log.info(
            'Transaction killer %s in config transaction-killer/main.yaml',
            'enabled' if self.conf.tkiller.processor.enable else 'disabled'
        )
        try:
            while True:
                if self.conf.tkiller.processor.enable:
                    self.process()
                eventlet.sleep(self.conf.tkiller.processor.interval)
        except greenlet.GreenletExit: # pylint: disable=no-member, c-extension-no-member
            self.log.info('Stop transaction killer.')


def set_defaults_in_config(config):
    """Set defaults in transaction killer's config"""
    if not config.tkiller.processor:
        config.processor.enable = True
    if not config.processor.threshold:
        config.processor.threshold = 99999
    if not config.processor.interval:
        config.processor.interval = 60

    if not config.processor.ignore_user:
        config.processor.ignore_user = re.compile('^root|repl|system user$')
    else:
        config.processor.ignore_user = re.compile(config.processor.ignore_user)
    if not config.processor.ignore_query:
        config.processor.ignore_query = re.compile('inexistent_query')
    else:
        config.processor.ignore_query = re.compile(config.processor.ignore_query)

    lvl = {
        'debug': logging.DEBUG,
        'info': logging.INFO,
        'warning': logging.WARNING,
        'error': logging.ERROR,
    }
    if not config.tkiller.log.level:
        config.tkiller.log.level = 'info'
    config.tkiller.log.level = lvl[config.tkiller.log.level]

    return config
