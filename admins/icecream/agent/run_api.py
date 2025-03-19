#!/usr/bin/env python3
""" Here icecream-agent api start """
import socket
import logging
import logging.config
import connexion
from raven.contrib.flask import Sentry
from icecream_agent.libs.common_helpers import load_config
from icecream_agent.constants import ENABLED_APIS, LOG_CONFIG, DEFAULT_CONFIG, CONFIG_PATH


logging.config.dictConfig(LOG_CONFIG)
logging.root = logging.getLogger('api')

CONFIG = load_config(DEFAULT_CONFIG, CONFIG_PATH)

logging.root.setLevel(CONFIG.log.level)


def launch_api():
    """ Fire with specs """
    swagger_global_args = {
        'host': socket.gethostname()
    }
    logging.info('Launching swagger apis with args: %s', swagger_global_args)
    swag = connexion.App(__name__, arguments=swagger_global_args)

    if CONFIG.sentry:
        sentry = Sentry(dsn=CONFIG.sentry)
        sentry.init_app(swag.app)

    for spec_path in ENABLED_APIS:
        logging.info('Adding api spec: %s', spec_path)
        swag.add_api(spec_path)
    return swag


APP = launch_api()


if __name__ == '__main__':
    APP.run(host='::', port=1991, debug=True)
