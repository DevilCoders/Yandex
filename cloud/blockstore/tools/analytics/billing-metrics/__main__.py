import argparse
import dateparser
import datetime
import json
import logging
import requests
import string
import sys
import time

BILLING_URL_TEMPLATE = string.Template(
    'https://${host}:16465/billing/v1/private/${path}?'
    'sourceName=${sourceName}&startTime=${startTime}&endTime=${endTime}')

CLUSTER_DESCRIPTION = {
    'prod': {
        'host': 'billing.private-api.cloud.yandex.net',
        'sourceName': 'topic-yc--billing-nbs-volume-0',
    },
    'preprod': {
        'host': 'billing.private-api.cloud-preprod.yandex.net',
        'sourceName': 'rt3.vla--yc-pre--billing-nbs-volume:0',
    },
}

TYPE_DESCRIPTION = {
    'unparsed': {
        'path': 'unparsedMetrics',
        'key': 'unparsedMetrics',
    },
    'expired': {
        'path': 'expiredMetrics',
        'key': 'expiredMetrics',
    },
    'discarded': {
        'path': 'discardedMetrics',
        'key': 'discardedMetrics',
    },
}


def deepget(dictionary, key):
    return reduce(
        lambda acc, x: acc.get(x) if isinstance(acc, dict) else None,
        key.split('.'),
        dictionary)


def parse_date(s):
    return dateparser.parse(s, settings={'DATE_ORDER': 'DMY'})


def datetime_to_timestamp(dt):
    return int(time.mktime(dt.utctimetuple()))


def get_billing_url(cluster, metrics_type, time_from, time_until):
    params = {
        'startTime': datetime_to_timestamp(time_from),
        'endTime': datetime_to_timestamp(time_until),
    }
    params.update(CLUSTER_DESCRIPTION[cluster])
    params.update(TYPE_DESCRIPTION[metrics_type])
    return BILLING_URL_TEMPLATE.substitute(params)


def fetch(url, token):
    headers = {
        'Content-Type': 'application/json',
        'X-YaCloud-SubjectToken': token,
    }
    return requests.get(url, headers=headers).json()


def prepare_args():
    parser = argparse.ArgumentParser(description='Extract billing metrics')
    parser.add_argument('-v', '--verbose', default=0, action='count')
    parser.add_argument('--cluster', choices=CLUSTER_DESCRIPTION.keys(), required=True)
    parser.add_argument('--type', nargs='+', choices=TYPE_DESCRIPTION.keys(), default=TYPE_DESCRIPTION.keys())
    parser.add_argument('--iam-token', required=True)
    parser.add_argument('--from', required=True, type=parse_date, dest='time_from')
    parser.add_argument('--until', type=parse_date, default=datetime.datetime.now(), dest='time_until')
    return parser.parse_args()


def prepare_logging(args):
    logging.basicConfig(level=max(0, logging.ERROR - 10 * args.verbose))


def get_metric(metric):
    if isinstance(metric, dict):
        return metric
    return json.loads(metric.replace("'", '"'))


def main():
    args = prepare_args()
    prepare_logging(args)
    for metrics_type in args.type:
        url = get_billing_url(args.cluster, metrics_type, args.time_from, args.time_until)
        j = fetch(url, args.iam_token)
        if 'totalCount' not in j:
            print >> sys.stderr, "bad response:", json.dumps(j, indent=4)
            return 1

        print '{}: (total {}{})'.format(
            metrics_type,
            j['totalCount'],
            ' and more' if j['isTruncated'] else '')
        for entry in j[TYPE_DESCRIPTION[metrics_type]['key']]:
            reason = deepget(entry, 'reason')
            metric = get_metric(entry['metric'])
            timestamp = metric['source_wt']
            host = metric['source_id']
            disk = metric['resource_id']
            disk_type = metric['tags']['type']
            disk_size = metric['tags']['size']
            cloud_id = metric['cloud_id']
            folder_id = metric['folder_id']
            print '{}: {}{} host={} disk={} ({} {}) cloud={} folder={}'.format(
                '{:%Y-%m-%d %H:%M:%S}'.format(datetime.datetime.fromtimestamp(timestamp)),
                metrics_type,
                ' ({})'.format(reason) if reason else '',
                host,
                disk,
                disk_type,
                '{:.1f} GiB'.format(float(disk_size) / 2**30),
                cloud_id,
                folder_id)

if __name__ == '__main__':
    sys.exit(main())
