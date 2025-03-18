#!/skynet/python/bin/python

import logging
from argparse import ArgumentParser


def main():
    logging.warning('This tool is removed.')


def parse_args():
    parser = ArgumentParser()
    parser.add_argument('--nomongo', action='store_true')
    parser.add_argument('--full', action='store_true')
    return parser.parse_args()


if __name__ == '__main__':
    arguments = parse_args()
    main()
