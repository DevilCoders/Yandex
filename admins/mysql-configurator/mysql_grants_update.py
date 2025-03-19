#!/usr/bin/python
"""
mysql-grants-update (4 Series). Updates mysql server grants
"""
from __future__ import print_function
import sys
import argparse
from mysql_configurator import load_config
from mysql_configurator.grants import Grantushka


def parse_args():
    """
    cli arguments parser
    :return: args
    """
    argsparser = argparse.ArgumentParser(
        description="mysql-grants-update (4 series).Without args use old way through secdist.",
        epilog='*: default repository path is'
               'git@github.yandex-team.ru:salt-media/<project_name>-secure. '
               'Repository path can be overrided in /etc/mysql-configurator/grants.conf')
    argsparser.add_argument('-v', '--verbose',
                            help='use verbose output', default=False, action='store_true')
    argsparser.add_argument('-c', '--cached',
                            help='use cached configuration', default=False, action='store_true')
    argsparser.add_argument('-n', '--dryrun',
                            help='dryrun(do not do anything, just show changes)',
                            default=False, action='store_true')
    argsparser.add_argument('-l', '--locals',
                            help='only show all defined users and exit',
                            default=False, action='store_true')
    argsparser.add_argument('-a', '--apihost',
                            help='API host to use for grants configs path(def: c.yandex-team.ru)',
                            default='c.yandex-team.ru', action='store')
    arguments = argsparser.parse_args()
    return arguments

def main():
    """main"""
    arguments = parse_args()

    if not arguments.verbose:
        config = load_config(force_lvl='INFO')
    else:
        config = load_config()

    config.grants.cached = True if arguments.cached else config.grants.cached
    config.grants.dryrun = True if arguments.dryrun else False

    # load grants configuration
    grantushka = Grantushka(config)
    # init hosts and users resolve
    grantushka.processhosts()
    grantushka.generatemyusers()
    # generate privileges
    grantushka.generate_sql()

    # only print users and exit
    if arguments.locals:
        for user in grantushka.conf['users']:
            print(user, grantushka.conf['users'][user])
        sys.exit(0)

    # update grants
    grantushka.update()


if __name__ == '__main__':
    main()
