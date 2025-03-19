import os
import random
import requests


DEFAULT_URL = 'https://c.yandex-team.ru/'


def group_name(dc, cluster, control, az):
    if control:
        group = f'cloud_{cluster}_nbs-control'
        if dc and dc != 'global':
            group += f'_{dc}'
    else:
        group = f'cloud_{cluster}_compute'
        if cluster in ['prod', 'preprod', 'testing']:
            group += f'_{dc}'
            if az:
                group += '_az'

    return group


class ConductorMock(object):

    def __init__(self, test_data_dir):
        self.__dir = test_data_dir

    def get_dc_hosts(self, dc, cluster, control, count, az):
        group = group_name(dc, cluster, control, az)
        with open(os.path.join(self.__dir, group + '.txt')) as f:
            hosts = [l.rstrip() for l in f.readlines()]

        return hosts if count == 0 else hosts[0:count]


class Conductor(object):

    def __init__(self, url=None, test_data_dir=None):
        self.__dir = test_data_dir
        self.__url = url if url else DEFAULT_URL

    def get_dc_hosts(self, dc, cluster, control, count, az):
        group = group_name(dc, cluster, control, az)
        url = f'{self.__url}api/groups2hosts/{group}'
        response = requests.get(url)
        response.raise_for_status()

        hosts = []
        all_hosts = []
        for host in response.iter_lines(decode_unicode=True):
            if len(hosts) < count or count == 0:
                hosts.append(host)
            all_hosts.append(host)

        if self.__dir is not None:
            with open(os.path.join(self.__dir, group + '.txt'), 'w') as f:
                for host in all_hosts:
                    f.write(host + '\n')

        return hosts


def get_dc_host(dc, cluster, control, count=1):
    return random.choice(Conductor(None).get_dc_hosts(dc, cluster, control, count, True))
