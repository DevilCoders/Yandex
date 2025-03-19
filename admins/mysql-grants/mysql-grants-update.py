#!/usr/bin/python

import sys

from mysql_grants import Grants
from mysql_grants import bcolors

def help():
    print(bcolors.BOLDWHITE + "Usage: mysql-grants-update [-g|-n]")
    print(bcolors.DEFAULT + "without args: update grants as usual(old way through secdist)")
    print(bcolors.DEFAULT + "-g : use git repository as grants storage(new way)*")
    print(bcolors.DEFAULT + "-n : only show config changes, don't rewrite and apply")
    print(bcolors.DEFAULT + "-v : verbose output")
    print(bcolors.DEFAULT + "-c : use cached configuration(see cache in /var/cache)")
    print(bcolors.DEFAULT + "* : default repositoy path is git@github.yandex-team.ru:salt-media/<project_name>-secure.")
    print(bcolors.DEFAULT + "* : repositoy path can be overrided in /etc/mysql-configurator/grants.conf")

if __name__ == '__main__':

    # defaults
    legacy = True
    verbose = False
    dryrun = False
    cached = False

    if '-h' in sys.argv:
        help()
        sys.exit(0)

    if '-g' in sys.argv:
        legacy = False
    else:
        legacy = True

    if '-n' in sys.argv:
        dryrun = True
    else:
        dryrun = False

    if '-v' in sys.argv:
        verbose = True
    else:
        verbose = False

    if '-c' in sys.argv:
        cached = True
    else:
        cached = False

    gconf = Grants(legacy, verbose, dryrun)

    if cached:
        gconf.read_cached_config()
    else:
        gconf.fetch_config()

    # prevent remove all grants if configuration is empty
    if len(gconf.grants) < 1:
        print("Grants configuration not found, exit with no changes.")
        sys.exit(0)

    gconf.expand_grants()

    gconf.read_existing_grants()
    gconf.build_users()

    gconf.build_grans_sql()
    gconf.update_grants()


    print("Done. Grants changed: %s; Failed statements: %s" % ((len(gconf.sql) - gconf.failed), gconf.failed))
