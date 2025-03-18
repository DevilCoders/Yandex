#!/skynet/python/bin/python
import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import copy

import gencfg
from gaux.aux_colortext import red_text
from core.argparse.parser import ArgumentParserExt
import core.argparse.types as argparse_types

INDENT = ' ' * 4


def print_stats(indent, data):
    min_value = min(data)
    min_count = len([x for x in data if x == min_value])
    warn = ' (%d of %d shards lost)' % (min_count, len(data)) if not min_value else ''
    print '%s %d avg %d max %d min%s %.2f (max - min)/max' % (
    indent, sum(data) / len(data), max(data), min(data), warn, (max(data) - min(data)) / (max(data) + 0.00000001))


def lost_power(intlookup, cond):
    result = 0.

    for brigade_group in intlookup.get_multishards():
        for brigade in brigade_group.brigades:
            if cond(brigade.basesearchers[0][0]):
                for lst in brigade.basesearchers:
                    for x in lst:
                        if x.power < brigade.power:
                            print red_text("Instances %s:%s with %f power must have at least %f power" % (
                            x.host.name, x.port, x.power, brigade.power))
                        else:
                            result += x.power - brigade.power

    return result


def get_parser():
    parser = ArgumentParserExt(description="Show shards power of intlookup")
    parser.add_argument("-i", "--intlookup", type=argparse_types.intlookup, required=True,
                        help="specify intlookup file")
    parser.add_argument("-x", "--extra-filter", type=argparse_types.pythonlambda, default=lambda x: True,
                        help="Calculate power with extra filter")
    parser.add_argument("-f", "--full-mode", action="store_true", default=False,
                        help="enable full mode")
    parser.add_argument("-c", "--check", dest="check_constraints", action="store_true", default=False,
                        help="check some constraints and show warnings")
    parser.add_argument("-b", "--brief", dest="brief", action="store_true", default=False,
                        help="show brief info")
    parser.add_argument("-B", "--brief-with-swtypes", dest="brief_swtypes", action="store_true", default=False,
                        help="show brief info with both switch types")

    return parser


def brief_statistics(config, show_swtypes):
    iterations = [(-1, 'General shard power:')] + (
    [(0, 'Switch0-only shard power:'), (1, 'Switch1-only shard power:')] if show_swtypes else [])
    for switch, head in iterations:
        print
        print head
        config_copy = copy.copy(config)
        if switch != -1:
            # config_copy = copy.copy(config)
            config_copy.filter_brigades(lambda b: b.switch_type == switch)

        overall_power = config_copy.calc_brigade_power()
        print_stats(INDENT + "Power:", overall_power)
        durability = config_copy.calc_durability()

        for attribute, min_power, points in durability:
            print INDENT + "Durability: %d min if %s {%s} fails" % (min_power, attribute, ','.join(points))

        if switch == -1:
            print

            used_dcs = sorted(list(set(map(lambda x: x.host.dc, config.get_used_base_instances()))))
            without_dc_power = dict(map(lambda x: (x, config.calc_brigade_power(without_dc=x)), used_dcs))
            for dc in sorted(without_dc_power.keys()):
                print_stats(INDENT + "Without dc %s: " % dc, without_dc_power[dc])

            print
            used_queues = sorted(list(set(map(lambda x: x.host.queue, config.get_used_base_instances()))))
            without_queue_power = dict(map(lambda x: (x, config.calc_brigade_power(without_queue=x)), used_queues))
            for queue in sorted(without_queue_power.keys()):
                print_stats(INDENT + "Without queue %s: " % queue, without_queue_power[queue])


def main(options):
    FULL = options.full_mode

    config_base_instances = options.intlookup.get_base_instances()
    config_int_instances = options.intlookup.get_int_instances()
    config_intl2_instances = options.intlookup.get_intl2_instances()

    print 'Info:'
    print INDENT + (' '.join(options.intlookup.tiers) if options.intlookup.tiers is not None else 'unnamed') + " tiers"
    print INDENT + str(options.intlookup.brigade_groups_count) + " groups (" + str(
        options.intlookup.brigade_groups_count * options.intlookup.hosts_per_group) + " instances)"
    print INDENT + str(len(config_base_instances)) + " base instances"
    print INDENT + str(len(config_int_instances)) + " int instances"
    print INDENT + str(len(config_intl2_instances)) + " intl2 instances"

    used_queues = sorted(list(set(map(lambda x: x.host.queue, options.intlookup.get_used_base_instances()))))
    used_dcs = sorted(list(set(map(lambda x: x.host.dc, options.intlookup.get_used_base_instances()))))
    print INDENT + ' '.join(used_queues) + " used queues"
    print INDENT + ' '.join(used_dcs) + " used dcs"

    # calculate used power
    print INDENT + str(sum(map(lambda x: x.power, config_base_instances))) + " used power"
    print INDENT + "%f" % lost_power(options.intlookup, lambda x: True) + " lost power"

    if options.brief or options.brief_swtypes:
        brief_statistics(options.intlookup, options.brief_swtypes)
    else:
        # print averaged values
        # with all searchers enabled
        overall_power = options.intlookup.calc_brigade_power()
        print "Average group power values:"
        print_stats(INDENT + "Overall: ", overall_power)

        if FULL == 1:
            without_queue_power = dict(
                map(lambda x: (x, options.intlookup.calc_brigade_power(without_queue=x)), used_queues))
            print "Averaged group power values (without queue):"
            for queue in sorted(without_queue_power.keys()):
                print_stats(INDENT + "Without queue %s: " % queue, without_queue_power[queue])
            durability = options.intlookup.calc_durability()
            print "Durability:"
            for attribute, min_power, points in durability:
                print INDENT + "Min power %d: fuckup at any %s in (%s)" % (min_power, attribute, ','.join(points))

        without_dc_power = dict(map(lambda x: (x, options.intlookup.calc_brigade_power(without_dc=x)), used_dcs))
        print "Averaged groups power values (without DC):"
        for dc in sorted(without_dc_power.keys()):
            print_stats(INDENT + "Without dc %s: " % dc, without_dc_power[dc])

        if options.extra_filter:
            print_stats(INDENT + "Without filtered: ",
                        options.intlookup.calc_brigade_power(without_flt=options.extra_filter))


if __name__ == '__main__':
    options = get_parser().parse_cmd()
    #    options = Options.parse_cmd()
    main(options)
