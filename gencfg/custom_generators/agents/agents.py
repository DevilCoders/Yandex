#!/skynet/python/bin/python
# -*- coding: utf-8 -*-

import argparse
import json
import hashlib
import os
import sys

sys.path.append(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))

import config
import gencfg
import gaux.aux_hbf
from core.db import CURDB


def parse_args():
    default_target_dir = os.path.join(config.WEB_GENERATED_DIR, 'all')

    parser = argparse.ArgumentParser()
    parser.add_argument('--target-dir', help='dir to store configs', default=default_target_dir)
    parser.add_argument('--group', help='group to make configs')
    return parser.parse_args()


def _set_agent_config(group, instance):
    return {
        'workloads': [],
        'meta': {
            'topology': {
                'group': group.card.name,
                'version': group.parent.db.get_repo().get_current_tag() or 'trunk',
            },
            'instance': get_instance_name(group, instance),
        }
    }


def _format_cpu_resource(gencfg_units, host_power):
    return round(100. * gencfg_units / host_power, 3)


def _container_to_dict(container, host_power):
    json_data = container.as_dict()
    if 'cpu_limit' in container:
        json_data['cpu_limit'] = _format_cpu_resource(container.cpu_limit, host_power)
    if 'cpu_guarantee' in container:
        json_data['cpu_guarantee'] = _format_cpu_resource(container.cpu_guarantee, host_power)
    return json_data


def _get_workload_args(group, workload, instance):
    arguments = []

    for config_type in workload.configs:
        if all(map(lambda prop: prop in config_type, ('file_name_template', 'parameter'))):
            file_name = config_type.file_name_template.format(
                INSTANCE=get_instance_name(group, instance),
                WORKLOAD=workload.name
            )
            arguments.extend([config_type.parameter, file_name])

    return arguments


def get_instance_name(group, instance):
    if group.card.properties.mtn.use_mtn_in_config:
        return '{}:{}'.format(
            gaux.aux_hbf.generate_mtn_hostname(instance, group, ''),
            instance.port
        )
    return instance.short_name()


def get_config_name(group, instance):
    return '{}_agent.cfg'.format(get_instance_name(group, instance))


def generate_config(group, target_dir):
    for instance in group.get_kinda_busy_instances():
        agent_config = _set_agent_config(group, instance)

        for workload in group.card.workloads:
            arguments = _get_workload_args(group, workload, instance)

            workload_port = instance.port + workload.port_offset
            monitoring_wl_port = workload_port + workload.monitoring_port_offset if 'monitoring_port_offset' in workload else 0

            workload_spec = {
                'container': _container_to_dict(workload.container, instance.host.power),
                'port': workload_port,
                'monitoring_port': monitoring_wl_port,
                'arguments': arguments
            }
            agent_config['workloads'].append({
                'spec': workload_spec,
                'meta': {
                    'hash': hashlib.md5(json.dumps(workload_spec, sort_keys=True)).hexdigest(),
                    'name': workload.name,
                    'tags': workload.tags.as_dict(),
                }
            })

        agent_file = get_config_name(group, instance)

        with open(os.path.join(target_dir, agent_file), 'w') as fp:
            json.dump(agent_config, fp, indent=2, sort_keys=True)


def main():
    args = parse_args()

    if not os.path.exists(args.target_dir):
        os.makedirs(args.target_dir)

    if args.group:
        groups = [CURDB.groups.get_group(args.group)]
    else:
        groups = CURDB.groups.get_groups()

    for group in groups:
        if 'workloads' in group.card:
            generate_config(group, args.target_dir)


if __name__ == '__main__':
    main()
