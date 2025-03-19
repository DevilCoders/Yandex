#!/usr/bin/env pymds

from __future__ import division, unicode_literals, print_function

import argparse
import logging
import time
from collections import defaultdict
from fnmatch import fnmatch

import arrow
import socket
import yaml
import json
from mds.admin.library.python.sa_scripts import mm

import requests

logger = logging.getLogger(__name__)
HOSTNAME = socket.gethostname()


@mm.mm_retry(tries=10, delay=1, backoff=0.6)
def get_nses(mm_client, mask):
    logger.debug('Looking for namespaces "{}"'.format(mask))
    if isinstance(mask, list):
        return mask
    return (ns for ns in mm_client.namespaces if not ns.deleted and fnmatch(ns.id, mask))


class EmptyStats(Exception):
    pass


def calc_used(point):
    return sum(
        [
            point['effective_used_space'],
            point['records_uncommitted_size'],
            point['records_removed_size'],
        ]
    )


@mm.mm_retry(tries=10, delay=1, backoff=0.6)
def _mm_stats(mm_client, ns_id, limit, offset=0):
    return list(
        mm_client.request(
            'get_monitor_space_distribution',
            [ns_id, {'limit': limit, 'offset': offset}],
            timeout=20,
        )['space_distribution']
    )


@mm.mm_retry(tries=10, delay=1, backoff=0.6)
def _flow_stats(mm_client):
    return mm_client.request('get_flow_stats', None)


def get_ns_stat(mm_client, ns, time_from=3600 * 24 * 7):
    mm_point_interval = 280  # assume points every 5 minutes
    limit = int(time_from // mm_point_interval + 1)

    stats = _mm_stats(mm_client, ns.id, limit, 0)
    if len(stats) < 10:  # skip new namespaces
        raise EmptyStats(ns.id)

    # assume stats sorted by ts in descending order
    last = stats[0]

    # https://st.yandex-team.ru/MDS-12741#629e0437da8e5462a4d55b07
    dc_weight_coefficients = ns.settings.dict().get('dc_weight_coefficients', {})
    if dc_weight_coefficients:
        flow_stats = _flow_stats(mm_client)
        current_free = last['free_effective_space']
        for dc, weight in dc_weight_coefficients.iteritems():
            if weight == 0:
                free = flow_stats.get('namespaces', {}).get(ns.id, {}).get(dc, {}).get('effective_free_space', 0)
                current_free -= free
    else:
        current_free = last['free_effective_space']

    current_used = calc_used(last)

    space_limit = 0
    gib_value = 1024 ** 3
    for limit_gib in ns.settings.dict().get('space-limits-GiB', {}).values():
        space_limit += limit_gib * gib_value

    # Temporal correction, should be removed, when MDS-11280 will be fully deployed to stable.
    current_free = min(space_limit - current_used, current_free)
    # Note: we can build over space limit, in this case actual total is greater than space limit,
    # but we want to think in our monitoring that current total is the space limit
    current_total = min(
        space_limit, current_used + current_free + last.get('reserved_effective_free_space', 0)
    )

    points = []
    for p in stats:
        if p['ts'] < time.time() - time_from - mm_point_interval:  # older then we need
            break
        if 'effective_used_space' not in p:  # old stats.
            break
        # last point, last request returned all points we asked, so there are could be more
        # and poin still not older then asked. try to get them
        if p is stats[-1] and len(stats) == limit:
            # assume new points has the same interval as last point we got
            mm_point_interval = (stats[0]['ts'] - stats[-1]['ts']) // len(stats)
            assert mm_point_interval > 0, 'Wrong points order for {}'.format(ns.id)
            offset, limit = limit, int(time_from // mm_point_interval - limit) + 1
            if limit > 0:
                logger.debug('{}: not enough points. going to get {} more'.format(ns.id, limit))
                stats.extend(_mm_stats(mm_client, ns.id, limit, offset))
            else:
                logger.warn(
                    'Got {} points with interval {}, but last point ts is {} ago'.format(
                        len(stats), mm_point_interval, hf_time(time.time() - stats[-1]['ts'])
                    )
                )
                break
        points.append([p['ts'], calc_used(p)])

    points = list(sorted(points))

    if points[0][0] > time.time() - time_from + mm_point_interval:
        logger.warning(
            '{}: Oldest point {} ago instead of {}'.format(
                ns.id, hf_time(time.time() - points[0][0]), hf_time(time_from),
            )
        )

    return current_free, current_total, points, space_limit


def derivative(points):
    assert len(points) > 2, "Must be more then 2 points for derivative"
    avg_interval = (points[-1][0] - points[0][0]) / len(points)
    prev_ts, prev_value = points[0]
    for ts, value in points[1:]:
        yield (value - prev_value) / avg_interval
        prev_ts, prev_value = ts, value


def weighted_average(values, q=1, weights=[1]):
    """
    :param values: numbers for averaging
    :param q: next value weight = previous value weight * q
    :param weights: values will be grouped to equal parts with given weights
      Example:
        values = [1,1,1,2], weights [1,2] and q=1
        [1,1] average to 1 and will have weight 1
        [1,2] average to 1.5 and will have weight 2
        result = (1 + 1.5*2)/3 = 1.3333
        in case of week [1,1,1,1,1,1,6] means last day have the same weight as whole week before
    :return: float
    """
    parts_avgs = []
    part_len = len(values) / len(weights)
    for i, w in enumerate(weights):
        start, end = int(part_len * i), int(part_len * (i + 1))
        part = values[start:end]
        part_weights = [q ** i for i in xrange(len(part))]
        parts_avgs.append(w * sum(v * w for v, w in zip(part, part_weights)) / sum(part_weights))
    return sum(parts_avgs) / sum(weights)


def hf_time(secs):
    if secs > 86400 * 365 * 10:
        return 'never'
    for n, v in [
        ('y', secs / 86400 / 365),
        ('d', secs / 86400),
        ('h', secs / 3600),
        ('m', secs / 60),
        ('s', secs),
    ]:
        if v > 1:
            break
    return '{0:.02f}{1}'.format(v, n)


def hf_to_secs(s):
    if not isinstance(s, basestring):
        return s

    if s.isdigit():
        return float(s)

    s = s.lstrip('-')
    mult = {
        'y': 365 * 24 * 3600,
        'w': 7 * 3600 * 24,
        'd': 3600 * 24,
        'day': 3600 * 24,
        'h': 3600,
        'm': 60,
        'min': 60,
        's': 1,
        'sec': 1,
    }.get(s[-1])

    return float(s[:-1]) * mult


def hf_size(value):
    for suf in ['B', 'KB', 'MB', 'GB', 'TB', 'PB', 'EB']:
        if abs(value) < 512:
            break
        value /= 2 ** 10

    if abs(value) < 0.00001:
        value = 0

    return '{0:.3g}{1}'.format(value, suf)


def hf_to_bytes(s):
    if not isinstance(s, basestring):
        return s

    if s.isdigit():
        return float(s)

    power = ['B', 'K', 'M', 'G', 'T', 'P', 'E'].index(s[-1]) * 10
    return float(s[:-1]) * 2 ** power


def check(value, warn, crit):
    if value < crit:
        status = 2
    elif value < warn:
        status = 1
    else:
        status = 0
    return status


def check_quote_diff(value, warn, crit):
    if value < 0 or value > crit:
        status = 2
    elif value > warn:
        status = 1
    else:
        status = 0
    return status


def must_ignore(ignore):
    if isinstance(ignore, bool):
        return ignore
    return arrow.get(ignore).timestamp > time.time()


def check_ns(mm_client, ns, ns_conf, mm_s3_ids, stats_url):
    try:
        free, total, points, space_limit = get_ns_stat(
            mm_client, ns, hf_to_secs(ns_conf['time_from'])
        )
    except EmptyStats:
        status, msg = 1, '{}: empty stats'.format(ns.id)
    else:
        ttl = check_ttl(ns)
        if mm_s3_ids.get(ns.id):
            diff_s3_mds_quote, s3_quota = check_s3_quota(stats_url, space_limit, *mm_s3_ids[ns.id])
            diff_s3_mds_quote_msg = ', {:.2%} quota diff between mds({}) and s3({})'.format(
                diff_s3_mds_quote, hf_size(space_limit), hf_size(s3_quota)
            )
            logger.info("{} quota mds({}) and s3({})".format(ns.id, space_limit, s3_quota))
        else:
            diff_s3_mds_quote = 0
            diff_s3_mds_quote_msg = ''

        if free == total == 0:
            return 0, "Namespace not found in this federation"
        free_p = 100 * free / total
        total_p = 100 * total / space_limit
        speeds = list(derivative(points))
        usage_increase_bps = weighted_average(speeds, ns_conf['weights_q'], ns_conf['weights'])
        time_to_end = free / usage_increase_bps if usage_increase_bps > 0 else float('inf')
        status = max(
            [
                check(free_p, ns_conf['percent_warn'], ns_conf['percent_crit']),
                check(free, hf_to_bytes(ns_conf['bytes_warn']), hf_to_bytes(ns_conf['bytes_crit'])),
                check(
                    time_to_end, hf_to_secs(ns_conf['time_warn']), hf_to_secs(ns_conf['time_crit'])
                ),
                check_quote_diff(
                    diff_s3_mds_quote, ns_conf.get('diff_warn', 5), ns_conf.get('diff_crit', 10)
                ),
            ]
        )
        msg = '{:.2f}% free, {}({}{}ps), quota {}, alloc {} ({:.2f}%), {}{}{}'.format(
            free_p,
            hf_size(free),
            '' if usage_increase_bps > 0 else '+',
            hf_size(-usage_increase_bps),
            hf_size(space_limit),
            hf_size(total),
            total_p,
            hf_time(time_to_end),
            ttl,
            diff_s3_mds_quote_msg,
        )

    return status, msg


STATUSES = {
    2: 'CRIT',
    1: 'WARN',
    0: 'OK',
}


def check_ttl(ns):
    ttl = ''
    attributes = ns.settings.dict().get('attributes', {}).get('ttl', {})
    if attributes.get('enable'):
        ttl = ', ttl'
    return ttl


def check_s3_quota(stats_url, mds_limit, *s3_ids):
    s3_sum = 0
    for s3_id in s3_ids:
        request = "{}/{}".format(stats_url, s3_id)
        for x in range(5):
            response = requests.get(request)
            if response.status_code == 200:
                break
            else:
                logger.info("Failed to get request {}".format(request))

        s3_sum += 0 if not response.json()['max_size'] else int(response.json()['max_size'])
    try:
        return mds_limit / s3_sum - 1, s3_sum
    except ZeroDivisionError:
        return -1, 0


def main(config):
    results = defaultdict(list)
    mm_s3_ids = defaultdict(list)
    mm_client = mm.mastermind_service_client(mm_hosts=[HOSTNAME], timeout=60)
    namespaces = get_nses(mm_client, config['ns_filter'])

    with open("/etc/elliptics/mastermind.conf", "r") as mm_config:
        data = json.load(mm_config).get('stat-reporter', {})
        s3_ids_conf = data.get('s3_id_to_mds_ns', {})
        url = data.get('s3_stats_url', {})

    for key, value in s3_ids_conf.items():
        # Skip default namespaces
        if value.startswith('s3-default'):
            continue

        mm_s3_ids[value].append(str(key))

    for ns in namespaces:
        ns_conf = get_ns_config(ns.id, config)
        try:
            status, msg = check_ns(mm_client, ns, ns_conf, mm_s3_ids, url)
        except Exception as e:
            status, msg = 2, str(e)
            logger.exception('{} check failed'.format(ns.id))
        if must_ignore(ns_conf['ignore']):
            status = 0
        # Ignore namespaces with statis-couple
        if ns.settings.get('static-couple', None):
            status = 0
        results[status].append((ns.id, msg))
        logger.debug('{}: {} {}'.format(ns.id, STATUSES[status], msg))

    if config['format'] == 'monrun':
        status = max(results.keys())
        status = min(config['max_status'], status)
        messages = results.get(2, []) + results.get(1, []) or ('all', 'OK')
        print(
            '{0};{1}'.format(
                status, ' | '.join('{}:{}'.format(ns_id, msg) for ns_id, msg in messages)
            )
        )
    elif config['format'] in ['juggler', 'juggler-batch']:
        events = [
            {
                "host": "mds-namespace-{}".format(ns_id.replace('_', '-')),
                "service": config['service'],
                "status": STATUSES[status],
                "description": msg,
            }
            for status in results
            for ns_id, msg in results[status]
        ]
        if config['format'] == 'juggler':
            events = {'events': events}
        print(json.dumps(events))
    else:
        print('Unknown format')
        print(results)


_DEFAULT_CONFIG = {
    'log_level': 'INFO',
    'log_file': '/var/log/mastermind-namespace-space.log',
    'ns_filter': '*',
    'format': 'monrun',
    'service': 'mm-namespace-space',
    'max_status': 2,
    'defaults': {
        'time_from': '7d',
        'time_warn': '7d',
        'time_crit': '2d',
        'percent_warn': 25,
        'percent_crit': 2,
        'bytes_warn': '100G',
        'bytes_crit': '10G',
        'ignore': False,
        'weights': [1, 1, 1, 1, 1, 1, 6],
        'weights_q': 1,
    },
    'namespaces': {
        '_example': {'time_warn': '3d', 'time_crit': '1d', 'ignore': '2020-05-07T05:00:00+03:00',},
    },
}


def merge_dicts(*args):
    def _merge(a, b):
        res = {}
        for k in set(b.keys() + a.keys()):
            if k not in b:
                res[k] = a[k]
            elif k not in a:
                res[k] = b[k]
            elif isinstance(a[k], dict) and isinstance(b[k], dict):
                res[k] = merge_dicts(a[k], b[k])
            else:
                res[k] = b[k]
        return res

    if not args:
        return {}
    if len(args) == 1:
        return args[0]
    res = {}
    for d in args:
        res = _merge(res, d)
    return res


def get_config(args):
    if args.config:
        with open(args.config) as f:
            file_config = yaml.load(f.read())
    else:
        file_config = {}
    return merge_dicts(
        _DEFAULT_CONFIG, file_config, {k: v for k, v in vars(args).items() if v is not None}
    )


def get_ns_config(ns_id, config):
    return merge_dicts(config['defaults'], config['namespaces'].get(ns_id, {}))


def get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('-n', '--ns-filter', nargs='*', type=str)
    parser.add_argument('-c', '--config', type=str)
    parser.add_argument('--log-level', type=str)
    parser.add_argument('--log-file', type=str)
    parser.add_argument('--max-status', type=int)
    parser.add_argument(
        '-f', '--format', help='Output format', choices=['monrun', 'juggler', 'juggler-batch']
    )
    parser.add_argument('--service', help='Service name for juggler output')
    return parser.parse_args()


if __name__ == '__main__':
    args = get_args()
    if args.ns_filter and len(args.ns_filter) == 1:
        args.ns_filter = args.ns_filter[0]
    config = get_config(args)

    log_cfg = {
        'format': '[%(asctime)s][%(levelname)s]: %(message)s',
        'level': config['log_level'].upper(),
    }
    if config['log_file'] not in [None, '-']:
        log_cfg['filename'] = config['log_file']
    logging.basicConfig(**log_cfg)

    try:
        main(config)
    except Exception as e:
        logger.exception('main failed')
        print('{};{}'.format(1, e))
