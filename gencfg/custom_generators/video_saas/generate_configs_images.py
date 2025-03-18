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
from pprint import pprint


def parse_cmd():
    parser = argparse.ArgumentParser(description='Generate images saas configs.')
    parser.add_argument('-d', '--dest-dir', required=True, dest='dest_dir', help='Destination dir')
    parser.add_argument('-T', '--topology', dest='topology', default='online', help='Topology version')
    return parser.parse_args()


def get_templates_dir():
    return os.path.join(os.path.dirname(os.path.abspath(__file__)), 'templates_images/')


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


def patch_searchmap(service_data, template, generateDistributors=True, generateBackends=True):
    for service_name, data in service_data.iteritems():
        if data['queue_type'] == 'logbroker':
            continue
        if generateDistributors:
            template['services'][service_name]['distributors'] = " ".join(
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
                        template['services'][service_name]['replicas']['default'].append(backend)

    return template


def generate_indexerproxy_test_config(service_data):
    base_json = os.path.join(get_templates_dir(), 'indexerproxy_searchmap_test.json')
    indexerproxy_conf = json.loads(open(base_json, 'r').read())

    indexerproxy_conf = patch_searchmap(service_data, template=indexerproxy_conf, generateBackends=True)

    return json.dumps(indexerproxy_conf, sort_keys=True, indent=4)


def generate_indexerproxy_prod_config(service_data):
    base_json = os.path.join(get_templates_dir(), 'indexerproxy_searchmap_prod.json')
    indexerproxy_conf = json.loads(open(base_json, 'r').read())

    indexerproxy_conf = patch_searchmap(service_data, template=indexerproxy_conf, generateBackends=True)

    return json.dumps(indexerproxy_conf, sort_keys=True, indent=4)


def generate_searchproxy_config(service_data):
    base_json = os.path.join(get_templates_dir(), 'searchproxy_searchmap.json')
    searchproxy_conf = json.loads(open(base_json, 'r').read())

    searchproxy_conf = patch_searchmap(service_data, template=searchproxy_conf, generateDistributors=False)

    return json.dumps(searchproxy_conf, sort_keys=True, indent=4)


def generate_distributors_config(service_name, rtyconfig, data):
    render_data = {}
    render_data['replicas'] = []
    for loc, shards in data['distributors'].iteritems():
        replicas = {len(instances["instances"]) for _, instances in shards.iteritems()}

        # Ensure we have identical replicas count for each shard
        if len(replicas) > 1:
            raise Exception("Inconsistant replicas count in location")

        for i in range(replicas.pop()):
            instances = []
            for _, shard_data in shards.iteritems():
                instance = shard_data['instances'].pop()['instance'] + ":" + shard_data['shard_range_hex_formatted']
                instances.append(instance)
            render_data['replicas'].append((loc, instances))


    env = Environment(loader=FileSystemLoader(get_templates_dir()), extensions=["jinja2.ext.do", ])
    template = env.get_template("distributors.conf.tpl")
    return template.render(data=render_data)


def generate_configs(services, proxy_generators, dest_dir, topology):
    service_data = {}
    for service_name, service in services.iteritems():
        service_data[service_name] = {}
        for itype, members in service['groups'].iteritems():
            service_data[service_name][itype] = {}
            for location, group_name in members.iteritems():
                service_data[service_name][itype][location] = copy.deepcopy(get_shards(group_name, topology))

        for filename, generator in service['generators'].iteritems():
            config_path = os.path.join(dest_dir, filename)
            with open(config_path, 'w') as fh:
                fh.write(generator(service_name, {}, copy.deepcopy(service_data[service_name])))

        service_data[service_name]['queue_type'] = service['queue_type']

    for filename, proxy_generator in proxy_generators.iteritems():
        config_path = os.path.join(dest_dir, filename)
        with open(config_path, 'w') as fh:
            fh.write(proxy_generator(copy.deepcopy(service_data)))


def generate_prod_configs(dest_dir, topology):
    services = {
        'imagesultra': {
            'groups': {
                'distributors': {
                    'sas': 'SAS_IMGS_ULTRA_REFRESH_DISTRIBUTOR_NEW',
                    'man': 'MAN_IMGS_ULTRA_REFRESH_DISTRIBUTOR_NEW',
                    # 'vla': 'VLA_IMGS_ULTRA_REFRESH_DISTRIBUTOR_NEW',
                },
                'basesearch': {
                    'sas': 'SAS_IMGS_ULTRA_BASE',
                    'man': 'MAN_IMGS_ULTRA_BASE',
                    'vla': 'VLA_IMGS_ULTRA_BASE',
                }
            },
            'generators': {
                'ultra_distributors.conf': generate_distributors_config
            },
            'queue_type': 'logbroker',
        },
        'imagesquick': {
            'groups': {
                'distributors': {
                    'sas': 'SAS_IMGS_QUICK_REFRESH_DISTRIBUTOR_TEST',
                    'man': 'MAN_IMGS_QUICK_REFRESH_DISTRIBUTOR_TEST',
                    'vla': 'VLA_IMGS_QUICK_REFRESH_DISTRIBUTOR_TEST',
                },
                'basesearch': {
                }
            },
            'generators': {
                'quick_distributors.conf': generate_distributors_config
            },
            'queue_type': 'distributor',
        },
        'imagesacceptance': {
            'groups': {
                'distributors': {
                    'sas': 'SAS_IMGS_QUICK_REFRESH_DISTRIBUTOR_ACCEPTANCE',
                },
                'basesearch': {
                    'sas': 'SAS_RTQUICK_BASE',
                }
            },
            'generators': {
                'acceptance_distributors.conf': generate_distributors_config
            },
            'queue_type': 'distributor',
        },
        'imagesexperiment': {
            'groups': {
                'distributors': {
                    'sas': 'SAS_IMAGES_REFRESH_DISTRIBUTOR_PRISM',
                    'man': 'MAN_IMAGES_REFRESH_DISTRIBUTOR_PRISM',
                    'vla': 'VLA_IMAGES_REFRESH_DISTRIBUTOR_PRISM',
                },
                'basesearch': {
                    'sas': 'SAS_IMAGES_REFRESH_BASE_PRISM',
                    'man': 'MAN_IMAGES_REFRESH_BASE_PRISM',
                    'vla': 'VLA_IMAGES_REFRESH_BASE_PRISM'
                }
            },
            'generators': {
                'experiment_distributors.conf': generate_distributors_config
            },
            'queue_type': 'distributor',
        }
    }

    proxy_generators = {
        'images-saas-indexerproxy.json': generate_indexerproxy_prod_config,
        'images-saas-searchproxy.json': generate_searchproxy_config
    }

    generate_configs(services, proxy_generators, dest_dir, topology)


def generate_test_configs(dest_dir, topology):
    services = {
        'imagesultra': {
            'groups': {
                'distributors': {
                    'man': 'MAN_IMGS_ULTRA_REFRESH_DISTRIBUTOR_TEST',
                },
                'basesearch': {
                    'man': 'MAN_IMGS_ULTRA_BASE_TEST',
                }
            },
            'generators': {
                'ultra_distributors.conf-test': generate_distributors_config
            },
            'queue_type': 'logbroker',
        }
    }

    proxy_generators = {
        'images-saas-indexerproxy-test.json': generate_indexerproxy_test_config,
        'images-saas-searchproxy-test.json': generate_searchproxy_config
    }

    generate_configs(services, proxy_generators, dest_dir, topology)


def main():
    options = parse_cmd()
    if not os.path.isdir(options.dest_dir):
        raise Exception("Destination dir doesn't exist!")

    generate_prod_configs(options.dest_dir, options.topology)
    generate_test_configs(options.dest_dir, options.topology)

if __name__ == '__main__':
    main()
