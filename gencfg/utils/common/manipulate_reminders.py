#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import datetime

import gencfg
from core.db import CURDB
import core.argparse.types as argparse_types
from core.argparse.parser import ArgumentParserExt
from core.exceptions import UtilNormalizeException
from gaux.aux_utils import correct_pfname
from core.card.node import CardNode
from core.card.types import Date

ACTIONS = ["add", "show"]


def get_parser():
    parser = ArgumentParserExt(description="Manipulate with reminders (show/remove expired, add new, ...")
    parser.add_argument("-a", "--action", type=str, required=True,
                        choices=ACTIONS,
                        help="Obligatory. Action to execute")
    parser.add_argument("-m", "--message", type=str, default=None,
                        help="Optional. Message for new reminder (action <add>)")
    parser.add_argument("-x", "--expires", type=int, default=None,
                        help="Optional. Expiration date in days since today (action <add>)")
    parser.add_argument("-g", "--groups", type=argparse_types.groups, default=None,
                        help="Optional. List of groups to process (action <add>)")
    parser.add_argument("--show-expired-only", action="store_true", default=False,
                        help="Optional. Show only expired reminders (action <show>)")

    return parser


def normalize(options):
    if options.action == "add":
        for option_name in ["message", "expires", "groups"]:
            if getattr(options, option_name, None) is None:
                raise UtilNormalizeException(correct_pfname(__file__), [option_name],
                                             "Option <%s> must be specified with action <add>" % option_name)

    if options.action == "show":
        if options.groups is None:
            options.groups = CURDB.groups.get_groups()

    non_uniq_groups = set(filter(lambda x: options.groups.count(x) > 1, options.groups))
    if len(non_uniq_groups) > 0:
        raise Exception("Groups %s mentioned at least twice in group list" % (",".join(map(lambda x: x.name, non_uniq_groups))))


def main(options):
    if options.action == "show":
        for group in options.groups:
            group_reminders = group.reminders
            if options.show_expired_only:
                group_reminders = filter(lambda x: x.expires.date < datetime.date.today(), group_reminders)

            if len(group_reminders) == 0:
                continue

            group_reminders.sort(cmp=lambda x, y: cmp(x.expires.date, y.expires.date))
            print "Group %s:" % group.card.name
            for reminder in group_reminders:
                print "    %s: %s" % (reminder.expires, reminder.message)
    elif options.action == "add":
        expires_datetime = Date.create_from_duration(options.expires)
        for group in options.groups:
            new_reminder = CardNode()
            new_reminder.expires = expires_datetime
            new_reminder.message = options.message
            group.card.reminders.append(new_reminder)
            group.mark_as_modified()

            print "Group %s: added reminder with message <%s> , expired at <%s>" % (
            group.card.name, options.message, new_reminder.expires)

        CURDB.groups.update(smart=1)
    else:
        raise Exception("Unknown action <%s>" % options.action)


if __name__ == '__main__':
    options = get_parser().parse_cmd()

    normalize(options)

    main(options)
