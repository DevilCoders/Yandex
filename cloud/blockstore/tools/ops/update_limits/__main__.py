import argparse
import logging
import requests
import sys
import urllib.parse

import cloud.blockstore.tools.cms.lib.pssh as libpssh


_SOLOMON_USER = 'robot-yc-nbs'
_SOLOMON_URL = 'http://solomon.yandex.net/data-api/get/'


def prepare_logging(args):
    log_level = logging.ERROR

    if args.silent:
        log_level = logging.INFO
    elif args.verbose:
        log_level = max(0, logging.ERROR - 10 * int(args.verbose))

    logging.basicConfig(
        stream=sys.stderr,
        level=log_level,
        format="[%(levelname)s] [%(asctime)s] %(threadName)s %(message)s")


def get_solomon_cluster_kikimr(args):
    if args.cluster in ('prod', 'preprod', 'testing'):
        return f'yandexcloud_{args.cluster}_{args.zone}'

    return f'yandexcloud_{args.cluster}'


def get_solomon_cluster_nbs(args):
    if args.cluster == 'prod':
        return f'yandexcloud_prod_{args.zone}'

    return f'yandexcloud_{args.cluster}'


def get_solomon_host(args):
    if args.cluster == 'prod':
        return 'cluster'

    return f'cluster-{args.zone}'


def get_used_kikimr(args):
    url = args.solomon_url + '?' + urllib.parse.urlencode({
        'who': _SOLOMON_USER,
        'project': 'kikimr',
        'cluster': get_solomon_cluster_kikimr(args),
        'service': 'pdisks',
        'l.host': 'cluster',
        'l.sensor': 'FreeSpaceBytes|UsedSpaceBytes',
        'l.media': 'ssd',
        'graph': 'auto',
        'l.pdisk': 'total',
        'b': '1h',
    })

    r = requests.get(url)
    r.raise_for_status()

    free_space_bytes = None
    used_space_bytes = None

    for s in r.json()['sensors']:
        second_last = s['values'][-2]['value']

        name = s['labels']['sensor']
        if name == 'FreeSpaceBytes':
            free_space_bytes = second_last
        else:
            used_space_bytes = second_last

    assert(free_space_bytes is not None)
    assert(used_space_bytes is not None)

    return (used_space_bytes, free_space_bytes)


def get_bytes_count(kind, args):
    url = args.solomon_url + '?' + urllib.parse.urlencode({
        'who': _SOLOMON_USER,
        'project': 'nbs',
        'cluster': get_solomon_cluster_nbs(args),
        'service' : 'service',
        'l.host': get_solomon_host(args),
        'l.sensor': 'BytesCount',
        'l.type': kind,
        'graph': 'auto',
        'b': '1h',
    })

    r = requests.get(url)
    r.raise_for_status()

    sensors = r.json()['sensors']
    assert(len(sensors) == 1)

    return sensors[0]['values'][-2]['value']


def wait_for_confirmation(yes, message):
    reaction = 'Y' if yes else None
    while reaction not in ['Y', 'n']:
        if reaction is not None:
            print('unsupported reaction, try again')
        reaction = input(f'{message}, continue? (Y/n) ')
    if reaction == 'n':
        print('aborting')
        sys.exit(1)


# https://stackoverflow.com/questions/1094841/get-human-readable-version-of-file-size
def sizeof_fmt(num, suffix='B'):
    for unit in ['', 'Ki', 'Mi', 'Gi', 'Ti', 'Pi', 'Ei', 'Zi']:
        if abs(num) < 1024.0:
            return "%3.1f%s%s" % (num, unit, suffix)
        num /= 1024.0
    return "%.1f%s%s" % (num, 'Yi', suffix)


class Pssh:

    def __init__(self, args):
        self.__impl = libpssh.Pssh(args.pssh_path)
        self.__compute_node = f'C@cloud_{args.cluster}_compute_{args.zone}_az[0]'

        self.__schemashard_dir = f'/{args.zone}/NBS'
        if args.cluster == 'preprod':
            self.__schemashard_dir = f'/pre-prod_{args.zone}/NBS'
        if args.cluster == 'testing':
            self.__schemashard_dir = f'/testing_{args.zone}/NBS'

    def __run(self, cmd):
        return self.__impl.run(
            'kikimr -s localhost:2135 db schema user-attribute ' + cmd,
            self.__compute_node)

    def get_current_ssd_limit(self):
        r = self.__run(f'get {self.__schemashard_dir}')
        logging.debug(r)

        if r is None:
            return None

        for s in r.splitlines():
            if s.startswith('__volume_space_limit_ssd:'):
                return int(s.split(':')[1])
        return None

    def update_ssd_limit(self, value):
        r = self.__run(f'set {self.__schemashard_dir} __volume_space_limit_ssd={value}')
        logging.debug(r)
        assert(r.strip() == '')


def calc_new_limit(ssd_bytes_count, hdd_bytes_count, used_space_bytes, free_space_bytes):
    used_kikimr = used_space_bytes / (used_space_bytes + free_space_bytes)
    return int((9 * ssd_bytes_count + hdd_bytes_count) / 8 * (0.85 / used_kikimr))


def main():
    parser = argparse.ArgumentParser()

    parser.add_argument(
        "--cluster",
        choices=['prod', 'preprod', 'testing'],
        type=str,
        required=True)

    parser.add_argument(
        "--zone",
        choices=['sas', 'vla', 'myt'],
        type=str,
        required=True)

    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="do not actually make changes")

    parser.add_argument(
        "--solomon-url",
        type=str,
        default=_SOLOMON_URL)

    parser.add_argument(
        "--pssh-path",
        type=str,
        default=libpssh.DEFAULT_PATH)

    parser.add_argument('-y', '--yes', help='skip all confirmations', action='store_true')

    parser.add_argument("-s", '--silent', help="silent mode", default=0, action='count')
    parser.add_argument("-v", '--verbose', help="verbose mode", default=0, action='count')

    args = parser.parse_args()

    prepare_logging(args)

    ssd_bytes_count = get_bytes_count('ssd', args)
    hdd_bytes_count = get_bytes_count('hdd', args)

    logging.debug(f'ssd_bytes_count: {ssd_bytes_count}')
    logging.debug(f'hdd_bytes_count: {hdd_bytes_count}')

    used_space_bytes, free_space_bytes = get_used_kikimr(args)

    logging.debug(f'used_space_bytes: {used_space_bytes}')
    logging.debug(f'free_space_bytes: {free_space_bytes}')

    new_limit = calc_new_limit(
        ssd_bytes_count,
        hdd_bytes_count,
        used_space_bytes,
        free_space_bytes
    )

    pssh = Pssh(args)

    current_limit = pssh.get_current_ssd_limit()
    assert(current_limit is not None)

    logging.debug(f'current_limit : {current_limit}')
    logging.debug(f'new_limit: {new_limit}')

    if args.dry_run:
        return 0

    wait_for_confirmation(
        args.yes,
        f'Current SSD limit {current_limit}B ({sizeof_fmt(current_limit)}) ' +
        f'will be replaced with {new_limit}B ({sizeof_fmt(new_limit)})')

    pssh.update_ssd_limit(new_limit)

    assert (pssh.get_current_ssd_limit() == new_limit)

    return 0


if __name__ == '__main__':
    sys.exit(main())
