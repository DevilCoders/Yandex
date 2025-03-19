"""
Raven init helper
"""
from library.python import svn_version  # pylint: disable=import-error
from raven import Client
from raven.transport.http import HTTPTransport


def init_raven(app):
    """
    Attach raven client with config to app
    """
    if app.config['SENTRY']['dsn']:
        client = Client(app.config['SENTRY']['dsn'], transport=HTTPTransport, release=f'2.{svn_version.svn_revision()}')
        client.environment = app.config['SENTRY']['environment']
        app.raven_client = client
    else:
        app.raven_client = None
