#!/skynet/python/bin/python
import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from argparse import ArgumentParser
from collections import defaultdict

import gencfg
import core.argparse.types as argparse_types
from gaux.aux_colortext import red_text, green_text
from core.db import CURDB
from utils.sky import blinov
from gaux.aux_utils import dict_to_options
from utils.common import show_instance_power, show_instance_groups


def is_good_instance(db, instance, iprops, raise_failed=True):
    if instance.host.is_vm_guest():
        return False

    group = db.groups.get_group(instance.type)

    if group.card.master is not None:
        msg = "Instance %s:%s in in bad group group %s" % (instance.host.name, instance.port, instance.type)
        if raise_failed:
            raise Exception(msg)
        else:
            return False

    # check if group is search group
    if db.version > '0.7':
        if (group.card.properties.nonsearch and iprops.get('allow_nonsearch',
                                                           False) is False) or group.card.properties.background_group:
            msg = "Instance %s%s group %s is non-search group" % (instance.host.name, instance.port, instance.type)
            if raise_failed:
                raise Exception(msg)
            else:
                return False

    return True


def _format_tags(group):
    res = []
    for key, val in sorted(group.card.tags.as_dict().items()):
        if isinstance(val, list):
            for el in val:
                res.append("%s=%s" % (key, el))
        else:
            res.append("%s=%s" % (key, val))

    return ", ".join(res)


def calc_and_show(db, descr, vertical, instances, options, from_cmd=False):
    suboptions = {'instances': instances.items()}
    total_power, total_memory = show_instance_power.main(dict_to_options(suboptions), db)
    total_instances = len(instances)
    total_hosts = len(set(map(lambda x: x.host, instances.keys())))
    if not from_cmd:
        result = (descr, vertical, total_power, total_memory, total_instances, total_hosts)
    else:
        if options.csv:
            print "%s;%s;%s;%s;%s;%s" % (descr, vertical, total_power, total_memory, total_instances, total_hosts)
        else:
            descr += ' (%s)' % vertical
            print green_text("Metagroup <%s>: %s total power, %s total memory, %s instances, %s hosts" % (
            descr, total_power, total_memory, total_instances, total_hosts))
    if options.verbose:
        suboptions['instances'] = map(lambda (x, y): (x, y.get('coeff', 1.0)), suboptions['instances'])
        by_group_data = show_instance_groups.main(dict_to_options(suboptions))

        power_sum = 0

        for gname, instance_percents, power in sorted(by_group_data, key=lambda x: -x[-1]):
            group = db.groups.get_group(gname)
            power_sum += power

            print green_text("    group %s: %.2f %.2f %s %d%%" % (
                gname,
                instance_percents,
                power,
                _format_tags(group),
                power_sum * 100 / total_power,
            ))

    if not from_cmd:
        return result


def parse_cmd():
    parser = ArgumentParser(description="Show metagroups power")

    parser.add_argument("-c", "--config", dest="config", type=argparse_types.yamlconfig, required=True,
                        help="Obligatory. Path to config")
    parser.add_argument("-d", "--db", type=argparse_types.gencfg_db, default=CURDB,
                        help="Optional. Path to db (current db used if not specified)")
    parser.add_argument("-i", "--ignore-intersection", dest="ignore_intersection", action="store_true", default=False,
                        help="Optional. Ignore bug with intersected metagroups")
    parser.add_argument("-o", "--other-statistics", dest="other_statistics", action="store_true", default=False,
                        help="Optional. Show other and all statistics")
    parser.add_argument("-v", "--verbose", dest="verbose", action="store_true", default=False,
                        help="Optional. Show verbose output")
    parser.add_argument("--dump-all-hosts", action="store_true", default=False,
                        help="Optional. Print all hosts")
    parser.add_argument("--csv", action="store_true", default=False,
                        help="Optional. Show main result in csv format")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    for metaproject in options.config:
        metaproject['descr'] = metaproject['descr'].encode('utf-8')

    return options


def main(options, db=CURDB, from_cmd=True):
    if not from_cmd:
        result = {
            'warnings': {
                'failed_instances': [],
                'non_empty_intersection': []
            },
            'metagroups_stats': {},
        }

    searcherlookup = db.build_searcherlookup()
    blinov.correct_searcherlookup(searcherlookup)

    # load instances
    for metaproject in options.config:
        metaproject['instances'] = dict()
        for flt_data in metaproject['filters']:
            iprops = {
                'coeff': float(flt_data.get('coeff', 1.0)),
                'allow_nonsearch': flt_data.get('allow_nonsearch', False)
            }

            parsed_flt = blinov.parse_filter(flt_data['flt'])
            extra_instances = parsed_flt.process(searcherlookup, dict_to_options({'werror': False}))

            already_in_metaproject = list(set(metaproject['instances'].keys()) & set(extra_instances))
            if len(already_in_metaproject):
                pass
#                raise Exception("Metaproject %s(%s) already have instances while applying filter <%s>: %s" % (metaproject['descr'], metaproject['vertical'], flt_data['flt'], ', '.join(map(lambda x: x.name(), already_in_metaproject[:20]))))
            for instance in extra_instances:
                metaproject['instances'][instance] = iprops

        failed_instances = map(lambda (x, y): x, (
        filter(lambda (x, y): not is_good_instance(db, x, y, raise_failed=False),
               metaproject['instances'].iteritems())))

        if len(failed_instances) > 0:
            if not from_cmd:
                result['warnings']['failed_instances'].append((metaproject['descr'], len(failed_instances),
                                                               len(metaproject['instances'].keys()), ','.join(
                    map(lambda x: '%s:%s' % (x.host.name, x.port), failed_instances[:20]))))
            else:
                if options.verbose:
                    print red_text("!!!!! Metagroup <%s> has %d of %d filtered instances: %s" % (
                        metaproject['descr'], len(failed_instances), len(metaproject['instances'].keys()),
                        ','.join(map(lambda x: '%s:%s' % (x.host.name, x.port), failed_instances[:20]))))
        metaproject['instances'] = dict(
            filter(lambda (x, y): is_good_instance(db, x, y, raise_failed=False), metaproject['instances'].iteritems()))

        # fix instances power
        instances_host_count = defaultdict(int)
        for instance in metaproject['instances'].iterkeys():
            instances_host_count[(instance.host, instance.type)] += 1
        for instance in metaproject['instances'].iterkeys():
            instance.power = instance.host.power / instances_host_count[(instance.host, instance.type)]

    # check inersection
    got_error = False
    for i in range(len(options.config)):
        for j in range(i + 1, len(options.config)):
            intersection = list(set(options.config[i]['instances']) & set(options.config[j]['instances']))
            intersection = filter(
                lambda x: options.config[i]['instances'][x]['coeff'] + options.config[j]['instances'][x]['coeff'] > 1.0,
                intersection)
            if len(intersection) > 0:
                if not from_cmd:
                    result['warnings']['non_empty_intersection'].append(
                        (options.config[i]['descr'], options.config[j]['descr'], len(intersection)))
                else:
                    print "!!!!! Non-empty intersection of <%s(%s)> and <%s(%s)> : %d instances (%s)" % (
                        options.config[i]['descr'], options.config[i]['vertical'],
                        options.config[j]['descr'], options.config[j]['vertical'],
                        len(intersection), ','.join(map(lambda x: '%s:%s' % (x.host.name, x.port), intersection[:20])))
                got_error = True
    if got_error and not options.ignore_intersection and from_cmd:
        sys.exit(1)

    # calculate and show results on common groups
    for metaproject in options.config:
        rr = calc_and_show(db, metaproject['descr'], metaproject['vertical'], metaproject['instances'], options,
                           from_cmd)
        if not from_cmd:
            result['metagroups_stats'][metaproject['descr']] = rr

    # calculate remaining power
    if options.other_statistics:
        used_instances = dict()
        for elem in options.config:
            used_instances.update(elem['instances'])

        unused_instances = dict(
            map(lambda x: (x, {}), filter(lambda x: x not in used_instances, searcherlookup.instances)))
        unused_instances = dict(
            filter(lambda (x, y): is_good_instance(db, x, y, raise_failed=False), unused_instances.iteritems()))

        rr = calc_and_show(db, "All other", 'UNKNOWN', unused_instances, options, from_cmd)
        if not from_cmd:
            result['metagroups_stats']['All other'] = rr

        all_instances = used_instances
        used_instances = None
        all_instances.update(unused_instances)

        # some hack to eleminate coeffs
        for k in all_instances.iterkeys():
            all_instances[k].pop('coeff', None)

        rr = calc_and_show(db, "All", '', all_instances, options, from_cmd)
        if not from_cmd:
            result['metagroups_stats']['All'] = rr

        if options.dump_all_hosts:
            all_hosts = list(set(map(lambda x: x.host.name, all_instances)))
            print ",".join(all_hosts)

    if not from_cmd:
        return result


if __name__ == '__main__':
    options = parse_cmd()
    main(options, options.db)
