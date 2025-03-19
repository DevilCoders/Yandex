"""
graphite sender module
"""
# pylint: disable=no-member
import ConfigParser
import logging
import eventlet.green.socket as socket


class Graphite(object):
    """"
    Main module to send metrics to graphite
    """
    def __init__(self):
        config = ConfigParser.RawConfigParser()
        config.read(['/etc/graphite-checks.conf'])

        self.log = logging.getLogger(self.__class__.__name__)
        self.prefix = ''
        self.next_ts = 0

        if config.has_option('main', 'metrics_prefix'):
            self.prefix = config.get('main', 'metrics_prefix') + '.' + \
                          socket.gethostname().replace('.', '_') + '.mysql.'

    def send(self, metrics):
        """ Formats and sends metrics to graphite """
        if not self.prefix:
            return

        timestamp = int(metrics['timestamp'])
        if timestamp < self.next_ts:
            return

        data = ''
        for name, value in metrics.items():
            if name != 'timestamp':
                data += '{prefix}{name} {value} {ts}\n'.format(
                    prefix=self.prefix,
                    name=name,
                    value=value,
                    ts=timestamp,
                    )

        if self.raw_send(data):
            self.next_ts = timestamp + 60

    def raw_send(self, data):
        """ Sends raw metrics to graphite """
        try:
            sock = socket.socket(socket.AF_INET6, socket.SOCK_STREAM)
            sock.connect(('::1', 42000))
            sock.send(data)
            sock.close()
            return True
        except socket.error as err:
            self.log.debug('Send error: %s', err)
            return False
