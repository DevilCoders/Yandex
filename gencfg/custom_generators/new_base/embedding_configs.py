#!/skynet/python/bin/python
# -*- coding: utf-8 -*-

import argparse
import logging

import gen_conf_utils
import gaux.aux_shards
import gaux.aux_resolver


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--target-dir', help='dir to store configs', required=True)
    parser.add_argument('--group', help='group to make configs', required=True)
    parser.add_argument('--inv-index-group', help='inverted index group', required=True)
    parser.add_argument('--config-template', help='path to config template', required=True)
    parser.add_argument('--add-hamster', help='generate hamster configs', action='store_true', default=False)
    return parser.parse_args()


def _inv_index_cgi(host, port, indent):
    return '{indent}SearchScriptCgis: "http2://{host}:{port}/inverted_index_storage"'.format(
        host=host, port=port, indent=indent,
    )


def _replicas(instances):
    assert instances
    replicas = '\n'.join(sorted([_inv_index_cgi(host, port, '  ') for host, port in instances]))
    return 'InvertedIndexStorageShardsReplics {\n' + replicas + '\n}'


def _dns_cache(shards_order, shard_template_to_instances_mapping):
    hosts = set()
    for inv_shard_template in shards_order:
        for host, port in shard_template_to_instances_mapping[inv_shard_template]:
            hosts.add(host)
    addrs = ' '.join('{}={}'.format(host, ip) for host, ip in gaux.aux_resolver.resolve_hosts(hosts, [6], fail_unresolved=False).iteritems() if ip)
    return 'DNSCache: "' + addrs + '"'

def patch_config_template(config_template, look_at):
    shard_template_to_instances_mapping = gen_conf_utils.invert_mapping(look_at)
    shards_order = sorted(shard_template_to_instances_mapping, key=gaux.aux_shards.shard_in_partition_number)
    shards = [
        _replicas(shard_template_to_instances_mapping[inv_shard_template])
        for inv_shard_template in shards_order
    ]
    dns_cache = _dns_cache(shards_order, shard_template_to_instances_mapping)
    return config_template.replace('__REPLICAS__', '\n'.join(shards)).replace('__DNS_CACHE__', dns_cache)


def generate_configs(target_dir, config_template, embedding_group_mapping, inv_index_group_mapping, workload=None):
    inv_index_by_partitions = gen_conf_utils.split_mapping_by_partitions(inv_index_group_mapping)

    # Dirty hack: switch off mem lock in workloads (hamster, etc)
    config_template_patched = config_template if not workload else 'LockMemory: False\n' + config_template

    for (host, port), shard_template in embedding_group_mapping.iteritems():
        partition_number = gaux.aux_shards.partition_number(shard_template)
        cfg_name = gen_conf_utils.config_name(host, port, workload)
        cfg_content = patch_config_template(config_template_patched, inv_index_by_partitions[partition_number])
        gen_conf_utils.store_cfg(target_dir, cfg_name, cfg_content)


def main():
    args = parse_args()
    with open(args.config_template) as f:
        config_template = f.read()
    generate_configs(
        args.target_dir,
        config_template,
        gen_conf_utils.instance_to_shard_mapping(args.group),
        gen_conf_utils.instance_to_shard_mapping(args.inv_index_group),
    )

    group_workloads = gen_conf_utils.get_group_workloads_names(args.group)
    inv_index_workloads = gen_conf_utils.get_group_workloads_names(args.inv_index_group)

    if group_workloads != inv_index_workloads:
        logging.warning(
            'Skip workloads %s due to these are not described at %s and %s',
            group_workloads.symmetric_difference(inv_index_workloads),
            args.group,
            args.inv_index_group,
        )

    for workload in group_workloads.intersection(inv_index_workloads):
        generate_configs(
            args.target_dir,
            config_template,
            gen_conf_utils.instance_to_shard_mapping(args.group),
            gen_conf_utils.instance_to_shard_mapping(args.inv_index_group, workload),
            workload,
        )


if __name__ == '__main__':
    main()
