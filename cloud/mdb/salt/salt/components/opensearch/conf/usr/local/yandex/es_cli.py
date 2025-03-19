#!/usr/bin/env python3

import argparse
import json
import socket

from elasticsearch import Elasticsearch

USER = "{{ salt.pillar.get('data:opensearch:users:mdb_admin:name') }}"
PASSWORD = "{{ salt.pillar.get('data:opensearch:users:mdb_admin:password') }}"


def parse_args():
    """
    Parse command-line arguments.
    """
    parser = argparse.ArgumentParser()
    parser.set_defaults(cmd=cmd_health)

    subparsers = parser.add_subparsers(dest='command')

    parser_health = subparsers.add_parser('health')
    parser_health.set_defaults(cmd=cmd_health)

    parser_shards = subparsers.add_parser('shards', )
    parser_shards.add_argument('-i', '--index')
    parser_shards.add_argument('-f', '--additional-fields')
    parser_shards.set_defaults(cmd=cmd_shards)

    parser_indices = subparsers.add_parser('indices')
    parser_indices.add_argument('-i', '--index')
    parser_indices.add_argument('--red', action='store_true')
    parser_indices.add_argument('--yellow', action='store_true')
    parser_indices.add_argument('--green', action='store_true')
    parser_indices.set_defaults(cmd=cmd_indices)

    parser_mapping = subparsers.add_parser('mapping')
    parser_mapping.add_argument('index')
    parser_mapping.set_defaults(cmd=cmd_mapping)

    parser_index = subparsers.add_parser('index')
    parser_index.add_argument('index')
    parser_index.add_argument('-r', '--routing-table', action='store_true')
    parser_index.set_defaults(cmd=cmd_index)

    parser_master = subparsers.add_parser('master')
    parser_master.set_defaults(cmd=cmd_master)

    parser_license = subparsers.add_parser('license')
    parser_license.set_defaults(cmd=cmd_license)

    parser_nodes = subparsers.add_parser('nodes')
    parser_nodes.add_argument('-f', '--additional-fields')
    parser_nodes.set_defaults(cmd=cmd_nodes)

    parser_reroute = subparsers.add_parser('reroute')
    parser_reroute.set_defaults(cmd=cmd_reroute)

    parser_explain = subparsers.add_parser('explain')
    parser_explain.add_argument('-i', '--index')
    parser_explain.add_argument('-s', '--shard', type=int, default=0)
    parser_explain.add_argument('-r', '--replica', action='store_true')
    parser_explain.set_defaults(cmd=cmd_explain)

    parser_version = subparsers.add_parser('version')
    parser_version.set_defaults(cmd=cmd_version)

    parser_node = subparsers.add_parser('node')
    parser_node.add_argument('-m', '--metric')
    parser_node.set_defaults(cmd=cmd_node)

    parser_thread_pool = subparsers.add_parser('thread_pool')
    parser_thread_pool.add_argument('-f', '--additional-fields')
    parser_thread_pool.set_defaults(cmd=cmd_thread_pool)

    return parser.parse_args()


def cmd_explain(args):
    """
    https://www.elastic.co/guide/en/elasticsearch/reference/current/cluster-allocation-explain.html
    """
    if args.index == None:
        printj(_get_client().cluster.allocation_explain())
        return

    printj(_get_client().cluster.allocation_explain(body={
        'index': args.index,
        'primary': not args.replica,
        'shard': args.shard,
    }))


def cmd_reroute(args):
    """
    https://www.elastic.co/guide/en/elasticsearch/reference/current/cluster-reroute.html
    """
    printj(_get_client().cluster.reroute(params={'retry_failed': 'true'}))


def cmd_health(args):
    """
    https://www.elastic.co/guide/en/elasticsearch/reference/current/cluster-health.html
    """
    printj(_get_client().cluster.health())


def cmd_shards(args):
    """
    https://www.elastic.co/guide/en/elasticsearch/reference/current/cat-shards.html
    """
    fields = 'index,shard,prirep,state,docs,store,node'
    if args.additional_fields:
        fields = fields + ',' + args.additional_fields
    print(_get_client().cat.shards(index=args.index, params={'v': 'true', 'h': fields}))


def cmd_indices(args):
    """
    https://www.elastic.co/guide/en/elasticsearch/reference/current/cat-indices.html
    """
    params = {'v': 'true'}
    if args.red:
        params['health'] = 'red'
    if args.yellow:
        params['health'] = 'yellow'
    if args.green:
        params['health'] = 'green'
    print(_get_client().cat.indices(index=args.index, params=params))


def cmd_mapping(args):
    """
    https://www.elastic.co/guide/en/elasticsearch/reference/current/indices-get-mapping.html
    """
    printj(_get_client().indices.get_mapping(index=args.index))


def cmd_index(args):
    """
    https://www.elastic.co/guide/en/elasticsearch/reference/current/indices-get-index.html
    https://www.elastic.co/guide/en/elasticsearch/reference/current/cluster-state.html
    """
    client = _get_client()

    if args.routing_table:
        state = client.cluster.state()
        host_uuids = {}
        for uuid, props in state.get('nodes', {}).items():
            host_uuids[uuid] = props['name']
        if args.index not in state['routing_table']['indices']:
            raise Exception(f'index {args.index} not found')
        routing_table = state['routing_table']['indices'][args.index]
        for shard in routing_table['shards'].values():
            for replica in shard:
                if replica['node']:  # replica might be unassigned
                    replica['fqdn'] = host_uuids[replica['node']]
        printj(routing_table)
        return

    printj(client.indices.get(index=args.index))


def cmd_master(args):
    """
    https://www.elastic.co/guide/en/elasticsearch/reference/current/cat-master.html
    """
    print(_get_client().cat.master())


def cmd_license(args):
    """
    https://www.elastic.co/guide/en/elasticsearch/reference/current/get-license.html
    """
    printj(_get_client().license.get())


def cmd_nodes(args):
    """
    https://www.elastic.co/guide/en/elasticsearch/reference/current/cat-nodes.html
    """
    fields = 'name,heap.percent,cpu,disk.used_percent,jdk,node.role,master,version'
    if args.additional_fields:
        fields = fields + ',' + args.additional_fields
    print(_get_client().cat.nodes(params={'v': 'true', 'h': fields}))


def _node_info(metric=None):
    nodes = _get_client().nodes.info(metric=metric, params={'flat_settings': 'true'})
    host_name = socket.gethostname()
    for props in nodes.get('nodes', {}).values():
        if props['name'] == host_name:
            return props
    raise Exception('could not get node info for {}'.format(host_name))


def cmd_version(args):
    print(_node_info()['version'])


def cmd_node(args):
    """
    https://www.elastic.co/guide/en/elasticsearch/reference/current/cluster-nodes-info.html
    """
    printj(_node_info(args.metric))


def cmd_thread_pool(args):
    """
    https://www.elastic.co/guide/en/elasticsearch/reference/current/cat-thread-pool.html
    """
    fields = 'host,name,queue,core,active,rejected,completed,max,largest'
    if args.additional_fields:
        fields = fields + ',' + args.additional_fields
    print(_get_client().cat.thread_pool(params={'v': 'true', 'h': fields}))


def printj(v):
    print(json.dumps(v, indent=4))


def _get_client():
    hosts = [
        {
            'host': socket.gethostname(),
            'port': 9200,
        }
    ]
    return Elasticsearch(
        hosts,
        use_ssl=True,
        verify_certs=True,
        http_auth=(USER, PASSWORD),
        ca_certs='/etc/ssl/certs/ca-certificates.crt'
    )


def main():
    args = parse_args()
    args.cmd(args)


if __name__ == '__main__':
    main()
