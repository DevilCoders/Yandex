"""
Utilities for dealing with Internal API
"""
import time
from collections import defaultdict
from operator import itemgetter

import requests
import retrying
from pkg_resources import parse_version

from tests.helpers.compute_driver import get_compute_api
from tests.helpers.utils import ALL_CA_LOCAL_PATH


class InternalAPIError(RuntimeError):
    """
    General API error exception
    """

    pass


def get_base_url(context):
    """
    Get base URL for sending requests to Internal API.
    """
    host = context.conf['compute_driver']['fqdn']
    port = context.conf['compute_driver']['port']
    return 'https://{0}:{1}'.format(host, port)


# pylint: disable=not-callable
def request(context, handle, method='GET', deserialize=False, **kwargs):
    """
    Perform request to Internal API and check response on internal server
    errors (error 500).

    If deserialize is True, the response will also be checked on all
    client-side (4xx) and server-side (5xx) errors, and then deserialized
    by invoking Response.json() method.
    """
    compute_config = context.conf['compute_driver']['compute']
    token = compute_config['token']

    if 'session' not in context:
        session = requests.Session()
        session.headers.update(
            {
                'Accept': 'application/json',
                'Content-Type': 'application/json',
                'X-YaCloud-SubjectToken': token,
            }
        )
        session.verify = ALL_CA_LOCAL_PATH
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


def decrypt(context, data, version=1):
    """
    Decrypt a dict and replace it with string
    """
    is_cyphertext = isinstance(data, dict) and int(data.get('encryption_version', -1)) == version and data.get('data')
    if not is_cyphertext:
        return data
    box = context.conf['dynamic']['internal_api']['box']
    return box.decrypt_utf(data['data'])


def get_clusters(context, folder_id, cluster_type=None, deserialize=True):
    """
    Get clusters.
    """
    if cluster_type is None:
        cluster_type = context.cluster_type
    handle = 'mdb/{0}/v1/clusters'.format(cluster_type)
    params = {'folderId': folder_id}

    return request(context, handle=handle, params=params, deserialize=deserialize)


def get_cluster_request(context, cluster_name, folder_id):
    """
    Get cluster by name. Return http response
    """
    cluster_id = _get_cluster_id(context, cluster_name, folder_id)
    handle = 'mdb/{0}/v1/clusters/{1}'.format(context.cluster_type, cluster_id)
    return request(context, handle=handle)


def get_cluster(context, cluster_name, folder_id):
    """
    Get cluster by name.
    """
    response = get_cluster_request(context, cluster_name, folder_id).json()
    if 'id' in response:
        return response

    raise InternalAPIError('Failed to get cluster: {0}'.format(response))


def _get_cluster_id(context, cluster_name, folder_id):
    """
    Get cluster id by name.
    """
    for cluster in get_clusters(context, folder_id)['clusters']:
        if cluster['name'] == cluster_name:
            return cluster['id']

    raise InternalAPIError('cluster {0} not found'.format(cluster_name))


def get_console_cluster_config(context, folder_id):
    """
    Get cluster configs for console.
    """
    handle = '/mdb/{0}/1.0/console/clusters:config'.format(context.cluster_type)
    return request(context, handle, params={'folderId': folder_id})


def get_hosts(context, cluster_id, deserialize=True):
    """
    Get cluster hosts.
    """
    handle = '/mdb/{0}/v1/clusters/{1}/hosts'.format(context.cluster_type, cluster_id)
    return request(context, handle, deserialize=deserialize)['hosts']


def get_host_config(context, hostname):
    """
    Get host config (pillar) by name.
    """
    return request(
        context,
        'api/v1.0/config/{fqdn}'.format(fqdn=hostname),
        headers={
            'Access-Id': context.conf['dynamic']['salt']['access_id'],
            'Access-Secret': context.conf['dynamic']['salt']['access_secret'],
        },
    ).json()


def load_cluster_type_info_into_context(context):
    """
    Reload cluster type information and update corresponding context data.
    """
    params = {'folderId': context.folder['folder_ext_id']}
    response = request(context, '/mdb/1.0/console/clusters:types', params=params, deserialize=True)

    for obj in response["clusterTypes"]:
        if obj['type'] != context.cluster_type:
            continue

        context.versions = sorted(obj.get('versions', []), key=parse_version)


def build_host_type_dict(context):
    """
    Get map of host_type => host
    """
    hosts = defaultdict(list)
    for host in context.hosts:
        host_type = host.get('type', context.cluster_type.upper())
        shard_name = host.get('shardName')
        k = shard_name if shard_name else host_type
        hosts[k].append(host)
    return hosts


def group_hosts_by_type(context):
    """
    Get classified list of context.hosts
    """
    host_map = build_host_type_dict(context)
    return list(host_map.values())


def patch_create_ssl_ctx(func):
    """
    Monkey patch for pymysql library, allows to localhost IP using SSL
    """

    def wrapper(self, *args, **kwargs):
        ctx = func(self, *args, **kwargs)
        ctx.check_hostname = False
        return ctx

    return wrapper


def convert_version_to_spec(version):
    """
    Convert postgresql version to config spec format (postgresqlConfig_9_6)
    Examples:
        '9.6' -> '9_6'
        '10'  -> '10'
    """
    return version.replace('.', '_')


def get_subclusters(context, cluster_id, deserialize=True):
    """
    Get subclusters.
    """
    handle = '/mdb/{0}/v1/clusters/{1}/subclusters'.format(context.cluster_type, cluster_id)
    return request(context, handle, deserialize=deserialize)['subclusters']


def get_subcluster(context, subcluster_id):
    """
    Get subcluster.
    """
    cluster_id = context.cluster['id']
    handle = f'/mdb/{context.cluster_type}/v1/clusters/{cluster_id}/subclusters/{subcluster_id}'
    return request(context, handle, deserialize=False)


def get_subcluster_by_name(context, name):
    subclusters = get_subclusters(context, cluster_id=context.cluster['id'])
    for sub in subclusters:
        if sub['name'] == name:
            return sub


def load_cluster_into_context(context, cluster_name=None, with_hosts=True):
    """
    Reload cluster with related objects and update corresponding context data.
    """
    if not cluster_name:
        cluster_name = context.cluster['name']

    context.cluster = get_cluster(context, cluster_name, context.folder['folder_ext_id'])
    context.cid = context.cluster['id']

    if with_hosts:
        context.hosts = get_hosts(context, context.cluster['id'])

    if context.cluster_type == 'hadoop':
        context.subclusters = get_subclusters(context, context.cluster['id'])
        context.subclusters_id_by_name = {subcluster['name']: subcluster['id'] for subcluster in context.subclusters}


def ensure_cluster_is_loaded_into_context(context, select='running'):
    """
    Ensures that cluster info is available via context. Useful when developing tests.
    """
    if 'cluster' in context:
        return

    clusters = get_clusters(context, context.folder['folder_ext_id'])['clusters']
    if select == 'running':
        clusters = [c for c in clusters if c['status'] == 'RUNNING']
        if len(clusters) > 1:
            raise Exception('Failed to determine active cluster under test: more than one RUNNING cluster found')
        if not clusters:
            raise Exception('Failed to determine active cluster under test: no RUNNING cluster is found')
        cluster = clusters[0]
    elif select == 'latest':
        cluster = sorted(clusters, key=itemgetter('createdAt'))[-1]
    else:
        raise Exception(f'Unknown select mode: {select}')

    load_cluster_into_context(context, cluster_name=(cluster['name']))


def get_task(context):
    """
    Get current task
    """
    task_req = request(
        context,
        handle=f'mdb/v1/operations/{context.operation_id}',
    )
    task_req.raise_for_status()
    return task_req.json()


@retrying.retry(wait_fixed=1000, stop_max_attempt_number=3)
def get_job(context, job_id):
    """
    Get dataproc job
    """
    cid = context.cluster['id']
    task_req = request(
        context,
        handle=f'mdb/hadoop/v1/clusters/{cid}/jobs/{job_id}',
    )
    task_req.raise_for_status()
    return task_req.json()


def get_dataproc_job_log(context):
    """
    Get dataproc job log
    """
    cid = context.cluster['id']
    job_id = context.job_id
    page_token = 0
    content = str()
    error = None
    while True:
        params = {}
        if page_token:
            params['pageToken'] = page_token
        chunk_req = request(
            context,
            handle=f'mdb/hadoop/v1/clusters/{cid}/jobs/{job_id}:logs',
            params=params,
        )

        response = chunk_req.json()
        if response.get('content'):
            content = content + response['content']
        if chunk_req.status_code >= 400:
            error = response.get('message')
            if error:
                return ('', error)
            chunk_req.raise_for_status()
        page_token = response.get('nextPageToken')
        # If content is empty and nextPageToken is not empty, that means that
        # job is still running and we should wait
        if not response['content'] and page_token:
            time.sleep(1)
        if page_token is None:
            break
    return (content, error)


def get_compute_instances_of_subcluster(context, subcluster_name, hosts_count):
    """
    Fetches all instance objects of subcluster from compute api.
    """
    hosts = get_hosts_of_subcluster(context, subcluster_name, hosts_count)
    api = get_compute_api(context)
    return [api.get_instance(host['name'], host['computeInstanceId']) for host in hosts]


def get_hosts_of_subcluster(context, subcluster_name, hosts_count):
    """
    Returns all host objects of subcluster from mdb api (aka internal api).
    """
    load_cluster_into_context(context)
    subcluster_id = context.subclusters_id_by_name.get(subcluster_name)
    assert subcluster_id, 'subcluster with name {} not found'.format(subcluster_name)
    hosts = [host for host in context.hosts if host['subclusterId'] == subcluster_id]
    assert hosts, "there're no hosts within subcluster {}".format(subcluster_name)
    assert len(hosts) == int(
        hosts_count
    ), f"expected subcluster {subcluster_name} to have {hosts_count} hosts, got {len(hosts)}"
    return hosts
