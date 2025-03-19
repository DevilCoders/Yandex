#!/usr/bin/env python3.6
import json

from es_lib import parse_args, get_client, die, SELF_FQDN


def main():
    """
    Program entry point.
    """
    args = parse_args()
    try:
        client = get_client(args.user, args.password)
        if client.nodes.info(node_id='data:true')['_nodes']['total'] > 1:
            cluster_state = client.cluster.state(metric='nodes,routing_table')
            result = hosts_with_unique_data(cluster_state).get(SELF_FQDN)
            if result:
                output_limit = 10
                die(2, f'has unique data: {json.dumps(result[:output_limit])}{" and more..." if len(result) > output_limit else ""}')
        die(0, 'OK')
    except Exception as e:
        die(2, f'cant get status: ({repr(e)})')


def hosts_with_unique_data(cluster_state):
    result = {}
    nodes = cluster_state['nodes']
    routing_table = cluster_state['routing_table']

    for index_name, index_props in routing_table['indices'].items():
        for shard_num, shard_props in index_props['shards'].items():
            replica_count = 0
            last_node_id = None
            for replica in shard_props:
                if replica['state'] == 'STARTED':
                    replica_count += 1
                    last_node_id = replica['node']
            if replica_count == 1:
                node_fqdn = nodes[last_node_id]['name']
                if node_fqdn not in result:
                    result[node_fqdn] = []
                result[node_fqdn].append({
                    'index': index_name,
                    'shard': shard_num,
                    'reason': 'non-started shards' if len(shard_props) > 1 else '0 replicas'
                })

    return result


if __name__ == '__main__':
    main()
