"""
Deploy API integration
"""
from typing import Dict

import base64
import requests
import json
from retrying import retry

from tests.helpers.utils import ALL_CA_LOCAL_PATH

from .utils import env_stage


class DeployAPIError(RuntimeError):
    """
    General API error exception
    """

    pass


# pylint: disable=not-callable
def request(conf, handle, method='GET', deserialize=False, **kwargs):
    """
    Perform request to Deploy API and check response on internal server
    errors (error 500).

    If deserialize is True, the response will also be checked on all
    client-side (4xx) and server-side (5xx) errors, and then deserialized
    by invoking Response.json() method.
    """
    deploy_config = conf['deploy_api']
    url = f"https://{deploy_config['fqdn']}:{deploy_config['port']}"

    kwargs['headers'] = {
        'Authorization': 'OAuth ' + deploy_config['token'],
        'Accept': 'application/json',
        'Content-Type': 'application/json',
    }
    kwargs['verify'] = ALL_CA_LOCAL_PATH

    res = requests.request(
        method,
        f'{url}/{handle.lstrip("/")}',
        **kwargs,
    )

    try:
        res.raise_for_status()
    except requests.HTTPError as exc:
        if deserialize or res.status_code == 500:
            msg = '{0} {1} failed with {2}: {3}'.format(method, handle, res.status_code, res.text)
            raise DeployAPIError(msg) from exc

    return res.json() if deserialize else res


def group_exists(conf: Dict, group_name: str) -> bool:
    res = request(conf, f'/v1/groups/{group_name}')
    if res.status_code == 404:
        return False
    if res.status_code == 200 and res.json():
        return True
    msg = f"Deploy API request 'group_exists' for group {group_name} failed with {res.status_code}: {res.text}"
    raise DeployAPIError(msg)


def create_group(conf: Dict, group_name: str) -> Dict:
    data = {'name': group_name}
    return request(conf, "/v1/groups", 'POST', deserialize=True, json=data)


def ensure_group(conf: Dict, group_name: str):
    if group_exists(conf, group_name):
        return
    create_group(conf, group_name)


def master_exists(conf: Dict, fqdn: str) -> bool:
    res = request(conf, f'/v1/masters/{fqdn}')
    if res.status_code == 404:
        return False
    if res.status_code == 200 and res.json():
        return True
    msg = f"Deploy API request 'master_exists' for master {fqdn} failed with {res.status_code}: {res.text}"
    raise DeployAPIError(msg)


def create_master(conf: Dict, fqdn: str, group_name: str) -> Dict:
    data = {
        'fqdn': fqdn,
        'isOpen': True,
        'group': group_name,
    }
    return request(conf, "/v1/masters", 'POST', deserialize=True, json=data)


def ensure_master(conf, fqdn: str, group_name: str):
    if master_exists(conf, fqdn):
        return
    create_master(conf, fqdn, group_name)


@retry(wait_fixed=5000, stop_max_attempt_number=120, retry_on_exception=lambda _: True)
def _wait_deploy_api(conf):
    res = request(conf, '/v1/groups')
    res.raise_for_status()


@retry(wait_fixed=5000, stop_max_attempt_number=120, retry_on_exception=lambda _: True)
def wait_master_alive(conf, fqdn):
    res = request(conf, f'/v1/masters/{fqdn}')
    res.raise_for_status()
    data = res.json()
    alive = data.get('isAlive', False)
    if not alive:
        raise Exception(f'Master {fqdn} is not alive. Status {alive}')


@env_stage('start', fail=True)
def register_master(state: Dict, conf: Dict, **_):
    group_name = conf['deploy_api']['group']
    fqdn = conf['compute_driver']['fqdn']
    _wait_deploy_api(conf)
    ensure_group(conf, group_name)
    ensure_master(conf, fqdn, group_name)
    wait_master_alive(conf, fqdn)


def list_shipments(conf):
    res = request(conf, '/v1/shipments')
    res.raise_for_status()
    return res.json()


def list_shipment_jobs(conf, shipment_id):
    params = {'shipmentId': shipment_id}
    res = request(conf, '/v1/jobs', params=params)
    res.raise_for_status()
    return res.json()


def get_job_results(conf, fqdn, job_id):
    params = {
        'fqdn': fqdn,
        'jobId': job_id,
    }
    res = request(conf, '/v1/jobresults', params=params)
    res.raise_for_status()
    job_results = res.json()
    for result in job_results['jobResults']:
        result_bytes = base64.decodebytes(bytes(result['result'], 'utf8'))
        result_data = json.loads(result_bytes)
        result['result'] = result_data
    return job_results
