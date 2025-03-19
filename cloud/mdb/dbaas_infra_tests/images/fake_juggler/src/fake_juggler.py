#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import time

from flask import jsonify
from marshmallow.validate import Equal
from webargs.fields import Int, List, Nested, Str
from webargs.flaskparser import use_args

from server_mock import ServerMock, check_auth

APP = ServerMock('fake_juggler')
FAILED = {}


@APP.reset_handler
def drop_state():
    """
    Clear all failed events
    """
    FAILED.clear()


@APP.route('/internal/fail', methods=['POST'])
@use_args({
    'host_name': Str(location='query', required=True),
    'service_name': Str(location='query', required=True),
})
def set_failed(args):
    """
    Mark service on host as CRIT
    """
    if args['host_name'] not in FAILED:
        FAILED[args['host_name']] = set()
    FAILED[args['host_name']].add(args['service_name'])
    return b''


@APP.route('/v2/downtimes/set_downtimes', methods=['POST'])
@use_args({
    'filters': List(Nested({
        'host': Str(required=True),
    }), location='json', required=True),
    'description': Str(location='json', required=True),
    'end_time': Int(location='json', required=True),
    'start_time': Int(location='json', required=True),
})
@check_auth
def set_downtime(_args):
    """
    Set downtime.
    """
    return b''


@APP.route('/v2/downtimes/get_downtimes', methods=['POST'])
@use_args({
    'filters': List(Nested({
        'host': Str(required=True),
    }), location='json', required=True),
})
@check_auth
def get_downtimes(_args):
    """
    Get downtimes for host.
    """
    return jsonify({'items': []})


@APP.route('/v2/events/get_raw_events', methods=['POST'])
@use_args({
    'filters':
        List(Nested({
            'host': Str(required=True),
            'service': Str(required=True),
        }), location='json', required=True),
})
@check_auth
def get_raw_events(args):
    """
    Return raw events (always OK unless in failed dict)
    """
    hostname = args['filters'][0]['host']
    service_name = args['filters'][0]['service']
    if service_name in FAILED.get(hostname, set()):
        status = 'CRIT'
    else:
        status = 'OK'

    return jsonify({
        'items': [{
            'description': 'mocked',
            'host': hostname,
            'instance': '',
            'received_time': time.time(),
            'service': service_name,
            'status': status,
            'tags': [],
        }],
    })


if __name__ == '__main__':
    APP.run(host='0.0.0.0', debug=True)
