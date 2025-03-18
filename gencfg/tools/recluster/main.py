#!/skynet/python/bin/python
"""
    Most of groups have section recluster (list of console commands to performa recluster of specified group). This script
    performs steps, specified in that section until all finished or one of command failed
"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg
from core.argparse.parser import ArgumentParserExt
import core.argparse.types
import gaux.aux_utils
from core.db import CURDB, DB
from gaux.aux_colortext import green_text, dblue_text, red_text

from tools.recluster.rcommand import TReclusterCommand


class EActions(object):
    SHOW = "show"  # show current status of recluster
    RECLUSTER = "recluster"  # recluster list of specified commands
    ALL = [SHOW, RECLUSTER]


ALL_STEPS = "all"


def get_parser():
    parser = ArgumentParserExt(description="Recluster group")
    parser.add_argument("-a", "--action", type=str, default=None,
                        choices=EActions.ALL,
                        help="Obligatory. Action to execute")
    parser.add_argument("-g", "--group", type=core.argparse.types.group, required=True,
                        help="Obligatory. Group to process")
    parser.add_argument("-c", "--commands", type=core.argparse.types.comma_list, default=[],
                        help="Optional. List of steps to execute for action %s (<%s> to perfrom all steps from the beginning) " % (EActions.RECLUSTER, ALL_STEPS))
    parser.add_argument("-v", "--verbose", action="count", dest="verbose_level", default=0,
                        help="Optional. Verbose mode. Multiple -v options increase the verbosity. The maximum is 1.")

    return parser


def normalize(options):
    if options.action is None:
        if options.commands != []:
            options.action = EActions.RECLUSTER
        else:
            raise Exception("You must specify <--action> optoin")

    if options.action == EActions.SHOW:
        if options.commands:
            raise Exception("You can not specify <--commands> option in action <%s>" % EActions.SHOW)
    elif options.action == EActions.RECLUSTER:
        if not options.commands:
            raise Exception("You must specify <--commands> option in action <%s>" % EActions.RECLUSTER)

    return options


def main(options):
    commands = []
    for elem in options.group.card.recluster.cleanup:
        commands.append(TReclusterCommand(options.group, "cleanup", elem))
    for elem in options.group.card.recluster.alloc_hosts:
        commands.append(TReclusterCommand(options.group, "alloc_hosts", elem))
    for elem in options.group.card.recluster.generate_intlookups:
        commands.append(TReclusterCommand(options.group, "generate_intlookups", elem))

    if options.action == EActions.SHOW:
        print "Group %s:" % options.group.card.name
        print "    commmands:"
        for command in commands:
            print gaux.aux_utils.indent(command.as_str(), ind='        ')
    elif options.action == EActions.RECLUSTER:
        # expand list for commands to proccess, replacing <all> by all steps, <clenaup> by all cleanup steps, ...
        to_process = []
        for command_name in options.commands:
            if command_name == ALL_STEPS:
                to_process.extend(map(lambda x: x.command_name, commands))
            elif command_name in ["cleanup", "alloc_hosts", "generate_intlookups"]:
                to_process.extend(command.command_name for command in commands if command.section_name == command_name)
            else:
                filtered = filter(lambda x: x.command_name == command_name, commands)
                if len(filtered) == 1:
                    to_process.append(command_name)
                else:
                    raise Exception("Command <%s> not found in recluster list for group <%s>" % (command_name, options.group.card.name))

        # simple validation
        dublicates = set(filter(lambda x: to_process.count(x) > 1, to_process))
        if len(dublicates) > 0:
            raise Exception("Found dublicates <%s> in opearation list <%s>" % (",".join(dublicates), ",".join(to_process)))

        def find_index(command_name):
            for index, command in enumerate(commands):
                if command.command_name == command_name:
                    return index
            raise Exception("Command name <%s> not found" % command_name)

        to_process_indexes = map(lambda x: find_index(x), to_process)
        for index in xrange(1, len(to_process)):
            if to_process_indexes[index] < to_process_indexes[index - 1]:
                raise Exception("Found commands <%s> and <%s>, which should be executed in reverse orders" % (to_process[index - 1].command_name, to_process[index].command_name))

        to_process_commands = map(lambda x: commands[x], to_process_indexes)
        print "Processing stages: %s" % (",".join(map(lambda x: green_text(x.command_name), to_process_commands)))
        for command in to_process_commands:
            print "    Command <%s>: Processing <%s>" % (
            green_text(command.command_name), dblue_text(command.command_body))

            # run command
            retcode, out, err = command.run_command()
            if retcode == 0:
                print "       Status: %s" % green_text("FINISHED")
            else:
                print """       Status: %s
       Code: %s
       Out:\n%s
       Err:\n%s""" % (
                    red_text("FAILED"), retcode, gaux.aux_utils.indent(out.rstrip("\n "), "            "),
                    gaux.aux_utils.indent(err.rstrip("\n "), "            ")
                )

                raise Exception("Command <%s> failed with status <%s>" % (command.command_body, retcode))
    else:
        raise Exception("Unsupported action <%s>" % options.action)


if __name__ == '__main__':
    options = get_parser().parse_cmd()

    normalize(options)

    main(options)
