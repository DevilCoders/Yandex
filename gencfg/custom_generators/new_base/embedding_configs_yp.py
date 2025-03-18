#!/skynet/python/bin/python
# -*- coding: utf-8 -*-

import argparse
import logging

import gen_conf_utils
import gaux.aux_shards
import gaux.aux_resolver
from core.db import CURDB  # noqa


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--target-dir', help='dir to store configs', required=True)
    parser.add_argument('--group', help='group to make configs', required=True)
    parser.add_argument('--inv-index-yplookup', help='inverted index yplookup', required=True)
    parser.add_argument('--config-template', help='path to config template', required=True)
    parser.add_argument('--workload', help='generate workload configs', required=False, type=str)
    return parser.parse_args()


def _inv_index_shard_replics(sd_expression):
    return 'InvertedIndexStorageShardsReplics {{\n    SearchScriptCgis: "{}"\n}}'.format(sd_expression)


# Supported configuration(s): embeddings in gencfg, invindex in yp.
def generate_configs(target_dir, config_template, embedding_group_mapping, yplookup, workload=None):
    # Dirty hack: switch off mem lock in workloads (hamster, etc)
    config_template = config_template if not workload else 'LockMemory: False\n' + config_template

    for (host, port), shard_template in embedding_group_mapping.iteritems():
        partition = gaux.aux_shards.partition_number(shard_template)
        content = config_template.replace('__REPLICAS__', '\n'.join(_inv_index_shard_replics(yplookup.base.sources[i].sd_expression)
                                                                    for i in range(partition * yplookup.base.group_size, (partition + 1) * yplookup.base.group_size)))
        content = content.replace('__DNS_CACHE__', '')
        config_name = gen_conf_utils.config_name(host, port, workload)
        gen_conf_utils.store_cfg(target_dir, config_name, content)


def main():
    args = parse_args()
    with open(args.config_template) as f:
        config_template = f.read()

    generate_configs(args.target_dir, config_template,
                     gen_conf_utils.instance_to_shard_mapping(args.group),
                     CURDB.yplookups.get_yplookup_by_name(args.inv_index_yplookup)[0],
                     args.workload)


if __name__ == '__main__':
    main()
