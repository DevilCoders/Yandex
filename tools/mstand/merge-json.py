#!/usr/bin/env python3

import argparse

import tools.merge_json
import yaqutils.args_helpers as uargs
import yaqutils.json_helpers as ujson


def parse_args():
    parser = argparse.ArgumentParser(description="Merge multiple JSON files into one")
    uargs.add_output(parser)
    uargs.add_input(parser, "input JSON file(s) to merge", multiple=True)
    return parser.parse_args()


def main():
    args = parse_args()
    pool = tools.merge_json.read_and_merge(args.input_file)
    ujson.dump_to_file(pool, args.output_file)


if __name__ == "__main__":
    main()
