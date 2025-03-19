#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import time

from flask import jsonify
from marshmallow.validate import Equal
from webargs.fields import List, Str
from webargs.flaskparser import use_args

from server_mock import ServerMock, check_auth

APP = ServerMock('fake_health')
FAILED = {}


@APP.route('/v1/listhostshealth', methods=['POST'])
@use_args({
    'hosts': List(Str(required=True), location='json', required=True),
})
def list_hosts_health(args):
    """
    Get health for hosts.
    """

    return jsonify({
        "hosts": [{
            "cid":
                "",
            "fqdn":
                fqdn,
            "services": [{
                "name": "mysql",
                "status": "Alive",
                "role": 'Master' if fqdn.startswith('man') else 'Replica',
            }],
            "status":
                "Alive",
        } for fqdn in args['hosts']],
    })


if __name__ == '__main__':
    APP.run(host='0.0.0.0', debug=True)
