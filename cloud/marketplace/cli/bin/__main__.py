#!/usr/bin/env python3

import sys

from cloud.marketplace.cli.yc_marketplace_cli.core import ManagementCommand


def main():
    ManagementCommand.execute(sys.argv)


if __name__ == "__main__":
    main()
