#!/usr/bin/env python

from flask import Flask, Response
import os.path
import supervisor.xmlrpc
from xmlrpc.client import ServerProxy


CLOSE_FILE = '/tmp/pgproxy_close'

app = Flask(__name__)


def get_supervisor_api(socket_path='/var/run/supervisor.sock'):
    return ServerProxy('http://127.0.0.1', transport=supervisor.xmlrpc.SupervisorTransport(
        None, None, serverurl='unix://' + socket_path)).supervisor


def is_ok():
    api = get_supervisor_api()
    processes = set([x['group'] for x in api.getAllProcessInfo() if x['statename'] == 'RUNNING'])
    return ('pgbouncer_external' in processes) and ('pgbouncer_internal' in processes)


@app.route('/ping')
def index():
    if os.path.exists(CLOSE_FILE):
        return Response("pgbouncer is closed", status=500)
    if is_ok():
        return Response("OK", status=200)
    else:
        return Response("pgbouncer not available", status=500)


if __name__ == '__main__':
    app.run(host='::', port=8080)
