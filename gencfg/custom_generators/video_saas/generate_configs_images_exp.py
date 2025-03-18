#!/skynet/python/bin/python

import os
import sys
import json
import argparse
import copy
from jinja2 import Environment, FileSystemLoader

sys.path.append(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))  # add gencfg root to path
sys.path.append(os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'balancer_gencfg' ))  # add balancer_gencfg root to path

import src.utils as Utils

def parse_cmd():
    parser = argparse.ArgumentParser(description='Generate images saas configs.')
    parser.add_argument('-d', '--dest-dir', required=True, dest='dest_dir', help='Destination dir')
    parser.add_argument('-T', '--topology', dest='topology', default='online', help='Topology version')
    return parser.parse_args()


def get_templates_dir():
    return os.path.join(os.path.dirname(os.path.abspath(__file__)), 'templates_images_exp/')


def get_shards(group_name, topology, shard_range_max=65533):
    intlookup = Utils.ErsatzIntlookup(group_name, topology)
    shards = intlookup.get_shards_count()
    result = {}
    for shard_id in range(shards):
        shard_low = shard_range_max * shard_id / shards
        shard_upper = shard_range_max * (shard_id + 1) / shards - 1 if (shard_id + 1) < shards else shard_range_max
        instances = intlookup.get_base_instances_for_shard(shard_id)
        result[shard_id] = {
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
        }
    return result


def patch_searchmap(data, template, service, generateDistributors=True, generateBackends=True):
    if generateDistributors:
        template['services'][service]['distributors'] = " ".join(
            ["%s:%s" % (instance['instance'], shard_data['shard_range_hex_formatted'])
             for geo, shards in data['distributors'].iteritems()
             for shard, shard_data in shards.iteritems()
             for instance in shard_data['instances']
             ])
    if generateBackends:
        for geo, shards in data['basesearch'].iteritems():
            for shard, shard_data in shards.iteritems():
                for instance in shard_data['instances']:
                    backend = {
                        'host': instance['fqdn'],
                        'search_port': instance['port'],
                        'shard_min': shard_data['shard_range'][0],
                        'shard_max': shard_data['shard_range'][1],
                    }
                    template['services'][service]['replicas']['default'].append(backend)

    return template


def generate_indexerproxy_config(data):
    base_json = os.path.join(get_templates_dir(), 'indexerproxy_searchmap_test.json')
    indexerproxy_conf = json.loads(open(base_json, 'r').read())

    indexerproxy_conf = patch_searchmap(data, template=indexerproxy_conf, service='imagesultra', generateBackends=True)

    return json.dumps(indexerproxy_conf, sort_keys=True, indent=4)


def generate_searchproxy_config(data):
    base_json = os.path.join(get_templates_dir(), 'searchproxy_searchmap.json')
    searchproxy_conf = json.loads(open(base_json, 'r').read())

    searchproxy_conf = patch_searchmap(data, template=searchproxy_conf, service='imagesultra',
                                       generateDistributors=False)

    return json.dumps(searchproxy_conf, sort_keys=True, indent=4)


def generate_rtyserver_config(data):
    render_data = []
    for loc, shards in data['distributors'].iteritems():
        replicas = {len(instances) for _, instances in shards.iteritems()}
        # Ensure we have identical replicas count for each shard
        if len(replicas) > 1:
            raise Exception("Inconsistant replicas count in location")

        for i in range(replicas.pop()):
            instances = []
            for _, shard_data in shards.iteritems():
                instance = shard_data['instances'].pop()['instance'] + ":" + shard_data['shard_range_hex_formatted']
                instances.append(instance)
            render_data.append((loc, instances))

    env = Environment(loader=FileSystemLoader(get_templates_dir()), extensions=["jinja2.ext.do", ])
    template = env.get_template("rtyserver.conf.tpl")
    return template.render(data=render_data)

def generate_configs(groups, generators, dest_dir, topology):
    data = {}
    for itype, members in groups.iteritems():
        data[itype] = {}
        for location, group_name in members.iteritems():
            data[itype][location] = copy.deepcopy(get_shards(group_name, topology))

    for filename, generator in generators.iteritems():
        config_path = os.path.join(dest_dir, filename)
        with open(config_path, 'w') as fh:
            fh.write(generator(copy.deepcopy(data)))

def generate_prod_configs(dest_dir, topology):
    groups = {'distributors': {
        'sas': 'SAS_IMAGES_REFRESH_DISTRIBUTOR_PRISM',
        'man': 'MAN_IMAGES_REFRESH_DISTRIBUTOR_PRISM',
        'vla': 'VLA_IMAGES_REFRESH_DISTRIBUTOR_PRISM',
    },
        'basesearch': {
            'sas':  'SAS_IMAGES_REFRESH_BASE_PRISM',
            'man':  'MAN_IMAGES_REFRESH_BASE_PRISM',
            'vla':  'VLA_IMAGES_REFRESH_BASE_PRISM',
        }
    }
    generators = {"images-saas-indexerproxy-exp.json": generate_indexerproxy_config,
                  "images-saas-searchproxy-exp.json": generate_searchproxy_config,
                  "images-saas-rtyserver-exp.conf": generate_rtyserver_config
                  }

    generate_configs(groups, generators, dest_dir, topology)


def main():
    options = parse_cmd()
    if not os.path.isdir(options.dest_dir):
        raise Exception("Destination dir doesn't exist!")

    generate_prod_configs(options.dest_dir, options.topology)


if __name__ == '__main__':
    main()
