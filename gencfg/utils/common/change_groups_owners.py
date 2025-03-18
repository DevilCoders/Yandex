#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from argparse import ArgumentParser
import datetime

import gencfg
from core.db import CURDB
from gaux.aux_colortext import red_text
import core.argparse.types as argparse_types
from core.exceptions import UtilNormalizeException
from core.settings import SETTINGS
from core.card.types import Date
from gaux.aux_utils import correct_pfname
from gaux.aux_staff import get_possible_group_owners


class EActions(object):
    CHANGE = "change" # change group owners according to input params
    FIXFIRED = "fixfired" # remove fired people for group owners
    ALL = [CHANGE, FIXFIRED]


def parse_cmd():
    parser = ArgumentParser(description="Add extra replicas to intlookup (from free instances)")
    parser.add_argument("-a", "--action", type=str, default="change",
                        choices = EActions.ALL,
                        help="Optional. Action to execute")
    parser.add_argument("-f", "--filter", type=argparse_types.pythonlambda, dest="flt", default=None,
                        help="Optional. Filter on groups to add or remove owners")
    parser.add_argument("-g", "--groups", type=argparse_types.xgroups, default=None,
                        help="Optional. List of groups to process")
    parser.add_argument("-d", "--to-add", type=argparse_types.comma_list, dest="add", required=False, default=[],
                        help="Optional. List of users to add")
    parser.add_argument("-r", "--to-remove", type=argparse_types.comma_list, dest="remove", required=False, default=[],
                        help="Optional. List of users to remove")
    parser.add_argument("-s", "--to-set", type=argparse_types.comma_list, dest="to_set", default=None,
                        help="Optional. Set specified list of users as owners (mutually exclusive with <-a> and <-r> options")
    parser.add_argument("--update-watchers", action="store_true", default=False,
                        help="Optional. Update watchers instead of owners")
    parser.add_argument('--slaves-as-well', action='store_true', default=False,
                        help='Optional. Change owners for slave groups as well')
    parser.add_argument("-y", "--apply", action="store_true", default=False,
                        help="Optional. Apply changes")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    return options


def normalize(options):
    if options.flt is None and options.groups is None:
        raise UtilNormalizeException(correct_pfname(__file__), ["filter", "groups"],
                                     "You must specify at least one of <--filter> <--groups> option")

    if options.to_set is not None:
        if options.add != [] or options.remove != []:
            raise UtilNormalizeException(correct_pfname(__file__), ["set", "add", "to_set"],
                                         "Options <--to-set> and <--to-add> <--to-remove> are mutually exclusive")

    if options.flt is None:
        options.flt = lambda x: True
    if options.groups is None:
        options.groups = CURDB.groups.get_groups()


def change_and_show(group, attrname, to_add, to_remove, to_set, allow_users, from_cmd):
    oldlist = getattr(group.card, attrname)

    if to_set is None:
        newlist = filter(lambda x: x not in to_remove, oldlist)
        if allow_users is not None:
            newlist = filter(lambda x: x in allow_users, newlist)
        newlist.extend(filter(lambda x: x not in newlist, to_add))
    else:
        newlist = to_set

    if from_cmd and set(oldlist) != set(newlist):
        print "Group %s changed %s: was <%s> become <%s>" % (group.card.name, attrname, ','.join(sorted(oldlist)), ','.join(sorted(newlist)))
        group.mark_as_modified()
    setattr(group.card, attrname, newlist)


def main(options, from_cmd=True):
    if options.action == EActions.CHANGE:
        allow_users = None
    else:
        if CURDB.version < '2.2.48':
            # do nothing
            allow_users = {x.name for x in CURDB.users.get_users()}
        else:
            allow_users = {x.name for x in CURDB.users.get_users() if x.retired_at is None}
            allow_users |= {x.name for x in CURDB.users.get_users() if (x.retired_at is not None) and ((datetime.date.today() - x.retired_at.date).days < 7)}
            allow_users |= {x.name for x in CURDB.staffgroups.get_all()}
            if CURDB.version < '2.2.56':
                # =========================================== RX-447 START ===========================================
                for abcgroup in CURDB.abcgroups.get_all():
                    allow_users.add('abc:{}'.format(abcgroup.name))
                    for role_id in abcgroup.roles:
                        allow_users.add('abc:{}:role={}'.format(abcgroup.name, role_id))
                # =========================================== RX-447 START ===========================================
            else:
                allow_users |= CURDB.abcgroups.available_abc_services()

    if options.slaves_as_well:
        slave_groups = sum((x.card.slaves for x in options.groups), [])
        options.groups.extend(slave_groups)

    # traverse all groups and fix owners
    for group in options.groups:
        if options.flt(group):
            if options.action == EActions.CHANGE:
                if options.update_watchers:
                    change_and_show(group, 'watchers', options.add, options.remove, options.to_set, allow_users, from_cmd)
                else:
                    change_and_show(group, 'owners', options.add, options.remove, options.to_set, allow_users, from_cmd)
            elif options.action == EActions.FIXFIRED:
                change_and_show(group, 'watchers', options.add, options.remove, options.to_set, allow_users, from_cmd)
                change_and_show(group, 'owners', options.add, options.remove, options.to_set, allow_users, from_cmd)
            else:
                raise Exception, "Unknown action <%s>" % options.action

    if options.apply:
        CURDB.update(smart=True)
    else:
        if from_cmd:
            print red_text("Not updated!!! Add option -y to update.")


if __name__ == '__main__':
    options = parse_cmd()

    normalize(options)

    main(options)
