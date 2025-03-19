#!/usr/bin/env python3.6
from es_lib import parse_args, get_client, die, SELF_FQDN


def main():
    """
    Program entry point.
    """
    args = parse_args()
    try:
        client = get_client(args.user, args.password)
        status = client.cluster.health(request_timeout=60)['status']
        if status == 'green':
            die(0, 'OK')
        elif status == 'yellow':
            die(1, 'replica shards are unassigned')
        elif status == 'red':
            explain = get_explain(client, SELF_FQDN)
            if explain == 'disk_threshold':
                die(1, 'primary shards are unassigned (disk space)')
            elif explain == 'no_valid_shard_copy':
                die(2, 'primary shards are unassigned (no valid shard copy, probably a node left)')
            die(2, 'primary shards are unassigned ({})'.format(explain))
        die(2, status)
    except Exception as e:
        die(2, f'cant get status: ({repr(e)})')


def get_explain(client, hostname):
    """
    Get why hostname can not run primary shards. Known reasons:
    1. disk_threshold
    2. no valid shard copy
    """
    def get_unassigned_primary_shards():
        cluster_state = client.cluster.state(metric='routing_table')
        shards = []
        for index_name, shards_info in cluster_state['routing_table']['indices'].items():
            for shard_name, shard_info in shards_info['shards'].items():
                for replica in shard_info:
                    if replica['primary'] and replica['state'] == 'UNASSIGNED':
                        shards.append({'index': index_name, 'shard': shard_name})
        return shards

    for shard in get_unassigned_primary_shards():
        explain = client.cluster.allocation_explain(body={
            'index': shard['index'],
            'shard': shard['shard'],
            'primary': True,
        })

        if explain['can_allocate'] == 'no_valid_shard_copy':
            return 'no_valid_shard_copy'

        for node in explain['node_allocation_decisions']:
            if node['node_name'] != hostname:
                continue
            for decider in node['deciders']:
                if decider['decider'] == 'disk_threshold':
                    return 'disk_threshold'
    return 'unknown'


if __name__ == '__main__':
    main()
