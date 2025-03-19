#!/usr/bin/env python
"""
This script helps to edit playbooks.
"""

import sys
from argparse import ArgumentParser

import ruamel.yaml

__author__ = "Andrey Trubachev <d3rp@yandex-team.ru>"


class Playbook(object):
    def __init__(self, file=None):
        self.load(file)

    def load(self, file=None):
        if file is None:
            self.content = ruamel.yaml.round_trip_load(sys.stdin)
        else:
            with open(file, "r") as stream:
                self.content = ruamel.yaml.round_trip_load(stream)

    def dump(self, file=None):
        if file is None or file == "-":
            ruamel.yaml.round_trip_dump(self.content, sys.stdout)
        else:
            with open(file, "w") as stream:
                ruamel.yaml.round_trip_dump(self.content, stream)

    def format(self, output_file):
        self.dump(output_file)

    def add_key(self, key, value):

        def find_and_add_key(d, keys, value):
            if len(keys) == 1:
                cur_value = d.get(keys[0])
                new_value = ruamel.yaml.load(value)
                if cur_value is None:
                    d[keys[0]] = new_value
                else:
                    if isinstance(cur_value, str) or isinstance(cur_value, int) or isinstance(cur_value, float):
                        if isinstance(new_value, list):
                            new_value.append(cur_value)
                            d[keys[0]] = new_value
                        elif isinstance(new_value, str) or isinstance(cur_value, int) or isinstance(cur_value, float):
                            d[keys[0]] = [cur_value, new_value]
                    elif isinstance(cur_value, list):
                        if isinstance(new_value, list):
                            d[keys[0]] = cur_value + new_value
                        elif isinstance(new_value, str) or isinstance(cur_value, int) or isinstance(cur_value, float):
                            cur_value.append(new_value)
                            d[keys[0]] = cur_value
            else:
                find_and_add_key(d.get(keys[0]), keys[1:], value)

        for i in self.content:
            find_and_add_key(i, key.split("."), value)

    def remove_key(self, key):

        def find_and_remove_key(d, keys):
            if len(keys) == 1:
                d.pop(keys[0], None)
            else:
                find_and_remove_key(d.get(keys[0]), keys[1:])

        for i in self.content:
            find_and_remove_key(i, key.split("."))

    def set_key(self, key, value):

        def find_and_set_key(d, keys, value):
            if len(keys) == 1:
                d[keys[0]] = ruamel.yaml.load(value)
            else:
                find_and_set_key(d.get(keys[0]), keys[1:], value)

        for i in self.content:
            find_and_set_key(i, key.split("."), value)

    def get_key(self, key):

        def find_key(d, keys):
            if len(keys) == 1:
                return d.get(keys[0])
            else:
                return find_key(d.get(keys[0]), keys[1:])

        result = []
        for i in self.content:
            result.append(find_key(i, key.split(".")))

        return result if len(result) > 1 else result[0]


def pb_format(args):
    pb = Playbook(args.input_file)
    pb.format(args.output_file)


def pb_add_key(args):
    if args.key is None or args.value is None:
        sys.stderr.write("Missing arguments `key` and/or `value`.\n")
        return 1

    pb = Playbook(args.input_file)
    pb.add_key(args.key, args.value)
    pb.dump(args.output_file)

    return 0


def pb_remove_key(args):
    if args.key is None:
        sys.stderr.write("Missing argument `key`.\n")
        return 1

    pb = Playbook(args.input_file)
    pb.remove_key(args.key)
    pb.dump(args.output_file)

    return 0


def pb_set_key(args):
    if args.key is None or args.value is None:
        sys.stderr.write("Missing arguments `key` and/or `value`.\n")
        return 1

    pb = Playbook(args.input_file)
    pb.set_key(args.key, args.value)
    pb.dump(args.output_file)

    return 0


def pb_get_key(args):
    if args.key is None:
        sys.stderr.write("Missing argument `key`.\n")
        return 1

    pb = Playbook(args.input_file)
    value = pb.get_key(args.key)
    if value is None:
        return 0
    elif isinstance(value, str):
        sys.stdout.write("{}\n".format(value))
    else:
        ruamel.yaml.round_trip_dump(pb.get_key(args.key), sys.stdout)

    return 0


def main():
    def add_command_parser(parser, command):
        cmd, cmd_desc, cmd_func = command
        cmd_parser = parser.add_parser(cmd, help=cmd_desc)

        if cmd in ["add", "remove", "get", "set"]:
            cmd_parser.add_argument('-k', '--key', type=str, dest='key',
                                    help='The key, can be nested, levels should be separated by dot.')
        if cmd in ["add", "set"]:
            cmd_parser.add_argument('-v', '--value', type=str, dest='value', help='The value of the key.')

        cmd_parser.set_defaults(func=cmd_func)

    commands = [
        ("format", "Format playbook.", pb_format),
        ("add", "Add the key.", pb_add_key),
        ("remove", "Remove the key.", pb_remove_key),
        ("set", "Set the value of the key.", pb_set_key),
        ("get", "Get a value of the key.", pb_get_key),
    ]
    description = "Helps to edit playbooks."
    usage = """pbeditor.py [-h] [-i INPUT_FILE] [-o OUTPUT_FILE]
    {format,add,remove,set,get} ...

The typical cases:
- get a key value
    > ./pbeditor.py -i services/market-common.yml get -k 'vars.tags'
    - market_stable
    - market
    >

- set the key value
    > ./pbeditor.py -i services/market-common.yml set -k 'vars.tags' -v '"market"' | ./pbeditor.py get -k 'vars.tags'
    market
    > ./pbeditor.py -i services/market-common.yml set -k 'vars.tags' -v '["market", "market_stable"]' | ./pbeditor.py get -k 'vars.tags'
    - market
    - market_stable
    >

- add the key value
    > ./pbeditor.py -i services/market-common.yml add -k 'vars.tags' -v 'market_disaster' | ./pbeditor.py get -k 'vars.tags'
    - market_stable
    - market
    - market_disaster
    > ./pbeditor.py -i services/market-common.yml add -k 'vars.tags' -v '["market_sw", "market_hw"]' | ./pbeditor.py get -k 'vars.tags'- market_stable
    - market
    - market_sw
    - market_hw
    >

- remove the key
    > ./pbeditor.py -i services/market-common.yml remove -k 'vars.tags' | ./pbeditor.py get -k 'vars.tags'
    >
"""

    parser = ArgumentParser(description=description, usage=usage)
    parser.add_argument('-i', '--input', type=str, dest='input_file', help='Read playbook from INPUT_FILE instead of stdin.')
    parser.add_argument('-o', '--output', type=str, dest='output_file',
                        help='Write playbook to OUTPUT_FILE instead of stdout.')
    subparser = parser.add_subparsers()
    for command in commands:
        add_command_parser(subparser, command)

    args = parser.parse_args()
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())
