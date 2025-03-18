#!/skynet/python/bin/python
import os
import sys
import json
import time
import logging
import functools

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg

from argparse import ArgumentParser
from core.db import CURDB
from core.card.json_utils import fix_card_node_json_for_save


ALIASES = {
    'power': 'reqs.instances.power',
    'memory': 'reqs.instances.memory_guarantee',
    'hdd': 'reqs.instances.disk',
    'ssd': 'reqs.instances.ssd',
    'net': 'reqs.instances.net_guarantee',
    'port': 'reqs.instances.port',
    'min_power': 'reqs.shards.min_power',
    'volumes': 'reqs.volumes'
}


def work_time(func):
    @functools.wraps(func)
    def wrapper(*args, **kwargs):
        start = time.time()
        r_value = func(*args, **kwargs)
        logging.debug('WORKTIME {} {}s'.format(func.__name__, time.time() - start))
        return r_value
    return wrapper


def get_list_form_comma_list(comma_list):
    result_list = []

    if comma_list.startswith('#'):
        filename = comma_list[1:]
        with open(filename, 'r') as data:
            for line in data:
                result_list.append(line.strip())
    else:
        result_list = [x.strip() for x in comma_list.split(',')]

    return result_list


def get_group_by_name(group_name, log_on_error=True):
    try:
        return CURDB.groups.get_group(group_name)
    except Exception as e:
        if log_on_error:
            print(str(e))


def groups(values):
    all_groups = []

    for group_name in get_list_form_comma_list(values):
        group = get_group_by_name(group_name.strip())
        if not group:
            continue
        all_groups.append(group)

    return all_groups


def group_slaves(values):
    slaves = []

    for group_name in get_list_form_comma_list(values):
        group = get_group_by_name(group_name.strip())
        if not group:
            continue
        slaves.append(group)

        for slave in group.card.slaves:
            slaves.append(slave)

    return slaves


def fields(values):
    return [x.strip() for x in values.split(',')]


def save_to_dict(group):
    scheme = CURDB.groups.SCHEME

    jsoned = group.card.as_dict()
    fix_card_node_json_for_save(scheme, jsoned)
    jsoned['slaves'] = map(str, jsoned['slaves'])

    return json.loads(json.dumps(jsoned))


def get_field(group_dict, path):
    path = path.split('.')
    if len(path) == 1:
        return group_dict[path[0]]
    return get_field(group_dict[path[0]], '.'.join(path[1:]))


def parse_cmd():
    parser = ArgumentParser(description='Script to upload host info trunk data to mongo')

    parser.add_argument('-g', '--groups', type=groups, required=False,
                        help='Names of groups')
    parser.add_argument('-s', '--slaves', type=group_slaves, required=False,
                        help='Names of master group for convert to list slaves.')
    parser.add_argument('-f', '--fields', type=fields, required=False,
                        help='Fields to show.\nCARD - full group card.\ninsatnces - full instances info.\nhosts - full hosts info')
    parser.add_argument('-v', '--verbose', action='store_true', default=False,
                        help='Optional. Explain what is being done.')

    if len(sys.argv) == 1:

        sys.argv.append('-h')

    options = parser.parse_args()

    if options.fields is None:
        options.fields = ['CARD']

    for i in xrange(len(options.fields)):
        if options.fields[i] in ALIASES:
            options.fields[i] = ALIASES[options.fields[i]]

    return options


@work_time
def get_groups_cards(list_groups, list_path_fields):
    data = {}
    for group in list_groups:
        data[group.card.name] = {}
        group_data = save_to_dict(group)

        for field_path in list_path_fields:
            if any([field_path.startswith(x) for x in ('hosts', 'instances')]):
                continue

            if 'CARD' in list_path_fields:
                data[group.card.name] = save_to_dict(group)

            else:
                data[group.card.name][field_path] = get_field(group_data, field_path)

    for group in list_groups:
        instances_info = None
        if any([x.startswith('instances') for x in list_path_fields]):
            instances_info = [
                {'host': x.host.name, 'port': x.port, 'power': x.power}
                for x in group.get_instances()
            ]

        hosts_info = None
        if any([x.startswith('hosts') for x in list_path_fields]):
            hosts_info = [x.save_to_json_object() for x in group.getHosts()]

        for field_path in list_path_fields:
            if field_path == 'instances':
                data[group.card.name][field_path] = instances_info

            elif field_path == 'instances_count':
                data[group.card.name][field_path] = len(instances_info)

            elif field_path == 'instances_power':
                instances_power = 0
                for i, instance_data in enumerate(instances_info):
                    instances_power += instance_data['power']
                data[group.card.name][field_path] = instances_power / (len(instances_info) or 1)

            elif field_path.startswith('instances.'):
                if 'instances' not in data[group.card.name]:
                    data[group.card.name]['instances'] = [{} for _ in xrange(len(instances_info))]

                path = '.'.join(field_path.split('.')[1:])
                for i, instance_data in enumerate(instances_info):
                    data[group.card.name]['instances'][i][path] = get_field(instance_data, path)

            if field_path == 'hosts':
                data[group.card.name][field_path] = hosts_info

            elif field_path == 'hosts_count':
                data[group.card.name][field_path] = len(hosts_info)

            elif field_path.startswith('hosts.'):
                if 'hosts' not in data[group.card.name]:
                    data[group.card.name]['hosts'] = [{} for _ in xrange(len(hosts_info))]

                path = '.'.join(field_path.split('.')[1:])
                for i, host_data in enumerate(hosts_info):
                    data[group.card.name]['hosts'][i][path] = get_field(host_data, path)

    return data


def main():
    options = parse_cmd()

    list_groups = []
    if options.groups is not None:
        list_groups.extend(options.groups)
    if options.slaves is not None:
        list_groups.extend(options.slaves)

    data = get_groups_cards(list_groups, options.fields)
    print(json.dumps(data, indent=4))

    return 0


if __name__ == '__main__':
    ret_value = main()
    sys.exit(ret_value)
