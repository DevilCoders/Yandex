#!/usr/bin/env python2.7
# -*- coding: utf-8 -*-

import argparse
import importlib
import yaqutils.six_helpers as usix


SUBCOMMANDS = {
    "squeeze_yt": "squeeze-yt",
    "squeeze_yt_single": "squeeze-yt-single",
}


def parse_args():
    parser = argparse.ArgumentParser(description="Shell utility to work with mstand.")
    subparsers = parser.add_subparsers()

    for module, command in usix.iteritems(SUBCOMMANDS):
        runner = importlib.import_module("cli_tools.{}".format(module))
        runner_parser = subparsers.add_parser(command,
                                              help=runner.DESCRIPTION,
                                              description=runner.DESCRIPTION)
        runner.add_arguments(runner_parser)
        runner_parser.set_defaults(func=runner.main)

    return parser.parse_args()


def main():
    cli_args = parse_args()
    cli_args.func(cli_args)


if __name__ == "__main__":
    main()
