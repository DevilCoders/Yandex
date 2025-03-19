"""
Monitoring server class
"""
import json
import logging
import os
import greenlet
from flask import Flask, Response  # pylint: disable=import-error
from flask_socketio import SocketIO  # pylint: disable=import-error


class Server(object):
    """
    The HTTP server that handles all the connections
    """
    def __init__(self, conf):
        self.log = logging.getLogger(self.__class__.__name__)
        self.conf = conf

        self.monitoring = None
        self.poller = None

        static_dir = '/usr/share/mysql-configurator-4/markup'
        if not os.path.isdir(static_dir):
            static_dir = 'static'

        self.app = Flask(__name__, static_url_path='', static_folder=static_dir)
        self.app.add_url_rule('/mon/<what>', 'mon', self.mon)
        self.app.add_url_rule('/data', 'data', self.data)
        self.app.add_url_rule('/', 'index', self.index)
        self.socketio = SocketIO(self.app)

    def send_updates(self, data):
        """ Sends data updates to the connected peers """
        self.socketio.emit('data', data)

    def index(self):
        """ Render index page """
        self.log.info('GET /')
        return self.app.send_static_file('index.html')

    def data(self):
        """ Render data json """
        self.log.info('GET /data')
        status = dict([(field, tuple(value)) for field, value in self.poller.status.items()])
        resp = Response(json.dumps(status))
        resp.headers['Access-Control-Allow-Origin'] = '*'
        resp.mimetype = "application/json"
        return resp

    def mon(self, what):
        """ Return the monitoring value """
        self.log.info('GET /mon/%s', what)
        state = self.monitoring.get(what)
        http_status = 200 if state[0] < 2 else 503
        resp = ('{};{}'.format(*state), http_status)
        return resp

    def run(self, monitoring, poller):
        """ The main routine """
        self.log.info('Starting listening for HTTP')
        self.monitoring = monitoring
        self.poller = poller
        try:
            self.socketio.run(self.app, host=self.conf['live']['host'], port=4417)
        except greenlet.GreenletExit:  # pylint:disable=c-extension-no-member
            pass
        self.log.info('Stopped listening for HTTP')
