import os
import re
import time
from pprint import pprint
from collections import defaultdict

import click

from sandbox.common import rest
from sandbox.common.proxy import OAuth

client = rest.Client(auth=OAuth(os.getenv('SANDBOX_TOKEN')))  # https://sandbox.yandex-team.ru/oauth

WAIT_STATUSES = ['ENQUEUING', 'ENQUEUED', 'ASSIGNED', 'PREPARING', 'EXECUTING', 'FINISHING']
SANDBOX_LINK = 'https://sandbox.yandex-team.ru/task/{}/view'
LUNAPARK_LINK = 'https://lunapark.yandex-team.ru/{}'
AMMO_URL = 'http://{host}/load_ammo.txt'
TANK_CONFIG_TEMPLATE = '''
phantom:
    load_profile: {{load_type: rps, schedule: "line(1, 400, 30s) step(400, 1000, 50, 60)"}}
    uris: []
    header_http: "1.1"
    loop: 100000
    writelog: "0"
    instances: 3000
    timeout: "10s"
    port: "443"
    ssl: true
uploader:
    enabled: true
    job_dsc: "Antiadblock load testing"
    job_name: "{serv}.{ver}"
    operator: {operator}
    package: yandextank.plugins.DataUploader
    task: "{ticket}"
    ver: "{serv}.{ver}"
'''
STUBS = {
    'loadtest_cryprox_sas': 's7onyu7gjotcvwbe.sas.yp-c.yandex.net',
    'loadtest_cryprox_man': 'qfgws3n7wmkixrxl.man.yp-c.yandex.net',
    'loadtest_cryprox_vla': 'kyf4dpzya53sfixm.vla.yp-c.yandex.net',
    'loadtest_cryprox_myt': 'n33p5budynl2xlqz.myt.yp-c.yandex.net',
    'loadtest_cryprox_iva': 'ypjfgmphs5h6bfr2.iva.yp-c.yandex.net',
}
TARGET_SERVICES = ['loadtest_cryprox_sas', 'loadtest_cryprox_man', 'loadtest_cryprox_vla', 'loadtest_cryprox_myt', 'loadtest_cryprox_iva']
SHOOT_RE = re.compile(r'https?://lunapark\.yandex-team\.ru/\d+')
TIMEOUT = 2400
CHECK_PERIOD = 60


def shoot(ver, ticket, operator):
    task_ids = dict()
    for service in TARGET_SERVICES:
        _id = client.task({"type": "SHOOT_VIA_TANKAPI"})["id"]
        task_ids[service] = _id
        client.task[_id] = {
            "owner": "ANTIADBLOCK",
            "kill_timeout": TIMEOUT,
            "important": False,
            "fail_on_any_error": False,
            "custom_fields": [
                {"name": "nanny_service", "value": service},
                {"name": "config_source", "value": 'file'},
                {"name": "config_content", "value": TANK_CONFIG_TEMPLATE.format(serv=service, ver=ver, ticket=ticket, operator=operator)},
                {"name": "ammo_source", "value": 'url'},
                {"name": "ammo_source_url", "value": AMMO_URL.format(host=STUBS[service])},
            ]
        }
    client.batch.tasks.start.update(task_ids.values())
    pprint({s: SANDBOX_LINK.format(i) for s, i in task_ids.items()})
    pprint('On air shoots: ' + LUNAPARK_LINK.format(ticket))
    task_results = dict()
    wait_tasks = task_ids.copy()
    for i in range(TIMEOUT/CHECK_PERIOD + 1):
        for service, _id in wait_tasks.items():
            task = client.task[_id].read()
            status = task['status']
            if status not in WAIT_STATUSES:
                task_results[service] = task['results']['info']
                wait_tasks.pop(service)
        pprint('Waiting {}, timeout in {} s'.format(wait_tasks.keys(), TIMEOUT - CHECK_PERIOD * i))
        if not wait_tasks:
            break
        time.sleep(CHECK_PERIOD)

    result = dict()
    for service, info in task_results.items():
        try:
            result[service] = SHOOT_RE.search(info).group()
        except Exception:
            pprint('Failed to parse result of task')
            pprint(SANDBOX_LINK.format(task_ids[service]))
            pprint(info)
    return result


@click.command('launch_shoots')
@click.option("--count", default=1, help="Number of shoots.")
@click.option("--ver", default='default', help="Shoot version.")
@click.option("--ticket", default="ANTIADB-1872", help="Startrek ticket link shoots to.")
@click.option("--operator", required=True, help="Your staff login.")
def launch_shoots(count, ver, ticket, operator):
    results = defaultdict(list)
    for i in range(count):
        result = shoot(ver + '.' + str(i), ticket, operator)
        pprint(result)
        for service in result:
            results[service].append(result[service])
    pprint(dict(results))


if __name__ == '__main__':
    launch_shoots()
