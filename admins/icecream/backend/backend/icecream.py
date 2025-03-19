#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""icecream backend main module"""
import logging.config
import connexion
from lib.auth import auth
from lib import utils, wrap_error_handler


def main():
    """main"""

    config = utils.load_config()
    logging.config.dictConfig(config.logging.toDict())

    connexion.FlaskApp.common_error_handler = wrap_error_handler(
        connexion.FlaskApp.common_error_handler
    )

    api = connexion.FlaskApp("icecream", specification_dir='spec', debug=True)
    api.app.before_request(auth)
    api.add_api('swagger.yaml', resolver=connexion.resolver.RestyResolver('v1'))

    return api.app

application = main()  # pylint: disable=invalid-name
