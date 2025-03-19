#!/usr/bin/env python

from app import app

import argparse
import logging


consoleHandler = logging.StreamHandler()
consoleHandler.setFormatter(logging.Formatter("%(asctime)s [%(threadName)s] [%(levelname)s]  %(message)-100s"))

rootLogger = logging.getLogger()
rootLogger.setLevel(logging.DEBUG)
rootLogger.addHandler(consoleHandler)


def main():
    parser = argparse.ArgumentParser()

    parser.add_argument(
        '-p',
        '--port',
        type=int,
        help='Output port for this http server, default: %(default)d',
        required=False, default=7890
    )

    args = parser.parse_args()

    app.run(host="::", port=args.port, threaded=False, processes=5)


if __name__ == '__main__':
    main()
