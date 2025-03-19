"""
Utilities for dealing with ElasticSearch
"""
from elasticsearch import Elasticsearch
from elasticsearch.helpers import bulk

from tests.helpers.internal_api import build_host_type_dict, get_cluster_ports

DATA_NODE_HOST = 'DATA_NODE'
MASTER_NODE_HOST = 'MASTER_NODE'


def _hosts(context, host_type=None):
    return [{'host': h, 'port': p} for h, p in get_cluster_ports(context, context.hosts, 9200, host_type=host_type)]


def _get_client(context=None, hosts=None, host_type=None, user=None, password=None):
    if not user:
        user = 'admin'
        password = context.cluster_config['config_spec']['admin_password']
    return Elasticsearch(
        hosts if hosts else _hosts(context, host_type),
        retry_on_timeout=True,
        max_retries=5,
        timeout=20,
        use_ssl=True,
        verify_certs=False,
        http_auth=(user, password),
        ca_certs="/opt/yandex/allCAs.pem")


def _count_node_roles(nodes_stat):
    res = {'master': 0, 'data': 0}
    for node in nodes_stat['nodes'].values():
        if 'master' in node['roles']:
            res['master'] += 1
        if 'data' in node['roles']:
            res['data'] += 1
    return res


def elasticsearch_alive(context, user=None, password=None, **_):
    """
    Check that elasticsearch is alive.
    """
    num_hosts = len(context.hosts)
    client = _get_client(context, user=user, password=password)
    health = client.cluster.health(params={'wait_for_status': 'yellow', 'timeout': '60s'})

    assert health['number_of_nodes'] == num_hosts, 'Number of hosts is {} (expected {})'.format(
        health['number_of_nodes'], num_hosts)

    hosts_by_type = build_host_type_dict(context)
    node_cnts = _count_node_roles(client.nodes.stats())

    if MASTER_NODE_HOST in hosts_by_type:
        assert len(
            hosts_by_type[MASTER_NODE_HOST]) == node_cnts['master'], 'Number of master nodes {} (expected {})'.format(
                len(hosts_by_type[MASTER_NODE_HOST]), node_cnts['master'])
        assert len(hosts_by_type[DATA_NODE_HOST]) == node_cnts['data'], 'Number of data nodes {} (expected {})'.format(
            len(hosts_by_type[DATA_NODE_HOST]), node_cnts['data'])
    else:
        assert node_cnts['data'] == node_cnts[
            'master'], 'Number of master nodes ({}) not equal to number of data nodes ({})'.format(
                node_cnts['data'], node_cnts['master'])
        assert node_cnts['data'] == len(hosts_by_type[DATA_NODE_HOST]), 'Number of data nodes {} (expected {})'.format(
            len(hosts_by_type[DATA_NODE_HOST]), node_cnts['data'])


def index_documents(context, index_name, docs):
    """
    Index documents to specified index.
    """
    for i, doc in enumerate(docs):
        doc['_index'] = index_name
        if '_id' not in doc:
            doc['_id'] = str(i)

    client = _get_client(context)
    cnt, _ = bulk(client, docs, max_retries=5)
    client.indices.refresh(index=index_name)
    return cnt


def count_documents(context, index_name):
    """
    Count documents within specified index.
    """
    client = _get_client(context)
    res = client.count(index=index_name)
    return res['count']


def elasticsearch_info(context):
    """
    Returns basic information about the cluster.
    """
    client = _get_client(context)
    return client.info()


def elasticsearch_version(context):
    """
    Returns cluster version.
    """
    return elasticsearch_info(context)["version"]["number"]


def _search_documents(context, hosts, index_name, query):
    client = _get_client(context=context, hosts=hosts)
    res = client.search(body=query, index=index_name)
    return {h['_id']: h['_source'] for h in res['hits']['hits']}


def search_documents(context, index_name, query):
    """
    Search documents within specified index on each host separately.
    """
    responses = []
    for host in _hosts(context):
        responses.append(_search_documents(context, [host], index_name, query))
    return responses


def _get_nodes_stats(context, path, role='data'):
    client = _get_client(context)
    nodes = client.nodes.stats()

    def _get_key(data):
        try:
            for key in path.split(':'):
                if key.isdigit():
                    key = int(key)
                data = data[key]
        except KeyError:
            raise Exception('path {} not found'.format(path))
        return data

    for params in nodes['nodes'].values():
        if role in params['roles']:
            return _get_key(params)

    raise Exception('role "{}" not found'.format(role))


def get_jvm_heap_in_bytes(context, role='data'):
    """
    Get jvm heap size in bytes.
    """
    return _get_nodes_stats(context, 'jvm:mem:heap_max_in_bytes', role)


def get_static_setting(context, setting, role='data'):
    """
    Get Elasticsearch static setting value.
    """

    def _get_setting(setting):
        cur = settings['defaults']
        for node in setting.split('.'):
            cur = cur[node]
        return cur

    host_type = {'data': DATA_NODE_HOST, 'master': MASTER_NODE_HOST}[role]

    client = _get_client(context, host_type=host_type)
    settings = client.cluster.get_settings(params={'include_defaults': 'true'})
    return _get_setting(setting)


def count_nodes(context, role=''):
    """
    Get number of nodes of specified type.
    """
    nodes = _get_client(context).nodes.stats()
    if role == '':
        return len(nodes['nodes'])
    return _count_node_roles(nodes)[role]


def nodes_plugins(context):
    """
    Get plugins installed on cluster nodes
    """
    info = _get_client(context).nodes.info(metric='plugins')
    nodes = []
    for _, node in info['nodes'].items():
        plugins = []
        for plugin in node['plugins']:
            plugins.append(plugin['name'])
        nodes.append(plugins)
    return nodes


def nodes_versions(context):
    """
    Get versions of cluster nodes
    """
    info = _get_client(context).nodes.info()
    return [{
        'fqdn': node['host'],
        'version': '.'.join(node['version'].split('.')[0:2]),
        'roles': node['roles'],
    } for node in info['nodes'].values()]
