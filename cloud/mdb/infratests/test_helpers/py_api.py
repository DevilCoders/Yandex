"""
Utilities for dealing with Python Internal API
"""
import requests
import time
from typing import Dict, Sequence, Tuple

from cloud.mdb.infratests.test_helpers.context import Context
from cloud.mdb.infratests.test_helpers.types import Cluster, Job, Host, Subcluster


class InternalAPIError(RuntimeError):
    """
    General API error exception
    """

    pass


def get_base_url(context: Context):
    """
    Get base URL for sending requests to Internal API.
    """
    return f"https://{context.test_config.py_api_url}:443"


def get_cluster(context: Context, cluster_name: str, folder_id: str) -> Cluster:
    """
    Get cluster by name.
    """
    response = get_cluster_request(context, cluster_name, folder_id).json()
    if 'id' in response:
        return response

    raise InternalAPIError('Failed to get cluster: {0}'.format(response))


def get_clusters(context: Context, folder_id: str, cluster_type: str = None, deserialize: bool = True):
    """
    Get clusters.
    """
    if cluster_type is None:
        cluster_type = context.cluster_type
    handle = 'mdb/{0}/v1/clusters'.format(cluster_type)
    params = {'folderId': folder_id}

    return request(context, handle=handle, params=params, deserialize=deserialize)


def _get_cluster_id(context: Context, cluster_name: str, folder_id: str) -> str:
    """
    Get cluster id by name.
    """
    for cluster in get_clusters(context, folder_id)['clusters']:
        if cluster['name'] == cluster_name:
            return cluster['id']

    raise InternalAPIError('cluster {0} not found'.format(cluster_name))


def get_cluster_request(context: Context, cluster_name: str, folder_id: str):
    """
    Get cluster by name. Return http response
    """
    cluster_id = _get_cluster_id(context, cluster_name, folder_id)
    handle = 'mdb/{0}/v1/clusters/{1}'.format(context.cluster_type, cluster_id)
    return request(context, handle=handle)


def get_console_cluster_config(context: Context):
    """
    Get cluster configs for console.
    """
    handle = '/mdb/{0}/1.0/console/clusters:config'.format(context.cluster_type)
    return request(context, handle, params={'folderId': context.test_config.folder_id})


def get_dataproc_job_log(context: Context) -> Tuple[str, str]:
    """
    Get dataproc job log
    """
    page_token = 0
    content = str()
    error = None
    while True:
        params = {}
        if page_token:
            params['pageToken'] = page_token
        chunk_req = request(
            context,
            handle=f'mdb/hadoop/v1/clusters/{context.cid}/jobs/{context.job_id}:logs',
            params=params,
        )
        response = chunk_req.json()
        if response.get('content'):
            content = content + response['content']
        if chunk_req.status_code >= 400:
            error = response.get('message')
            if error:
                return '', error
            chunk_req.raise_for_status()
        page_token = response.get('nextPageToken')
        # If content is empty and nextPageToken is not empty, that means that
        # job is still running and we should wait
        if not response['content'] and page_token:
            time.sleep(1)
        if page_token is None:
            break
    return content, error


def get_job(context: Context, job_id: str) -> Job:
    """
    Get dataproc job
    """
    task_req = request(
        context,
        handle=f'mdb/hadoop/v1/clusters/{context.cid}/jobs/{job_id}',
    )
    task_req.raise_for_status()
    return task_req.json()


def get_hosts(context: Context, cluster_id: str, deserialize: bool = True) -> Sequence[Host]:
    """
    Get cluster hosts.
    """
    handle = '/mdb/{0}/v1/clusters/{1}/hosts'.format(context.cluster_type, cluster_id)
    return request(context, handle, deserialize=deserialize)['hosts']


def get_hosts_of_subcluster(context: Context, subcluster_name: str):
    """
    Returns all host objects of subcluster from py api.
    """
    subcluster_id = context.subcluster_id_by_name[subcluster_name]
    return [host for host in context.hosts if host['subclusterId'] == subcluster_id]


def get_subclusters(context: Context, cluster_id: str, deserialize: bool = True) -> Sequence[Subcluster]:
    """
    Get subclusters.
    """
    handle = '/mdb/{0}/v1/clusters/{1}/subclusters'.format(context.cluster_type, cluster_id)
    return request(context, handle, deserialize=deserialize)['subclusters']


def get_task(context: Context) -> Dict:
    """
    Get current task
    """
    task_req = request(
        context,
        handle=f'mdb/v1/operations/{context.operation_id}',
    )
    task_req.raise_for_status()
    return task_req.json()


def load_cluster_into_context(context: Context, cluster_name: str = None, with_hosts: bool = True):
    """
    Reload cluster with related objects and update corresponding context data.
    """
    if not cluster_name:
        cluster_name = context.cluster['name']

    folder_id = context.test_config.folder_id
    context.cluster = get_cluster(context, cluster_name, folder_id)
    context.cid = context.cluster['id']

    if with_hosts:
        context.hosts = get_hosts(context, context.cid)

    if context.cluster_type == 'hadoop':
        context.subclusters = get_subclusters(context, context.cid)
        context.subcluster_id_by_name = {s['name']: s['id'] for s in context.subclusters}


def modify_cluster(context: Context, cluster_data: dict) -> requests.Response:
    handle = 'mdb/{type}/v1/clusters/{cluster_id}'.format(type=context.cluster_type, cluster_id=context.cid)
    return request(context, handle=handle, method='PATCH', json=cluster_data)


# pylint: disable=not-callable
def request(context: Context, handle: str, method: str = 'GET', deserialize: bool = False, **kwargs):
    """
    Perform request to Internal API and check response on internal server
    errors (error 500).

    If deserialize is True, the response will also be checked on all
    client-side (4xx) and server-side (5xx) errors, and then deserialized
    by invoking Response.json() method.
    """
    if 'session' not in context:
        session = requests.Session()
        session.headers.update(
            {
                'Accept': 'application/json',
                'Content-Type': 'application/json',
                'X-YaCloud-SubjectToken': context.user_iam_token,
            }
        )
        session.verify = context.test_config.ca_path
        context.session = session

    res = context.session.request(
        method,
        f'{get_base_url(context)}/{handle.lstrip("/")}',
        **kwargs,
    )

    try:
        res.raise_for_status()
    except requests.HTTPError as exc:
        if deserialize or res.status_code == 500:
            msg = '{0} {1} failed with {2}: {3}'.format(method, handle, res.status_code, res.text)
            raise InternalAPIError(msg) from exc

    return res.json() if deserialize else res


def submit_dataproc_job(context: Context, job_data: dict) -> requests.Response:
    url = 'mdb/hadoop/v1/clusters/{0}/jobs'.format(context.cid)
    return request(
        context,
        method='POST',
        handle=url,
        json=job_data,
    )
