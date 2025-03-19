#!/usr/bin/env python2
# -*- coding: utf-8 -*-
import argparse
import json
import logging
import os
import sys

from juggler_sdk import JugglerApi, const


def run_cli(generator):
    logging.basicConfig(level=logging.INFO)
    parser = argparse.ArgumentParser(description='Generate juggler checks.')
    parser.add_argument('--apply', action='store_true',
                        help='apply changes on juggler server (no-dry run)')
    parser.add_argument('--force', action='store_true', help='force changes')
    generator.add_arguments(parser)

    def run(args):
        generate(args, generator)

    parser.set_defaults(target=run)

    args = parser.parse_args()
    args.target(args)


def generate(args, generator):
    if args.apply:
        print("Going to generate checks, save to file, and APPLY.")
    else:
        print("No-apply (dry run) mode: generate, save file,"
              + " send dry run (validation) request to server.")

    g = generator(args)
    filename = "{}.json".format(g.filename())
    checks = g.checks()
    write_dump(filename, checks)

    # NOTE(skipor): copy-paste from juggler-sdk cli.py
    client = JugglerApi("http://juggler-api.search.yandex.net", dry_run=not args.apply,
                        oauth_token=os.environ.get(const.OAUTH_TOKEN_ENV_VAR))
    changed_checks = []
    for check in checks:
        result = client.upsert_check(check, force=args.force)
        if result.changed:
            changed_checks.append((check.host, check.service))

    removed_checks = client.cleanup().removed
    if changed_checks:
        print("Changed or created checks:")
        printChecks(changed_checks)
    if removed_checks:
        print("Removed checks:")
        printChecks(removed_checks)
    if not changed_checks and not removed_checks:
        print("Nothing changed - checks are in sync.")
    elif not args.apply:
        print("Validate changes in '{}', and then apply changes run command with '--apply' flag."
              .format(filename))

    if not args.apply:
        return

    print("Changes successfully applied!")
    if len(checks) == 0:
        return

    hosts = set()
    print("See result:")
    for check in checks:
        if check.host in hosts:
            continue
        hosts.add(check.host)
        print("  https://juggler.yandex-team.ru/aggregate_checks/?query=host%3D{}".
              format(check.host))



def write_dump(output_path, checks_list):
    def write(stream):
        json.dump([x.to_dict(minimize=True) for x in checks_list], stream, sort_keys=True, indent=4)

    if output_path == '-':
        write(sys.stdout)
    else:
        with open(output_path, "w") as stream:
            write(stream)
        print("{0} checks written to {1}".format(len(checks_list), output_path))


def printChecks(checks):
    print('\n'.join('  {}:{}'.format(h, s) for h, s in checks))
