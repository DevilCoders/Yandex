"""CLOUD-28676: Aux helper utility"""

import sys

from bootstrap.api.helper.app import get_parser, main


if __name__ == '__main__':
    options = get_parser().parse_args()

    status = main(options)

    sys.exit(status)
