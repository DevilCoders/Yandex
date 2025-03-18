import argparse

from tools.nots.cli.commands.bundle_webpack import bundle_webpack_parser
from tools.nots.cli.commands.create_node_modules import create_node_modules_parser
from tools.nots.cli.commands.compile_ts import compile_ts_parser


def parse_args():
    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers()

    bundle_webpack_parser(subparsers)
    create_node_modules_parser(subparsers)
    compile_ts_parser(subparsers)

    return parser.parse_args()


def main():
    args = parse_args()
    args.func(args)


if __name__ == "__main__":
    main()
