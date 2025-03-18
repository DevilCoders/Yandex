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

class ETemplateName(object):
    VIDEO_REFRESH = 'video_refresh'
    VIDEO_DEV = 'video_refresh_dev'
    VIDEO_RT_DEV = 'video_rt_dev_base'
    ALL = [VIDEO_REFRESH, VIDEO_DEV, VIDEO_RT_DEV]

def get_parser():
    parser = argparse.ArgumentParser(description='Generate video saas configs.')
    parser.add_argument('-d', '--dest-dir', required=True, dest='dest_dir', help='Destination dir')
    parser.add_argument('-T', '--topology', dest='topology', default='online', help='Topology version')
    parser.add_argument('-t', '--template-name', default = ETemplateName.VIDEO_REFRESH,
            choices = ETemplateName.ALL,
            help='Optional. Template name: one of {} ({} by default)'.format(','.join(ETemplateName.ALL), ETemplateName.VIDEO_REFRESH)
    )

    return parser


def get_templates_dir():
    return os.path.join(os.path.dirname(os.path.abspath(__file__)), 'templates/')


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


def patch_searchmap(basesearches, template, service, generateBackends=True):
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
                    template['services'][service]['replicas']['default'].append(backend)

    return template


def generate_searchproxy_config(data, params):
    base_json = os.path.join(get_templates_dir(), 'searchproxy_searchmap.json')
    searchproxy_conf = json.loads(open(base_json, 'r').read())

    basesearches = data.get('basesearch', {})
    searchproxy_conf = patch_searchmap(basesearches, template=searchproxy_conf, service='videodirect')

    return json.dumps(searchproxy_conf, sort_keys=True, indent=4)


def generate_rtyserver_config(data, params):
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

    env = Environment(loader=FileSystemLoader(get_templates_dir()), extensions=["jinja2.ext.do", ])
    template = env.get_template(params['base_tpl'])
    return template.render(data=render_data)


def main(options):
    #TODO: (yrum) convert this to yaml
    if options.template_name == ETemplateName.VIDEO_REFRESH:
        groups = {
            'distributors': {
            },
            'basesearch': {
                'man': 'MAN_VIDEO_QUICK_BASE_NEW',
                'sas': 'SAS_VIDEO_QUICK_BASE_NEW',
                'vla': 'VLA_VIDEO_QUICK_BASE_NEW',
            }
        }
        params = {
            'distributor_replica_ids' : {
                'sas' : [1,2,3],
                'man' : [7,8,9],
                'vla' : [10,11,12]
            },
            'base_tpl': 'rtyserver.conf.tpl',
            'generate_videort' : True
        }
        generators = {
            "video-saas-searchproxy.json": generate_searchproxy_config,
            "video-saas-rtyserver.conf": generate_rtyserver_config
        }

    elif options.template_name == ETemplateName.VIDEO_RT_DEV:
        groups = {
            'distributors' : {
            },
            'basesearch' : {
                'vla': 'VLA_SAAS_VIDEO_REFRESH_RT_DEV_BASE',
            },
        }
        params = {
            'base_tpl': 'rtyserver-rt.conf.tpl',
            'generate_videort' : True
        }
        generators = {
            "video-saas-rt-dev-rtyserver.conf": generate_rtyserver_config
        }
    elif options.template_name == ETemplateName.VIDEO_DEV:
        groups = {
            'distributors' : {
            },
            'basesearch' : {
                'all': 'SAS_SAAS_VIDEO_REFRESH_3DAY_DEV_BASE+VLA_SAAS_VIDEO_REFRESH_3DAY_DEV_BASE',
            },
            'img-distributors' : {
                'all': 'MAN_IMGS_ULTRA_REFRESH_DISTRIBUTOR_TEST',
            }
        }
        params = {
            'base_tpl': 'rtyserver-dev.conf.tpl',
            'generate_videort' : True
        }
        generators = {
            "video-saas-dev-rtyserver.conf": generate_rtyserver_config,
            "video-saas-dev-searchproxy.json": generate_searchproxy_config,
        }
    else:
        raise Exception('Unknown template name <{}>'.format(options.template_name))

    if not os.path.isdir(options.dest_dir):
        raise Exception("Destination dir doesn't exist!")

    data = {}
    for group_type, members in groups.iteritems():
        data[group_type] = {}
        for location, group_name in members.iteritems():
            data[group_type][location] = get_shards(group_name, options.topology)

    for filename, generator in generators.iteritems():
        print filename, generator
        config_path = os.path.join(options.dest_dir, filename)
        with open(config_path, 'w') as fh:
            fh.write(generator(data, params))


if __name__ == '__main__':
    options = get_parser().parse_args()

    main(options)


