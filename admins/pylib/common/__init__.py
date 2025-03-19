import sys
import os
sys.path.append(os.path.join(os.path.dirname(__file__), '..', '..', '..'))

from sandbox.projects.common.nanny.client import NannyClient
import yaml
import re


def nanny_client():
    client = NannyClient('https://nanny.yandex-team.ru', yaml.load(open(os.path.expanduser('~/.runtime.yaml')))['token'])
    return client


def sox_services():
    return [
        'music_stable_back_myt',
        'music_stable_back_sas',
        'music_stable_back_man',
        'music_stable_billing_man',
        'music_stable_billing_sas',
        'music_stable_billing_vla',
        'music_stable_connector_man',
        'music_stable_connector_sas',
        'music_stable_connector_vla',
        'music_stable_export_man',
        'music_stable_export_sas',
        'music_stable_export_vla',
        'music_stable_heavy_worker_man',
        'music_stable_heavy_worker_sas',
        'music_stable_heavy_worker_vla',
        'music_stable_lite_worker_man',
        'music_stable_lite_worker_sas',
        'music_stable_lite_worker_vla',
    ]


def music_services(nanny):
    banned_list = [re.compile(x) for x in (
        '^/music/back/dev/',
        '^/music/dev/',
        '^/music/lyrics/',
        '^/music/match/',
        '^/music/yamrec/',
        'android',
        'musfront',
        'balancer_pinger',
    )]
    result = []
    for service in nanny.list_services_by_category('/music')['result']:
        category = service['info_attrs']['content']['category']
        banned = False
        for ban in banned_list:
            if ban.search(category):
                banned = True
                break

        if not banned:
            result.append(service['_id'])
    return result


