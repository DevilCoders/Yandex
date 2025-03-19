""" iptables_except_my_group runnable """
import sys
import argparse
import logging
from iptables_except_my_group import utils


logger = logging.getLogger(__name__)


def setup_argparser():
    """ Configure args for run from command line """
    parser = argparse.ArgumentParser()
    parser.add_argument("action", choices=["close", "open", "check", "status"])
    parser.add_argument("port", type=int, nargs="?", default=1334)
    parser.add_argument("-b", "--remove_backup_suffix",
                        help="Remove `backup` suffix from my_group_name",
                        action="store_true", default=True)
    parser.add_argument("-f", "--force",
                        help="Force to close even dangerous ports",
                        action="store_true")
    parser.add_argument("-d", "--debug", help="Enable debug logging",
                        action="store_true", default=False)
    args = parser.parse_args()
    return args


def main():
    """ Main function """
    args = vars(setup_argparser())
    utils.setup_logger(default_debug=args.pop('debug'))

    try:
        utils.operate_iptables(**args)
    except RuntimeError as error:
        logger.error(error)
        sys.exit(1)
    except Exception as error:
        logger.error('Unexpected error: %s', error, exc_info=error)
        sys.exit(1)
