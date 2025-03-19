"""CLOUD-24765: Api for access to bootstrap storage"""

import sys

from bootstrap.api.app import get_parser, main


if __name__ == '__main__':
    options = get_parser().parse_args()

    status = main(options)

    sys.exit(status)
