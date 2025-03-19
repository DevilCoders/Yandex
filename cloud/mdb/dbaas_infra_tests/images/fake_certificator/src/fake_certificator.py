#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Certificator mock. Warning: most responses are incomplete
"""
from datetime import datetime, timedelta

from flask import jsonify, make_response
from marshmallow.validate import Equal, OneOf
from webargs.fields import Str
from webargs.flaskparser import use_args

from ca import PKI
from server_mock import ServerMock, check_auth

APP = ServerMock('fake_certificator')
PKI_OBJ = PKI('/config/pki')
REVOKED = set()
DATE_FORMAT = '%Y-%m-%dT%H:%M:%SZ'


@APP.route('/api/certificate/', methods=['GET'])
@use_args({
    'Host': Str(location='headers', required=True),
    'host': Str(location='query', required=True),
})
@check_auth
def get_certificate(args):
    """
    Return cert info if issued
    """
    localhost = args['Host']
    hostname = args['host']
    # return 0 if not issued yet.
    if hostname not in PKI_OBJ.certs:
        return jsonify({'count': 0})

    end_date = (datetime.now() + timedelta(days=3650)).strftime(format=DATE_FORMAT)
    issued = (datetime.now() + timedelta(minutes=-1)).strftime(format=DATE_FORMAT)
    data = {
        'count':
            1,
        'results': [{
            'status': 'issued' if hostname not in REVOKED else 'revoked',
            'revoked': None if hostname not in REVOKED else issued,
            'url': 'http://{me}/api/certificate/{host}'.format(
                me=localhost,
                host=hostname,
            ),
            'priv_key_deleted_at': None,
            'download2': 'http://{me}/download/{host}'.format(
                me=localhost,
                host=hostname,
            ),
            'end_date': end_date,
            'issued': issued,
            'serial_number': hex(PKI_OBJ.certs[hostname].get_serial_number())[2:],
        }],
    }
    return jsonify(data)


@APP.route('/api/certificate/<hostname>', methods=['DELETE'])
@check_auth
def revoke_certificate(hostname=None):
    """
    Revoke issued cert
    """
    if hostname not in PKI_OBJ.certs:
        return make_response(
            jsonify({
                'error': 'Certificate is not issued yet',
            }),
            400,
        )
    elif hostname in REVOKED:
        return make_response(
            jsonify({
                'detail': 'Can not revoke certificate with status revoked',
            }),
            500,
        )

    PKI_OBJ.revoke(hostname)
    REVOKED.add(hostname)

    data = {'revoked': datetime.now().strftime(format=DATE_FORMAT)}

    return jsonify(data)


@APP.route('/api/certificate/', methods=['POST'])
@use_args({
    'Host': Str(location='headers', required=True),
    'type': Str(
        validate=OneOf(['mdb']),
        location='json',
        required=True,
    ),
    'ca_name': Str(
        validate=Equal('InternalCA'),
        location='json',
        required=True,
    ),
    'hosts': Str(location='json', required=True),
})
@check_auth
def issue_certificate(args):
    """
    Trigger certificate generation
    """
    hosts = args['hosts'].split(',')
    hostname = hosts[0]
    localhost = args['Host']
    # get_cert(cname) triggers build
    PKI_OBJ.get_cert(hostname, hosts)
    end_date = datetime.now() + timedelta(days=3650)
    data = {
        'download2': 'http://{me}/download/{host}'.format(
            me=localhost,
            host=hostname,
        ),
        'end_date': end_date.strftime(format=DATE_FORMAT),
        'serial_number': hex(PKI_OBJ.certs[hostname].get_serial_number())[2:],
    }
    return make_response(jsonify(data), 201)


@APP.route('/crl', methods=['GET'])
def get_crl():
    """
    CRL distribution point mock (return always differs)
    """
    crl = PKI_OBJ.get_crl()
    return make_response(crl.encode('utf-8'))


@APP.route('/download/<hostname>.pem', methods=['GET'])
@APP.route('/download/<hostname>', methods=['GET'])
@check_auth
def download(hostname=None):
    """
    Download key and certificate for given CN
    """
    if hostname not in PKI_OBJ.certs:
        return make_response(
            jsonify({
                'error': 'Certificate is not issued yet',
            }),
            400,
        )
    response_key = PKI_OBJ.get_key(hostname).encode('utf8')
    response_cert = PKI_OBJ.get_cert(hostname).encode('utf8')
    return make_response(response_key + response_cert)


if __name__ == '__main__':
    APP.run(host='0.0.0.0', debug=True)
