#!/usr/bin/env python2.7

import argparse

import experiment_pool.pool_helpers as upool
import yaqutils.args_helpers as uargs
import yaqutils.json_helpers as ujson
import yaqutils.misc_helpers as umisc


def parse_args():
    parser = argparse.ArgumentParser(description="mstand convert pool to ab format")
    uargs.add_verbosity(parser)
    uargs.add_output(parser)
    uargs.add_input(parser, help_message="mstand pool to convert", multiple=False)

    return parser.parse_args()


def main():
    args = parse_args()
    umisc.configure_logger(args.verbose, args.quiet)
    data_in_ab_format = upool.convert_pool_to_ab_format(upool.load_pool(args.input_file))
    ujson.dump_json_pretty_via_native_json(data_in_ab_format, args.output_file)


if __name__ == "__main__":
    main()
