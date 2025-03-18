#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))
from argparse import ArgumentParser

import gencfg
from core.db import CURDB
from collections import defaultdict

import core.argparse.types as argparse_types
from gaux.aux_colortext import red_text, green_text

from tools.analyzer.runner import Runner
from tools.analyzer.functions import instance_documents


def _RG(n, fmt='%s'):
    if n > 0:
        return green_text(('+%s' % fmt) % n)
    else:
        return red_text(('%s' % fmt) % n)


QUIET = False


def parse_cmd():
    parser = ArgumentParser(description="Calculate extra instances")

    parser.add_argument("-c", "--config", dest="config", type=argparse_types.yamlconfig, required=True,
                        help="Obligatory. Path to config")
    parser.add_argument("-r", "--read-from-skynet", action="store_true", dest="read_from_skynet", default=False,
                        help="Optional. Reread document sizes from skynet")
    parser.add_argument("-q", "--quiet", action="store_true", dest="quiet", default=False,
                        help="Optional. Quiet output")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    global QUIET
    if options.quiet:
        QUIET = True

    return options


def printq(text):
    global QUIET
    if not QUIET:
        print(text)


def get_current_tiers_documents(options):
    tiers = map(lambda x: x['name'],
                filter(lambda x: not x.get('new', False) and not x.get('fake', False), options.config['tiers']))
    printq("Getting current document count for %s" % ','.join(tiers))

    flt = options.config.get('filter', None)
    if flt:
        flt = eval(flt)

    # find production instances for every tiers
    instances_tiers = {}
    tiers_counts = defaultdict(int)
    for tier in tiers:
        # find all intlookups with this tier and check if it is production
        for intlookup in CURDB.intlookups.get_intlookups():
            if intlookup.tiers is not None and tier in intlookup.tiers:
                if intlookup.base_type is not None:
                    group = CURDB.groups.get_group(intlookup.base_type)
                    if group.card.name in ['MSK_WEB_BASE_R1_BACKUP', 'MSK_WEB_BASE_PRIEMKA_BACKUP',
                                           'MSK_WEB_BASE_HAMSTER', 'MSK_WEB_SNIPPETS_SSD_BASE', 'ALL_TSNET_BASE',
                                           'MSK_WEB_SNIPPETS_SSD_NIDX',
                                           'SAS_WEB_SNIPPETS_SSD_BASE']:  # FIXME: get rid off this hack
                        continue

                brigades = sum(map(lambda x: x.brigades, intlookup.get_tier_brigade_groups(tier)), [])
                basesearchers = sum(map(lambda x: x.get_all_basesearchers(), brigades), [])
                if flt:
                    basesearchers = filter(lambda x: flt(x), basesearchers)
                for instance in basesearchers:
                    instances_tiers[instance] = tier
                tiers_counts[tier] += len(basesearchers)

    if options.read_from_skynet:
        # get number of documents in all instances
        printq(green_text("Starting runner"))
        instances = instances_tiers.keys()
        runner = Runner()
        failed, result = runner.run_on_instances(instances, [instance_documents], timeout=30)

        # calculate for all tiers
        printq(green_text("Start collecting"))
        tiers_alive_documents = dict(map(lambda x: (x, 0), tiers))
        tiers_alive_instances = dict(map(lambda x: (x, 0), tiers))
        for instance in result:
            if 'instance_documents' in result[instance] and result[instance]['instance_documents'] is not None:
                tiers_alive_documents[instances_tiers[instance]] += result[instance]['instance_documents']
                tiers_alive_instances[instances_tiers[instance]] += 1
        result = dict(map(lambda x: (x, (
        tiers_alive_documents[x] / float(tiers_alive_instances[x]) * CURDB.tiers.get_total_shards(x) / 1000000000, tiers_counts[x])), tiers))
        printq('\n'.join(map(lambda x: '    %s: %.2f mlrd docs, %s alive instances, %s total instances' %
                                       (x, result[x][0], tiers_alive_instances[x], tiers_counts[x]), tiers)))
    else:
        result = dict(map(lambda x: (x['name'], (x['cursize'], tiers_counts[x['name']])),
                          filter(lambda x: not x.get('new', False) and not x.get('fake', False),
                                 options.config['tiers'])))
        printq('\n'.join(
            map(lambda x: '    %s: %.2f mlrd docs, %s total instances' % (x, result[x][0], tiers_counts[x]), tiers)))

    return result


def main(options):
    new_tiers = filter(lambda x: x.get('new', False), options.config['tiers'])
    fake_tiers = filter(lambda x: x.get('fake', False), options.config['tiers'])
    changing_tiers = filter(lambda x: not x.get('new', False) and not x.get('fake', False), options.config['tiers'])

    # calculate new instances counts
    current_document_count = get_current_tiers_documents(options)
    printq("Extra documents in changing tiers:")
    for tier in changing_tiers:
        extra = tier['newsize'] - current_document_count[tier['name']][0]
        printq("    %s: %s mlrd (%s mlrd dps, %s replicas)" % (tier['name'], _RG(extra), _RG(
            current_document_count[tier['name']][0] / CURDB.tiers.get_total_shards(tier['name']), '%.5f'),
                                                               _RG(tier['replicas'])))
    printq("Documents in new tiers:")
    for tier in new_tiers:
        printq("    %s: %s mlrd (%s mlrd dps, %s replicas)" % (tier['name'], _RG(tier['newsize']), _RG(tier['docspershard']), _RG(tier['replicas'])))
    printq("Documents in fake tiers:")
    for tier in fake_tiers:
        printq("    %s: %s instances" % (tier['name'], _RG(tier['instances'])))

    new_tier_icount = {}
    for tier in changing_tiers:
        new_tier_icount[tier['name']] = tier['newsize'] / current_document_count[tier['name']][0] * tier[
            'sizefactor'] * CURDB.tiers.get_total_shards(tier['name']) * tier['replicas']
    for tier in new_tiers:
        new_tier_icount[tier['name']] = tier['newsize'] / tier['docspershard'] * tier['replicas']
    for tier in fake_tiers:
        new_tier_icount[tier['name']] = tier['instances']
    printq("New instances counts:")
    printq('\n'.join(
        map(lambda x: '    %s: %s instances' % (x['name'], new_tier_icount[x['name']]), options.config['tiers'])))

    # calculate extra instances
    printq("Instances in extra groups:")
    extra_groups = map(lambda x: CURDB.groups.get_group(x), options.config.get('extra_groups', '').split(','))
    for group in extra_groups:
        if len(filter(lambda x: x == group, extra_groups)) > 1:
            raise Exception("Group %s found at least twice" % group.card.name)
    extra_instances = 0
    for group in extra_groups:
        extra_group_instances = 0
        for host in group.getHosts():
            extra_group_instances += host.memory / 12  # FIXME use function from one of tiers
        extra_instances += extra_group_instances
        printq("    %s: %s" % (group.card.name, extra_group_instances))
    printq("    Total: %s" % extra_instances)

    # calculate needed instances
    needed_instances = int(sum(new_tier_icount.values()))
    have_instances = int(sum(map(lambda x: x[1], current_document_count.values())) + extra_instances)
    print("Total needed instances %s" % _RG(needed_instances))
    print("Already have instances %s" % _RG(have_instances))
    print("Needed instances %s, needed hosts %s (with %d instances)" % (_RG(needed_instances - have_instances), _RG(
        (needed_instances - have_instances) / options.config['instances_per_host']),
                                                                        options.config['instances_per_host']))


if __name__ == '__main__':
    options = parse_cmd()
    main(options)
