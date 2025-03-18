#!/skynet/python/bin/python

import os
import sys
import json
from jinja2 import Environment, FileSystemLoader
import json
import copy
import argparse

sys.path.append(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))  # add gencfg root to path
sys.path.append(os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'balancer_gencfg' ))  # add balancer_gencfg root to path

import src.utils as Utils


def get_shards(group_name_or_list, topology, shard_range_max=65533):
   # result = {}
    result = []
    intlookups = [ Utils.ErsatzIntlookup(group_name, topology) for group_name in group_name_or_list.split('+') ]
    shards_counts = [ intlookup.get_shards_count() for intlookup in intlookups ]
    if len(set(shards_counts)) > 1:
        raise ValueError("Inconsistent shards in {}".format(group_name_or_list))
    if len(shards_counts) == 0:
        return result

    shards = shards_counts[0]
    for intlookup in intlookups:
        for shard_id in range(shards):
            shard_low = shard_range_max * shard_id / shards
            shard_upper = shard_range_max * (shard_id + 1) / shards - 1 if (shard_id + 1) < shards else shard_range_max
            instances = intlookup.get_base_instances_for_shard(shard_id)
            result.append({
                'shard_range': (shard_low, shard_upper),
                'shard_range_hex_formatted': "%s:%s" % (format(shard_low, '02X'), format(shard_upper, '02X'))
                if shard_id < shards - 1 else
                format(shard_low, '02X'),
                'instances': [
                    {
                        'instance': x.name(),
                        'fqdn': x.get_host_name(),
                        'port': x.port,
                    } for x in sorted(instances, key=lambda x: (x.name(), x.port))
                    ]
            })

    return result


def patch_searchmap(distributors, basesearches, template, service, generateDistributors=True, generateBackends=True):
    if generateDistributors:
        template['services'].setdefault(service, {})['distributors'] = " ".join(
             ["%s:%s" % (instance['instance'], shard_data['shard_range_hex_formatted'])
             for geo, shards in distributors.iteritems()
             for shard_data in shards
             for instance in shard_data['instances']
             ])
    if generateBackends:
        for geo, shards in basesearches.iteritems():
            for shard_data in shards:
                for instance in shard_data['instances']:
                    backend = {
                        'host': instance['fqdn'],
                        'search_port': instance['port'],
                        'shard_min': shard_data['shard_range'][0],
                        'shard_max': shard_data['shard_range'][1],
                    }
                    template['services'].setdefault(service, {}).setdefault('replicas', {}).setdefault('default', []).append(backend)

    return template


def load_services_data(services, topology):
    service_data = {}
    for service_name, service in services.iteritems():
        service_data[service_name] = {}
        for itype, members in service['groups'].iteritems():
            service_data[service_name][itype] = {}
            for location, group_name in members.iteritems():
                service_data[service_name][itype][location] = copy.deepcopy(get_shards(group_name, topology))
    return service_data


def generate_proxy_config(template_path, data, services):
    proxy_conf = json.loads(open(template_path, 'r').read())
    for service, genDistr, genBackends in services:
        distributors, basesearches = data[service].get('distributors', {}), data[service].get('basesearch', {})
        proxy_conf = patch_searchmap(distributors, basesearches, template=proxy_conf, service=service, generateDistributors=genDistr, generateBackends=genBackends)

    return json.dumps(proxy_conf, sort_keys=True, indent=4)


def generate_rtyserver_config(templates_dir, data, params):
    render_data = []
    loc_order = ['sas', 'msk', 'man', 'vla'] # Fixed order to preserve ReplicaId
    predefined_ids = params.get('distributor_replica_ids', None)
    auto_id_last = 0

    for loc in loc_order:
        shards = data['distributors'].get(loc)
        if not shards:
            continue # location is not present

        # Ensure we have identical replicas count for each shard
        replicas = {len(shard_data['instances']) for shard_data in shards}
        if len(replicas) != 1:
            print shards
            raise Exception("Inconsistent replicas count in location " + loc)
        replicas_count = replicas.pop()

        # Generate replica ids for distributors
        if predefined_ids:
            replica_ids = predefined_ids.get(loc);
            if not replica_ids:
                raise Exception("predefined_ids is not defined for {} location".format(loc))
            if replicas_count != len(replica_ids):
                raise Exception("Number of replicas has probably changed, please change 'predefined_ids' in generate_configs.py accordingly")
        else:
            replica_ids = range(auto_id_last + 1, auto_id_last + replicas_count + 1)
            auto_id_last += replicas_count

        shards_copy=copy.deepcopy(shards)

        # make render_data
        for i in range(replicas_count):
            instances = []
            replica_id = replica_ids.pop(0)

            for shard_data in shards_copy:
                instance = shard_data['instances'].pop()['instance'] + ":" + shard_data['shard_range_hex_formatted']
                instances.append(instance)
            render_data.append((loc, instances, replica_id))

    # render the template

    env = Environment(loader=FileSystemLoader(templates_dir), extensions=["jinja2.ext.do", ])
    template = env.get_template(params['base_tpl'])
    return template.render(data=render_data)
