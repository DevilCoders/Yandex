#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from marshmallow.validate import OneOf
from webargs.fields import Str
from webargs.flaskparser import use_args

from formatters import json_formatter, xml_formatter
from methods import oauth, sessionid
from server_mock import ServerMock
from flask import jsonify


APP = ServerMock('fake_passport')

FORMATTERS = {
    'xml': xml_formatter,
    'json': json_formatter,
}

METHODS = {
    'oauth': oauth,
    'sessionid': sessionid,
}


@APP.route('/blackbox')
@use_args({
    'method':
        Str(validate=OneOf(list(METHODS)), location='query', required=True),
    'oauth_token':
        Str(location='query'),
    'sessionid':
        Str(location='query'),
    'host':
        Str(location='query'),
    'userip':
        Str(location='query', required=True),
    'format':
        Str(validate=OneOf(list(FORMATTERS)), location='query', missing='xml'),
})
def blackbox(args):
    """
    Entry point for Passport / Blackbox requests.
    """
    method = args['method']
    fmt = args['format']

    data = METHODS[method](APP.config['PASSPORT'], args)

    return FORMATTERS[fmt](**data)


@APP.route('/ping')
def ping():
    """
    Entry point for ping requests.
    """
    return jsonify({})


if __name__ == '__main__':
    APP.run(host='0.0.0.0', debug=True)
