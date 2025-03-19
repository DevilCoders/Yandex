#!/usr/bin/env python
import sys
import json

import core
import helpers
import grants
from helpers import CONSOLE as log

DEFAULT_GRANTS_CACHE = "/var/cache/mongo-grants/mongo_grants_config"


def main():
    """ The main entry point """
    args = parse_args()

    log.debug_level = args.debug

    log.info("Grants file: {0}".format(args.local))
    parsed_grants = grants.get_grants(args.local)

    if args.debug > 2:
        log.notice("{0}> Grants content <{0}".format("=" * 8))
        log.debug(json.dumps(parsed_grants, indent=2, sort_keys=True, default=str))
        log.notice("{0}====+ END +======={0}".format("=" * 8))

    for mode in ('mongos', 'mongodb'):
        if args.replica and mode == 'mongos':
            continue
        if args.shard and mode == 'mongodb':
            continue
        log.warn("Update users on mode '{0}'".format(mode))
        new_admin, users = grants.merge_common_users(parsed_grants, mode)

        admin = helpers.mongorc(mode)
        if admin:  # For safe update admin
            admin["roles"] = {"admin": grants.DEF_ADMIN_GRANTS}
        else:
            log.debug("Can't get admin from mongorc, use from grants file")
            admin = new_admin

        if args.init:
            log.ok("Initialize auth for mode {0}".format(mode))
            # http://docs.mongodb.org/manual/core/authentication/#localhost-exception
            # now localhost exception work only from master
            client = helpers.connect(mode=mode, init=True)
            if args.dry_run:
                admin = None
            else:
                admin = new_admin
                client['admin'].add_user(new_admin['name'], new_admin['passwd'])
            log.ok("Admin user added")

        helpers.mongorc(mode, user=new_admin, dry_run=args.dry_run)
        helpers.write_etc_file(new_admin)

        if not args.rc:
            try:
                client = helpers.connect(mode=mode, user=admin)
            except Exception:                                 # pylint: disable=W0702
                log.exception(sys.exc_info())
                continue

            log.ok("Expand special grants macro")
            users = grants.expand_all(client, users)
            log.ok("Add users from mode {0}".format(mode))
            for _, data in users.items():
                core.add_user(client, data, dry_run=args.dry_run)

            log.ok("Clean users from mode {0}".format(mode))
            users = core.cleanup_users(client, users, args.clean, args.dry_run)


def parse_args():
    """ Parses args, did you not think of it? """
    import argparse
    parser = argparse.ArgumentParser(
        description='Mongo grants update.',
        formatter_class=lambda prog: argparse.HelpFormatter(
            prog,
            max_help_position=27,
            width=120,
        )
    )
    parser.add_argument('--init', action='store_true', required=False,
                        help='Initiate autorization (run once immediately after the authorization is enabled)')
    parser.add_argument('-c', '--clean', action='store_true', required=False, help='Remove users without grants')
    parser.add_argument('-n', '--dry-run', action='store_true', required=False,
                        help='Perform a trial run with no changes made')

    parser.add_argument('-r', '--replica', action='store_true', required=False,
                        help='Update replica(mongod) users (default both, replica and shard)')
    parser.add_argument('-s', '--shard', action='store_true', required=False,
                        help='Update shard(mongos) users (default both, replica and shard)')

    parser.add_argument('-l', '--local', metavar='path', type=str, default=DEFAULT_GRANTS_CACHE, required=False,
                        help='Grants file name on local fs (default {})'.format(DEFAULT_GRANTS_CACHE))

    parser.add_argument('-d', '--debug', action='count', required=False, help='Debug mode')
    parser.add_argument('-u', '--user',  metavar="u", help='Ignored')
    parser.add_argument('-p', '--project', metavar="p", help='Ignored')

    rc_group = parser.add_argument_group('mongorc file')
    rc_group.add_argument('--rc', action='store_true', required=False, help='Update only rc file')

    args = parser.parse_args()
    log.debug_level = args.debug
    log.trace("My arguments {0}".format(args))

    return args
