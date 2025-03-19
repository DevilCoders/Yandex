"""CLOUD-24598: simple script to bootstrap database"""

import sys

from bootstrap.db.admin.app import get_parser, validate_options, main


if __name__ == "__main__":
    options = get_parser().parse_args()

    validate_options(options)

    status = main(options)

    sys.exit(status)
