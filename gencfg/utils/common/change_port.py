#!/skynet/python/bin/python
import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import re

import gencfg
from core.db import CURDB
from core.argparse.parser import ArgumentParserExt
import core.argparse.types as argparse_types
from core.exceptions import UtilNormalizeException
from gaux.aux_utils import correct_pfname
from utils.common import find_most_unused_port
from core.igroups import CIPEntry


def get_parser():
    parser = ArgumentParserExt(description="Utility to change instance port of group instances")
    parser.add_argument("--db", type=argparse_types.gencfg_db, default=CURDB,
                        help="Optional. Path to db")
    parser.add_argument("-g", "--group", type=argparse_types.group, required=True,
                        help="Obligatory. Group name.")

    port_options = parser.add_mutually_exclusive_group(required=True)
    port_options.add_argument("-p", "--port", type=int, default=None,
                        help="Optional. First port")
    port_options.add_argument("--port-func", type=str, default=None,
                        help="Optional. Port func (ie: new23456 | old8908 | auto)")

    parser.add_argument("-s", "--port-step", type=int, default=None,
                        help="Optional. Port step, is used only with --port")
    parser.add_argument("--no-apply", action="store_true", default=False,
                        help="Optional. Do not apply changes")


    return parser


def normalize(options):
    if options.port is not None and options.port_step is None:
        raise UtilNormalizeException(correct_pfname(__file__), ["port_step"],
                                     "You must specify --port-step when specifying --port")
    if options.port_step is not None and options.port is None:
        raise UtilNormalizeException(correct_pfname(__file__), ["port"],
                                     "You must specify --port when specifying --port-step")
    if options.port is not None and (options.port <= 0 or options.port >= 31768):
        raise UtilNormalizeException(correct_pfname(__file__), ["port"],
                                     "Invalid port value %s: must be in range (0, 32768)" % options.port)
    if options.port_step is not None and options.port_step not in [1, 8]:
        raise UtilNormalizeException(correct_pfname(__file__), ["port_step"],
                                     "Invalid port step value %s: must be one of [1, 8]" % options.port_step)
    if options.port_func is not None and not re.match("(old\d+|new\d+|auto)$", options.port_func):
        raise UtilNormalizeException(correct_pfname(__file__), ["port_func"],
                                     "Invalid port_func value %s" % options.port_func)


def main(options):
    # calculate port and port step
    if options.port_func is not None:
        m = re.match("old(\d+)$", options.port_func)
        if m:
            options.port = int(m.group(1))
            options.port_step = 1
        else:
            m = re.match("new(\d+)$", options.port_func)
            if m:
                options.port = int(m.group(1))
                options.port_step = 8
            else:
                if options.port_func == "auto":
                    options.port = find_most_unused_port.jsmain({'port_range': 8, 'db': options.db}).port
                    options.port_step = 8
                else:
                    raise Exception("Can not parse port_func %s" % options.port_func)

    def replace_func(instance, group_name=options.group.card.name):
        if instance.type != group_name:
            return [False]
        new_instance = options.db.groups.get_instance_by_N(instance.host.name, instance.type, instance.N)
        new_instance.power = instance.power
        instance.copy_from(new_instance)
        if options.group.custom_instance_power_used:
            options.group.custom_instance_power[CIPEntry(instance)] = instance.power
        return [False]

    if options.port_step not in [1, 8]:
        raise Exception('For non-automated groups the only allowed values for port step is 1 or 8.')
    instancePort = '%s%s' % ('old' if options.port_step == 1 else 'new', options.port)
    if options.group.card.legacy.funcs.instancePort != instancePort:
        old_instances = options.group.get_instances()
        base_group_name = options.group.card.name if not options.group.card.name.endswith(
            '_INT') else options.group.card.name[:-len('_INT')]
        intlookups = options.db.intlookups.get_intlookups()
        # noinspection PyUnusedLocal
        intlookups = [intlookup for intlookup in intlookups if intlookup.base_type == base_group_name]
        options.group.card.legacy.funcs.instancePort = instancePort
        options.group.refresh_after_card_update()
        for instance in old_instances:
            replace_func(instance)

    options.group.mark_as_modified()
    for intlookup in map(lambda x: options.db.intlookups.get_intlookup(x), options.group.card.intlookups):
        intlookup.mark_as_modified()

    if not options.no_apply:
        options.db.groups.update(smart=True)
        options.db.intlookups.update(smart=True)


def jsmain(d):
    options = get_parser().parse_json(d)
    normalize(options)
    return main(options)


if __name__ == '__main__':
    options = get_parser().parse_cmd()
    normalize(options)
    main(options)
