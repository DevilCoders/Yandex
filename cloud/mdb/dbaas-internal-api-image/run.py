#!/usr/bin/env python3
"""
Debug run script.

Warning: never start this application with run.py on production.
Debug could expose critical information to users.
"""

import os
import signal
import sys

from dbaas_internal_api import create_app

APP = create_app()


def exit_message(*_):
    """
    Display exit message
    Coverage in multiprocess mode requires SIGTERM handler.
    Use this simple function for it.
    """
    print('Stopping internal-api')
    sys.exit(0)


def main():
    """
    main entry point
    """
    debug = bool(os.getenv('INTERNAL_API_DEBUG'))
    if not debug:
        signal.signal(signal.SIGTERM, exit_message)

    APP.run(host=os.getenv('INTERNAL_API_HOST'), port=int(os.getenv('INTERNAL_API_PORT', '5000')), debug=debug)


if __name__ == '__main__':
    main()
