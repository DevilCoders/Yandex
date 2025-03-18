#!/usr/bin/env python3
import argparse
import serp.serpset_converter as ss_conv

import yaqutils.args_helpers as uargs
import yaqutils.misc_helpers as umisc


def parse_args():
    parser = argparse.ArgumentParser(description="Convert JSON serpset to jsonlines")
    uargs.add_generic_params(parser)

    uargs.add_input(parser)
    uargs.add_input(parser)

    return parser.parse_args()


def main():
    cli_args = parse_args()
    umisc.configure_logger(verbose=cli_args.verbose, quiet=cli_args.quiet)
    ss_conv.convert_serpset_to_jsonlines(serpset_id=None, raw_file=cli_args.input_file,
                                         jsonlines_file=cli_args.output_file)


if __name__ == "__main__":
    main()
