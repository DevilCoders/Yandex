#!/usr/bin/env python3
""" UWSGI job """
import logging
import logging.config
from raven import Client
from icecream_agent.models.hw import Dom0
from icecream_agent.models.icecream_http import IceClient, IceClientError
from icecream_agent.constants import DEFAULT_CONFIG, CONFIG_PATH, LOG_CONFIG, TMP_DIR
from icecream_agent.libs.common_helpers import random_sleep, load_config


logging.config.dictConfig(LOG_CONFIG)
logging.root = logging.getLogger('mule')

CONFIG = load_config(DEFAULT_CONFIG, CONFIG_PATH)
SENTRY = None
if CONFIG.sentry:
    SENTRY = Client(CONFIG.sentry)
CONFIG_PARAM = [CONFIG.pool]

logging.root.setLevel(CONFIG.log.level)


def icecream_register():
    """ Send heartbeat to icecream api with info about host """
    logging.info('Registering in icecream api...')
    machine = Dom0(*CONFIG_PARAM)
    client = IceClient(machine, CONFIG.api.host, CONFIG.api.token)
    try:
        status = client.register().status_code
        logging.info('Registered')
    except IceClientError as err:
        logging.error(err.message)
        status = err.code
        if SENTRY is not None:
            SENTRY.captureException()

    with open(TMP_DIR+'/icecream_register.tmp', 'w') as status_file:
        status_file.write(str(status))


if __name__ == '__main__':
    logging.info('Starting mule...')
    while True:
        logging.info('Starting round...')
        random_sleep(30, 60)
        icecream_register()
        logging.info('Round finished')
