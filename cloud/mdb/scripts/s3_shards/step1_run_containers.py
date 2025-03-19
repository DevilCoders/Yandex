#!/usr/bin/env python3

# https://wiki.yandex-team.ru/users/annkpx/naklikivanie-novyx-shardov-s3/#1.samikontejjnery

import requests

from time import sleep
from http.cookies import SimpleCookie

DC = {
    'i': 'man',
    'k': 'vla',
    'h': 'sas'
}

# Copy cookies from your browser on https://mdb.yandex-team.ru/
raw_cookies = ''
COOKIES = {k: v.value for k, v in SimpleCookie(raw_cookies).items()}

START = 33
STOP = 65

for n in range(START, STOP):
    for c, dc in DC.items():
        print('Try to create:', f's3db{n}{c} -> {dc}')
        url = f'https://mdb.yandex-team.ru/api/v2/containers/s3db{n}{c}.db.yandex.net'

        data = {
            "fqdn": f"s3db{n}{c}.db.yandex.net",
            "geo": dc,
            "bootstrap_cmd": f"/usr/local/yandex/porto/mdb_bionic.sh s3db{n}{c}.db.yandex.net",
            "volumes": [
                {
                    "path": "/",
                    "dom0_path": f"/data/s3db{n}{c}.db.yandex.net",
                    "backend": "native",
                    "space_limit": "2 TB"
                }
            ],
            "cpu_guarantee": "16",
            "cpu_limit": "16",
            "memory_guarantee": "64 GB",
            "memory_limit": "64 GB",
            "net_guarantee": "128 MB",
            "net_limit": "128 MB",
            "io_limit": "300 MB",
            "extra_properties": {
                "project_id": "0:1589"
            },
            "cluster": "s3db",
            "project": "pgaas",
            "secrets": {
                "/etc/yandex/mdb-deploy/deploy_version": {
                    "mode": "0644",
                    "content": "2"
                },
                "/etc/yandex/mdb-deploy/mdb_deploy_api_host": {
                    "mode": "0644",
                    "content": "deploy-api.db.yandex-team.ru"
                }
            }
        }

        resp = requests.put(
            url,
            json=data,
            cookies=COOKIES
        )

        print('Status:', resp.status_code)
        if resp.status_code != 200:
            print('Response:', resp.text)
            exit(1)

        print('Response:', resp.json())
        sleep(1)

    sleep(3)
