#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from StringIO import StringIO

from pkg_resources import require

require('PyYAML')
import yaml

import gencfg
from core.card.updater import CardUpdater
from core.argparse.parser import ArgumentParserExt
from core.db import CURDB
from core.card.node import Scheme
import core.argparse.types as argparse_types
from gaux.aux_colortext import red_text


class EStatuses(object):
    STATUS_OK = 0
    STATUS_FAIL = 1


def get_parser():
    parser = ArgumentParserExt(usage="""
    update_card.py -s
        -- to show card scheme
    update_card.py -g GROUP
        -- to show group card
    update_card.py -g GROUP -k KEY
        -- to show group card key value
    update_card.py -g GROUP -k KEY -v VALUE [OPTIONS]
        -- to change group card value

    KEY is given in form of dot delimited path, i.e. "reqs.instances.memory_guarantee"
    VALUE is given in form of YAML expression, i.e. "true", "5", "hello world"
    """)
    parser.add_argument("--db", type=argparse_types.gencfg_db, default=CURDB,
                        help="Optional. Path to db")
    parser.add_argument("-g", "--groups", type=argparse_types.groups, default=None,
                        help="group to perform action on")
    parser.add_argument("--update-slaves", action="store_true", default=False,
                        help="Optional. Update value in all slave groups from specified ones master-groups as well")
    parser.add_argument("-k", "--key", dest="key", default=None,
                        help="hierarchical key")
    parser.add_argument("-v", "--value", dest="value", default=None,
                        help="value in YAML format; if not set this script will show you key value")
    parser.add_argument("-y", "--apply", action="store_true", default=False,
                        help="Optional. Apply changes")
    parser.add_argument("-s", "--show-scheme", dest="show_scheme", action="store_true", default=False,
                        help="show card scheme")

    return parser


def parse_json(request):
    parser = get_parser()
    return parser.parse_json(request)


def normalize(options):
    if options.key is not None:
        options.key = tuple(options.key.split('.'))

    if options.update_slaves:
        slave_groups = filter(lambda x: x not in options.groups, sum(map(lambda x: x.slaves, options.groups), []))
        options.groups.extend(slave_groups)


def run_for_group(options, group):
    if options.key is None:
        return EStatuses.STATUS_OK, group.save_card_to_str()

    if not options.key:
        raise Exception("Empty key")

    if options.value is None:
        info = CardUpdater().get_group_card_info_old(group)
        found = None
        for item in info:
            if tuple(item['path']) == options.key:
                found = item
                break
        if found is None:
            raise Exception('Key %s does not exist' % options.key)
        return EStatuses.STATUS_OK, str(found['value'])
    else:
        if options.value != '':
            stream = StringIO(options.value)
            try:
                options.value = yaml.load(stream)
            except yaml.parser.ParserError:  # FIXME: kostil
                pass
        if options.value == 'None':
            options.value = None

        updates = {options.key: options.value}

        has_updates, is_ok, invalid_values = CardUpdater().update_group_card(group, updates, mode="util")

        if not is_ok:
            return EStatuses.STATUS_FAIL, "\n".join(invalid_values.values())
        else:
            group.modified = True
            return EStatuses.STATUS_OK, None


def main(options):
    if options.show_scheme:
        scheme = Scheme.get_scheme_as_text()
        scheme = scheme.split('\n')
        scheme = [x.rstrip() for x in scheme if x.rstrip()]
        scheme = [x for x in scheme if not x.lstrip().startswith('#')]
        return EStatuses.STATUS_OK, '\n'.join(scheme), False

    for group in options.groups:
        status, out = run_for_group(options, group)
        if status == EStatuses.STATUS_FAIL:
            raise Exception("Util failed for group <%s> with the following message: <%s>" % (group.card.name, out))

        if options.value is None:
            print out

    if options.apply:
        options.db.update(smart=True)


def jsmain(d):
    options = get_parser().parse_json(d)
    normalize(options)
    return main(options)


if __name__ == '__main__':
    options = get_parser().parse_cmd()

    normalize(options)

    main(options)

    if options.value is not None and options.apply == False:
        print red_text("Not updated!!! Add option -y to update.")
