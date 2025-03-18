#!/skynet/python/bin/python
"""Script for comminicating with capaview optimization interface"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import getpass
import json
import urllib
import urllib2
import math
from collections import defaultdict
import time

import gencfg
from core.db import CURDB
import core.argparse.parser
from core.settings import SETTINGS
import core.argparse.types
import utils.common.manipulate_startrek as manipulate_startrek
import gaux.aux_utils as aux_utils
from gaux.aux_colortext import red_text


class ECapaviewOptimizationStatus(object):
    PENDING = 'PENDING'
    ACCEPTED = 'ACCEPTED'
    DELAYED = 'DELAYED'
    REJECTED = 'REJECTED'


class EActions(object):
    UPLOAD = "upload"  # upload new optimization wave
    DOWNLOAD = "download"  # download optimization wave current state
    ALL = [UPLOAD, DOWNLOAD]


def get_parser():
    parser = core.argparse.parser.ArgumentParserExt(description='Manipulate capi optimization interface in various ways')
    parser.add_argument('-a', '--action', type=str, required=True,
                        choices=EActions.ALL,
                        help='Obligatory. Action to execute')
    parser.add_argument('-w', '--optimization-wave', type=str, default=None,
                        help='Optional. Optimization wave name (obligatory for <{}> actions)'.format(EActions.UPLOAD))
    parser.add_argument('-d', '--optimization-wave-descr', type=str, default='Sample description',
                        help='Optional. Optimization wave description')
    parser.add_argument('-t', '--st-ticket', type=str, default=None,
                        help='Optional. Startrek ticket to associate with optimization wave')
    parser.add_argument('-s', '--suggest-list', type=core.argparse.types.cpu_suggest_list, default = [],
                        help='Optional. List of suggests (obligatory for <{}> actions)'.format(EActions.UPLOAD))
    parser.add_argument('--extra-reject', type=core.argparse.types.groups, default = None,
                        help='Optional. List of extra groups to reject for <{}> action'.format(EActions.DOWNLOAD))
    parser.add_argument('--min-gain', type=int, default=None,
                        help='Optional. Skip groups with gain less than specified')
    parser.add_argument('--ignore-reject', action='store_true', default=False,
                        help='Optional. Treat REJECTED/DELAYD status as ACCEPTED')
    parser.add_argument('--apply', action='store_true', default=False,
                        help='Optional. Apply achanges (for action <{}>)'.format(EActions.DOWNLOAD))
    parser.add_argument("-v", "--verbose", action="count", default=0,
                        help="Optional. Increase output verbosity (maximum 2)")

    return parser


def normalize(options):
    if options.action == EActions.UPLOAD:
        if options.optimization_wave is None:
            raise Exception('Missing <--optimization-wave> with action <{}>'.format(options.action))
        if not len(options.suggest_list):
            raise Exception('Missing <--suggest-list> with action <{}>'.format(options.action))
    if options.action == EActions.DOWNLOAD:
        if options.optimization_wave is None:
            raise Exception('Missing <--optimization-wave> with action <{}>'.format(options.action))
        if options.ignore_reject and options.apply:
            raise Exception('Options <--ignore-reject> and <--apply> are mutually exclusive')

    if options.extra_reject is None:
        options.extra_reject = []
    options.extra_reject = [x.card.name for x in options.extra_reject]

def has_optimization_wave(wave_name):
    """Check if optimization wave is already in database"""
    data = aux_utils.retry_urlopen(2, SETTINGS.services.capaview.optimization.rest.url)
    data = json.loads(data)
    if len([x for x in data if x['name'] == wave_name]) > 0:
        return True
    return False


def main_upload(options):
    """Upload new optimization wave"""

    # create new startrek ticket if not found
    if options.st_ticket is None:
        util_params = dict(
            action=manipulate_startrek.EActions.CREATE,
            queue='RX',
            summary='Optimization wave <{}>'.format(options.optimization_wave),
            description='All objections on optimization wave <{}> are accepted there'.format(options.optimization_wave),
            assignee=getpass.getuser(),
            followers=SETTINGS.services.capaview.optimization.config.followers,
        )
        util_result = manipulate_startrek.jsmain(util_params)
        options.st_ticket = util_result['key']
        print 'Created task <{}{}>'.format(SETTINGS.services.startrek.http.url, options.st_ticket)

    # create new wave
    if has_optimization_wave(options.optimization_wave):
        print 'Wave <{}> already in database'.format(options.optimization_wave)
    else:
        url = SETTINGS.services.capaview.optimization.rest.url
        data=json.dumps(dict(
            name=options.optimization_wave,
            st_ticket_id=options.st_ticket,
            description=options.optimization_wave_descr,
        ))
        headers = {
            'Content-Type': 'application/json'
        }
        aux_utils.retry_urlopen(2, urllib2.Request(url, data, headers))
        print 'Created wave <{}>'.format(options.optimization_wave)

    # upload suggest list
    url = '{}/{}/change_requests'.format(SETTINGS.services.capaview.optimization.rest.url, options.optimization_wave)
    suggest_cards = [dict(group=x.group.card.name, original_group_guarantee=x.group.get_instances()[0].power,
                          suggested_group_guarantee=x.power, optimization_wave=options.optimization_wave
                         ) for x in options.suggest_list
                    ]
    data = json.dumps(suggest_cards)
    headers = {
        'Content-Type': 'application/json'
    }
    aux_utils.retry_urlopen(2, urllib2.Request(url, data, headers))
    print 'Uploaded {} groups for update:'.format(len(options.suggest_list))
    if options.verbose > 0:
        for suggest in options.suggest_list:
            print '   Suggest for {}: {:.2f} -> {:.2f}'.format(suggest.group.card.name, suggest.group.get_instances()[0].power, suggest.power)


def apply_for_group(group, new_guarantee):
    """Set new guarantee for group"""
    instances = group.get_kinda_busy_instances()
    for intlookup in (CURDB.intlookups.get_intlookup(x) for x in group.card.intlookups):
        intlookup.mark_as_modified()

    group.card.legacy.funcs.instancePower = 'exactly{}'.format(new_guarantee)
    group.card.properties.cpu_guarantee_set = True
    group.card.audit.cpu.last_modified = time.strftime('%Y-%m-%d %H:%M:%S', time.localtime())

    for instance in instances:
        instance.power = new_guarantee

    group.mark_as_modified()


def main_download(options):
    # get current suggest list state
    url = '{}/{}/change_requests'.format(SETTINGS.services.capaview.optimization.rest.url, urllib.quote(options.optimization_wave))
    jsoned = json.loads(aux_utils.retry_urlopen(3, url))['data']

    # check for unexisting groups
    unexisting_groups = [x['group'] for x in jsoned if not CURDB.groups.has_group(x['group'])]
    if unexisting_groups:
        print red_text('The following groups do not exist anymore: {}'.format(' '.join(unexisting_groups)))
    jsoned = [x for x in jsoned if CURDB.groups.has_group(x['group'])]

    # get stats for reposnd types
    respond_types_count = defaultdict(int)
    for elem in jsoned:
        respond_types_count[elem['status']] += 1
    print 'Respond statuses:'
    for respond_type in sorted(respond_types_count.keys()):
        print '    Status <{}>: {} groups'.format(respond_type, respond_types_count[respond_type])
    if not options.ignore_reject:
        jsoned = [x for x in jsoned if x['status'] in (ECapaviewOptimizationStatus.PENDING, ECapaviewOptimizationStatus.ACCEPTED)]
        jsoned = [x for x in jsoned if x['group'] not in options.extra_reject]

    # get gain data
    gain_data = []
    for elem in jsoned:
        group = CURDB.groups.get_group(elem['group'])
        suggested_guarantee = int(math.ceil(elem['suggested_group_guarantee']))
        orig_guarantee = int(math.ceil(elem['original_group_guarantee']))
        gain = sum(x.power for x in group.get_kinda_busy_instances()) - len(group.get_kinda_busy_instances()) * suggested_guarantee
        gain = int(gain)
        gain_data.append((group, orig_guarantee, suggested_guarantee, gain))
    gain_data.sort(key = lambda (x, y, z, t): -t)

    # filter out groups with recently changed cpu guarantee
    def is_recently_modified(group):
        if group.card.audit.cpu.last_modified is None:
            return False
        t = time.mktime(time.strptime(group.card.audit.cpu.last_modified, '%Y-%m-%d %H:%M:%S'))
        if t > time.time() - 60 * 24 * 60 * 60:  # two month
            return True
        return False

    gain_data = [x for x in gain_data if not is_recently_modified(x[0])]

    # filter out groups with small gain
    if options.min_gain is not None:
        gain_data = [x for x in gain_data if x[-1] > options.min_gain]

    # report result
    total_gain = sum(t for (x, y, z, t) in gain_data)
    print 'Suggest for groups ({} potential gain):'.format(total_gain)
    for group, orig_guarantee, suggested_guarantee, gain in gain_data:
        print '    Group {} (owners {}): change {} -> {} , gained {}'.format(group.card.name, ','.join(group.card.owners), orig_guarantee, suggested_guarantee, gain)

    if options.apply:
        print 'Applying for {} groups:'.format(len(gain_data))
        for group, orig_guarantee, suggested_guarantee, gain in gain_data:
            apply_for_group(group, suggested_guarantee)

        CURDB.update(smart=True)

        # generate commit message
        all_groups = [x[0] for x in gain_data]
        all_owners = [x.card.owners for x in all_groups]
        all_owners = sorted(set(sum(all_owners, [])))
        all_owners = ['{}@'.format(x) for x in all_owners]
        msg = 'Apply automatically generated guarantee for groups: {} ({}).'.format(' '.join(x.card.name for x in all_groups), ' '.join(x for x in all_owners))
        print 'Commit msg: {}'.format(msg)


def main(options):
    if options.action == EActions.UPLOAD:
        main_upload(options)
    elif options.action == EActions.DOWNLOAD:
        main_download(options)
    else:
        raise Exception('Unknown action <{}>'.format(options.action))


if __name__ == '__main__':
    options = get_parser().parse_cmd()

    normalize(options)

    main(options)
