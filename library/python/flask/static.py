# -*- coding: utf-8 -*-

from __future__ import print_function, absolute_import, division

import hashlib
import logging
import mimetypes

import flask

import library.python.resource as lpr

logger = logging.getLogger(__name__)


def iter_static_data_from_resources(resource_prefix):
    for path, data in lpr.iteritems(prefix=resource_prefix, strip_prefix=True):
        yield path, {
            'data': data,
            'mimetype': mimetypes.guess_type(path)[0],
            'etag': hashlib.sha1(data).hexdigest(),
        }


def get_static_data_from_resources(prefix):
    return dict(iter_static_data_from_resources(prefix))


def send_static_data(static_data, path):
    content = static_data.get(path)
    if content is None:
        flask.abort(404)

    if flask.request.if_none_match and content['etag'] in flask.request.if_none_match:
        return flask.Response(status=304)  # not modified

    resp = flask.make_response(content['data'])
    resp.mimetype = content['mimetype']
    resp.set_etag(content['etag'])
    return resp.make_conditional(flask.request)


def serve_static_data_from_resources(app, prefix, resource_prefix):
    static_data = get_static_data_from_resources(resource_prefix)

    if static_data:
        logger.debug('Static data for %s found at %s: %d rules', prefix, resource_prefix, len(static_data))
    else:
        logger.warning('No static data for %s found at %s', prefix, resource_prefix)
    for path, desc in static_data.items():
        logger.debug('Static data in %s - %s %s %s (%s bytes)', prefix, desc['etag'], path, desc['mimetype'], len(desc['data']))

    if not prefix.endswith('/'):
        prefix += '/'
    rule = prefix + '<path:filename>'

    @app.route(rule)
    def static(filename):
        # NB: this handler mimics the default one (see `Flask.send_static_file`)
        # The handler must be named 'static' and its argument - 'filename'.
        return send_static_data(static_data, filename)
