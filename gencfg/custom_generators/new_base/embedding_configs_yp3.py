#!/skynet/python/bin/python
# -*- coding: utf-8 -*-

import argparse

import gen_conf_utils
from core.db import CURDB  # noqa


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--target-dir', help='dir to store configs', required=True)
    parser.add_argument('--yplookup', help='yplookup to make configs for', required=True)
    parser.add_argument('--config-template', help='path to config template', required=True)
    parser.add_argument('--workload', help='workload name', required=True)
    return parser.parse_args()


def _inv_index_shard_replics(sd_expression):
    return 'InvertedIndexStorageShardsReplics {{\n    SearchScriptCgis: "{}"\n}}'.format(sd_expression)


def patch_config_template(config_template, partition_number, yplookup):
    return config_template.replace(
        '__REPLICAS__',
        '\n'.join(
            _inv_index_shard_replics(yplookup.base.sources[i].sd_expression)
            for i in range(partition_number * yplookup.base.group_size, (partition_number + 1) * yplookup.base.group_size)
        )
    ).replace('__DNS_CACHE__', '')


def generate_configs(target_dir, config_template, yplookup, workload):
    for partition_number, source in enumerate(yplookup.intl1.sources):
        cfg_name = '{pod_set_id}.{partition_number}.{workload}.embeddingstorage.cfg'.format(
            pod_set_id=yplookup.intl1.pod_set_id,
            partition_number=partition_number,
            workload=workload
        )
        cfg_content = patch_config_template(config_template, partition_number, yplookup)
        gen_conf_utils.store_cfg(target_dir, cfg_name, cfg_content)


def main():
    args = parse_args()
    with open(args.config_template) as f:
        config_template = f.read()
    generate_configs(
        args.target_dir,
        config_template,
        CURDB.yplookups.get_yplookup_by_name(args.yplookup)[0],
        args.workload
    )


if __name__ == '__main__':
    main()
