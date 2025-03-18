#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg
from core.db import CURDB
import core.argparse.types as argparse_types
from core.histdb.events import EEventTypes
from core.argparse.parser import ArgumentParserExt


def get_parser():
    parser = ArgumentParserExt(description="Host histdb events")
    parser.add_argument("-t", "--event-type", dest="event_type", type=str, default=None,
                        choices=EEventTypes.ALL,
                        help="Optional. Event type: %s" % ','.join(EEventTypes.ALL))
    parser.add_argument("-n", "--event-name", dest="event_name", type=str, default=None,
                        help="Optional. Event name (different event names for every type: ADDED REMOVED ...)")
    parser.add_argument("-o", "--event-object-id", dest="event_object_id", type=str, default=None,
                        help="Optional. Object name (for group events - group name, for host events - host name, ...)")
    parser.add_argument("-f", "--filter", dest="flt", type=argparse_types.pythonlambda, default=None,
                        help="Optional. Filter on event object")
    # spceiatl options to specify group, host, ...
    parser.add_argument("-g", "--group", dest="group", type=str, default=None,
                        help="Optional. Events for specific group")
    parser.add_argument("-s", "--host", dest="host", type=str, default=None,
                        help="Optonal. Events for specific host")

    return parser


def parse_cmd():
    parser = get_parser()

    if len(sys.argv) == 1:
        sys.argv.append('-h')

    options = parser.parse_args()

    if len(filter(lambda x: x is not None, [options.event_object_id, options.group, options.host])) > 1:
        raise Exception("You can specify at most one of <--event-object-id --group --host> option")

    if options.group is not None:
        options.event_type = 'GROUP'
        options.event_object_id = options.group
    if options.host is not None:
        options.event_type = 'HOST'
        options.event_object_id = options.host

    return options


def parse_json(request):
    parser = get_parser()
    return parser.parse_json(request)


def main(options):
    def flt(x):
        if options.event_type and options.event_type != x.event_type:
            return False
        if options.event_name and options.event_name != x.event_name:
            return False
        if options.event_object_id and options.event_object_id != x.event_object.object_id:
            return False
        if options.flt and not options.flt(x):
            return False
        return True

    events = CURDB.histdb.get_events(event_type=options.event_type, event_name=options.event_name,
                                     event_object_id=options.event_object_id, flt=flt)

    return events


if __name__ == '__main__':
    options = parse_cmd()

    events = main(options)

    for event in events:
        print event
