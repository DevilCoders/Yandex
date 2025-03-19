""""
Juggler downtime module
"""
import logging
import json
import eventlet.green
import requests


STATUS_NAMES = ["OK", "WARN", "CRIT"]


class Juggler(object):
    """
    Main class to set downtime
    """
    def __init__(self, conf):
        self.log = logging.getLogger(self.__class__.__name__)
        self.hostname = eventlet.green.socket.gethostname() # pylint: disable=no-member
        self.api = conf.juggler.api
        self.timeout = conf.juggler.timeout
        self.max_ticker = 60 // conf.live.interval
        self.ticker = 0

    def _send(self, data):
        """ Really send data """
        try:
            for line in data:
                self.log.debug(
                    'Sending to juggler: %(service)20s => %(status)4s  %(description)s',
                    line)

            resp = requests.post(self.api, timeout=self.timeout, data=json.dumps(data))
            if resp.status_code != 200:
                self.log.warning('Juggler status code is %s', resp.status_code)
        except requests.exceptions.RequestException as err:
            self.log.warning('Error sending to juggler: %s', err)

    def send_diffs(self, diffs):
        """ Send diffs to juggler """
        params = []
        for diff in diffs:
            params.append({
                'host': self.hostname,
                'service': 'mysql_' + diff['name'],
                'instance': '',
                'status': STATUS_NAMES[diff['status']],
                'description': diff['description'],
            })
        self._send(params)

    def send_statuses(self, statuses):
        """ Send statuses to juggler """
        params = []
        for name in statuses:
            params.append({
                'host': self.hostname,
                'service': 'mysql_' + name,
                'instance': '',
                'status': STATUS_NAMES[statuses[name][0]],
                'description': statuses[name][1],
            })
        self._send(params)

    def send(self, diffs, statuses):
        """Send data to juggler

        Sends diffs immediately if available, else statuses once every
        N minutes

        Arguments:
            diffs {list} -- The list of diffs
            statuses {dict} -- Current status info
        """
        self.ticker += 1
        if self.ticker > self.max_ticker:
            self.ticker = 0
            self.send_statuses(statuses)
            return

        if diffs:
            self.send_diffs(diffs)
