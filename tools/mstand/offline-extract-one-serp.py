#!/usr/bin/env python3

import argparse
import logging

import serp.serpset_parser as ssp
import yaqutils.args_helpers as uargs
import yaqutils.misc_helpers as umisc


def parse_args():
    parser = argparse.ArgumentParser(description="Extract one serp from serpset as pretty JSON")

    uargs.add_verbosity(parser)
    uargs.add_input(parser, help_message="raw serpset (.jsonl) to inspect")
    uargs.add_output(parser, help_message="one extracted serp (json)")

    parser.add_argument("-n", "--serp-num", type=int, help="serp number (first is 1)")
    parser.add_argument("-t", "--text-pattern", help="text pattern to match")

    return parser.parse_args()


def main():
    cli_args = parse_args()
    umisc.configure_logger(cli_args.verbose)

    text_pattern = cli_args.text_pattern
    serp_num = cli_args.serp_num
    if serp_num == 0:
        serp_num = 1
        logging.info("We've kindly adjusted serp_num to %s", serp_num)

    ssp.extract_one_serp(serpset_file_or_id=cli_args.input_file, serp_file=cli_args.output_file,
                         serp_num=cli_args.serp_num, text_pattern=text_pattern)


if __name__ == "__main__":
    main()
