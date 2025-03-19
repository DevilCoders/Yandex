"""
Bootstrap script
"""
import argparse
import logging
import sys

from . import deploy
from .bootstrap import Bootstrap
from .utils.config import get_config


def get_args():
    """
    Parse arguments
    """
    parser = argparse.ArgumentParser()
    parser.add_argument('-s', '--stage', type=str, default='salt')
    parser.add_argument('-c', '--config', type=str, default='bootstrap.conf')
    return parser.parse_args()


def init_logging():
    """
    Logging initialization
    """
    root = logging.getLogger()
    root.setLevel(logging.INFO)
    handler = logging.StreamHandler(sys.stdout)
    formatter = logging.Formatter("%(asctime)s %(levelname)s %(message)s")
    handler.setFormatter(formatter)
    root.addHandler(handler)


def main():
    """
    Main function, entry point for bootstrap script
    """
    args = get_args()
    init_logging()
    config = get_config(args.config)
    bootstrap = Bootstrap(*config)
    # bootstrap.drop()
    bootstrap.deploy()


if __name__ == '__main__':
    main()
