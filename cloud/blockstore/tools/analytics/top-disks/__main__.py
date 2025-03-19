import argparse
import logging
import requests
import datetime
import timeit

from concurrent.futures import ThreadPoolExecutor, as_completed
from collections import defaultdict, namedtuple
from time import sleep
from json import dumps

request_info = namedtuple('request_info', 'request description dashboard')

logger = logging.getLogger('blockstore-top-disks')
logger.setLevel(logging.DEBUG)

formatter = logging.Formatter(fmt="%(asctime)s - %(name)s - %(levelname)s - %(message)s", datefmt='%m/%d/%Y %I:%M:%S')
ch = logging.StreamHandler()
ch.setLevel(logging.DEBUG)
ch.setFormatter(formatter)
logger.addHandler(ch)

TOP_REQUESTS = [
    request_info('ReadBlocksSmall', 'Small(<64KB) ReadBlocks requests', 'nbs-volume-read-performance'),
    request_info('ReadBlocksLarge', 'Large(>=64KB) ReadBlocks requests', 'nbs-volume-read-performance'),
    request_info('WriteBlocksSmall', 'Small(<64KB) WriteBlocks requests', 'nbs-volume-write-performance'),
    request_info('WriteBlocksLarge', 'Large(>=64KB) WriteBlocks requests', 'nbs-volume-write-performance'),
    request_info('ZeroBlocksSmall', 'Small(<64KB) ZeroBlocks requests', 'nbs-volume-zero-performance'),
    request_info('ZeroBlocksLarge', 'Large(>=64KB) ZeroBlocks requests', 'nbs-volume-zero-performance')]

HOST_TEMPLATE = 'http://{}:8766/counters/counters=blockstore/component=service_volume/json'
SOLOMON_PATH = 'https://solomon.yandex-team.ru/?'
SOLOMON_PARAMS = 'project=nbs&cluster={}&service=service_volume&host=cluster&volume={}&dashboard={}'


def query_conductor(group, auth):
    response = None
    try:
        response = requests.get(
            'https://c.yandex-team.ru/api/groups2hosts/' + group,
            headers={'Authorization': auth})
    except Exception as e:
        logger.error("Got exception resolving group {} : ".format(group) + str(e))
        return
    logger.debug("Hosts found: {}".format(response.text))
    return [HOST_TEMPLATE.format(host) for host in response.iter_lines()]


def get_hosts(args):
    if args.host is not None:
        return [HOST_TEMPLATE.format(args.host)]
    if args.hosts_file is not None:
        return [HOST_TEMPLATE.format(host) for host in open(args.hosts_file, 'r')]
    if args.group is not None:
        return query_conductor(args.group, args.auth)


def find_n_top(disks_metric, n):
    def partition(pairs, l, r):
        pivot, i = pairs[r][1], l
        for j in range(l, r):
            if pairs[j][1] >= pivot:
                pairs[i], pairs[j] = pairs[j], pairs[i]
                i = i + 1
        pairs[i], pairs[r] = pairs[r], pairs[i]
        return i

    def find_top(disks_metric, l, r, n):
        if n <= r - l + 1:
            index = partition(disks_metric, l, r)

            if index - l == n - 1:
                return disks_metric[:n + 1]

            if index - l > n - 1:
                return find_top(disks_metric, l, index - 1, n)

            return find_top(disks_metric, index + 1, r, n - index + l - 1)
        else:
            return disks_metric

    return find_top(disks_metric, 0, len(disks_metric) - 1, n)


def query_metrics(hosts, timeout):
    pool = ThreadPoolExecutor(len(hosts))
    futures = [pool.submit(requests.get, url) for url in hosts]
    metrics = []
    for f in as_completed(futures, timeout):
        try:
            metrics.append(f.result())
        except Exception as e:
            logger.error("Got exception in query_metrics: " + str(e))
    return metrics


def collect_metrics(metrics, requests, percentile):
    metrics_set = defaultdict(list)
    sensors = metrics.json()['sensors']
    for sensor in sensors:
        if 'volume' not in sensor['labels']:
            continue
        if ('request' in sensor['labels']) and sensor['labels']['request'] in requests:
            request_metrics = metrics_set[sensor['labels']['request']]
            if ('percentiles' in sensor['labels']) and (sensor['labels']['sensor'] == percentile):
                request_metrics.append((sensor['labels']['volume'], sensor['value']))
    return metrics_set


def build_wiki_table(request, captions, data, history, args, dashboard):
    body = '===' + request + '\n'
    body += '#|\n'
    body += '**||' + '|'.join(c for c in captions) + '||**\n'
    for disk, value in data:
        body += '||'
        body += '((' + SOLOMON_PATH + SOLOMON_PARAMS.format(args.cluster, disk, dashboard) + ' ' + disk + '))'
        body += '|' + str(value)
        body += '|' + str(datetime.timedelta(seconds=history[disk])) + '||'
    body += '|#\n'
    return body


def update_history(request_history, top_disks, update_interval):
    result = {}
    for d in top_disks:
        result[d] = update_interval
        if d in request_history:
            result[d] += request_history[d]
    return result


def run():
    parser = argparse.ArgumentParser()

    parser.add_argument('--group', help='conductor group name')
    parser.add_argument('--host', help='host to query stats')
    parser.add_argument('--hosts_file', help='name of file with list of cluster hosts')
    parser.add_argument('--top', help='report top N disks', default=10, type=int)
    parser.add_argument('--percentile', help='percentile', default='99')
    parser.add_argument('--period', help='update top every N seconds', default=15, type=int)
    parser.add_argument('--auth', help='authorization token (format: "OAuth <token>")')
    parser.add_argument('--wiki_root', help='wiki path to store results')
    parser.add_argument('--title', help='title of wiki report page')
    parser.add_argument('--cluster', help='cluster name in solomon')

    args = parser.parse_args()

    hosts = get_hosts(args)

    requests_history = defaultdict(dict)

    if hosts is not None:
        while True:

            start = timeit.timeit()
            cluster_metrics = query_metrics(hosts, max([args.period - 5, 10]))
            end = timeit.timeit()

            if len(cluster_metrics):
                cluster_top_disks = defaultdict(list)

                for host_metrics in cluster_metrics:
                    metrics = collect_metrics(host_metrics, [r.request for r in TOP_REQUESTS], args.percentile)
                    for request_name, request_metrics in metrics.items():
                        cluster_top_disks[request_name] += find_n_top(request_metrics, args.top)

                body = ''

                for request, description, dashboard in TOP_REQUESTS:
                    logger.debug("{}:\n".format(request))

                    top_disks = find_n_top(cluster_top_disks[request], args.top)
                    top_disks = sorted(top_disks, key=lambda x: x[1], reverse=True)

                    if len(top_disks) != 0:
                        requests_history[request] = update_history(
                            requests_history[request],
                            (disk[0] for disk in top_disks),
                            args.period)

                        body += build_wiki_table(
                            description,
                            ['Disk name', 'Latency {} percentile (us)'.format(args.percentile), 'Time in top'],
                            top_disks,
                            requests_history[request],
                            args,
                            dashboard)

                    for disk in top_disks:
                        logger.debug(("{} - {}\n".format(disk[0], disk[1])))

                try:
                    logger.debug("{}".format(body))
                    response = requests.post(
                        args.wiki_root,
                        headers={'Authorization': args.auth, 'Content-type': 'application/json'},
                        data=dumps({'title': args.title, 'body': body}))
                    logger.debug("{}".format(response.content))
                except Exception as e:
                    logger.error("Got exception while posting to wiki: " + str(e))

            sleep(max((args.period - (end - start), 0)))
    else:
        logger.error("No hosts provided")


run()
