#!/usr/bin/env python3

import argparse

import yaqutils.args_helpers as uargs
import yaqutils.file_helpers as ufile
import yaqutils.json_helpers as ujson
import yaqutils.misc_helpers as umisc


def parse_args():
    parser = argparse.ArgumentParser(description="Filter users with specific testid")
    uargs.add_input(parser, help_message="Source sjson file (YT json table dump)")
    uargs.add_verbosity(parser)

    parser.add_argument(
        "-t",
        "--testid",
        required=True,
        help="testid to search",
    )

    return parser.parse_args()


def main():
    cli_args = parse_args()
    umisc.configure_logger(cli_args.quiet, cli_args.verbose)

    with ufile.fopen_read(cli_args.input_file) as table_fd:
        for line in table_fd:
            data = ujson.load_from_str(line)
            value = data["value"]
            testids = value.split("\t")
            if cli_args.testid in testids:
                print(line.rstrip())


if __name__ == "__main__":
    main()
