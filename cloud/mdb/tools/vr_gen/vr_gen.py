#!/usr/bin/env python
"""
Valid-resources table generator

Run this script in metadb private pillar dir
"""

import argparse
import json
import logging
import os
import re
import yaml

from collections import namedtuple
from copy import deepcopy
from humanfriendly import parse_size

log = logging.getLogger(__name__)

FilenamesMap = namedtuple('FilenamesMap', 'source, disk_types, geo, flavor, output')


# https://st.yandex-team.ru/MDB-16682#62e287f86ece3b4b82425c25
def ensure_string_keys(geo_availability):
    geo_availability = {str(k): v for k, v in geo_availability.items()}
    return geo_availability


def get_old_filenames(env: str) -> FilenamesMap:
    return FilenamesMap(
        source='vr_gen.conf',
        disk_types='disk_types.sls',
        geo='geos.sls',
        flavor='flavors.sls',
        output=env + '.json',
    )


K8S_FILENAMES = FilenamesMap(
    source='vr_gen.yaml',
    disk_types='disk_type.yaml',
    geo='geo.yaml',
    flavor='flavor.yaml',
    output='valid_resources.json',
)


def generate_flavor_list(pool, key_criterion, inclusion_subset=None, exclusion_rules=None):
    """
    Generate a list of applicable flavors based on a set of regexen.
    """

    def ensure_pattern_limits(pattern_string):
        """Ensure patterns are limited to full string until noted otherwise"""
        if not pattern_string:
            return ''
        # Beginning of the string
        if pattern_string[0] != '^':
            pattern_string = '^' + pattern_string
        # End of the string
        if pattern_string[-1] != '$':
            pattern_string = pattern_string + '$'
        return pattern_string

    include_regexen = []
    for pattern in inclusion_subset:
        include_regexen.append(re.compile(ensure_pattern_limits(pattern)))

    flavors_matched = set()
    for regex in include_regexen:
        flavors_matched.update(filter(regex.search, (x[key_criterion] for x in pool)))
    result = []
    for candidate in pool:
        if candidate[key_criterion] in flavors_matched:
            excluded_by = None
            if exclusion_rules:
                for rule_name, exclude_rule in exclusion_rules.items():
                    for key, value in exclude_rule.items():
                        if candidate[key] == value:
                            excluded_by = (rule_name, key, value)
                            break
            if excluded_by is not None:
                log.info('Excluded element %s by rule %s', candidate['name'], excluded_by[0])
                continue
            result.append(candidate)

    return result


def get_disk_type_map(base_directory, is_k8s, filenames):
    """
    Read disk types from pillar file
    """
    file = filenames.disk_types
    with open(os.path.join(base_directory, file)) as disk_types_file:
        disk_types = yaml.load(disk_types_file, Loader=yaml.FullLoader)
    if not is_k8s:
        disk_types = disk_types['data']['dbaas_metadb']['disk_type_ids']
    return {x['disk_type_ext_id']: x['disk_type_id'] for x in disk_types}


def get_geo_id_map(base_directory, is_k8s, filenames):
    """
    Read geo from pillar file
    """
    file = filenames.geo
    with open(os.path.join(base_directory, file)) as geos_file:
        geos = yaml.load(geos_file, Loader=yaml.FullLoader)
    if not is_k8s:
        geos = geos['data']['dbaas_metadb']['geos']
    return {x['name']: x['geo_id'] for x in geos}


def get_flavors(base_directory, is_k8s, filenames):
    """
    Read flavors from pillar file
    """
    file = filenames.flavor
    with open(os.path.join(base_directory, file)) as flavors_file:
        flavors = yaml.load(flavors_file, Loader=yaml.FullLoader)
    if not is_k8s:
        flavors = flavors['data']['dbaas_metadb']['flavors']
    # Return only visible flavors
    return [flavor for flavor in flavors if flavor.get('visible', True)]


def get_available_geos(geo_availability, flavor, include_filter=None, exclude_filter=None):
    """
    Returns geos flavor is available in
    """
    geo_availability = ensure_string_keys(geo_availability)
    geos = geo_availability.get(str(flavor['generation']), [])
    geos = geo_availability.get(flavor['platform_id'], geos)
    if include_filter:
        geos = [x for x in geos if x in include_filter]
    if exclude_filter:
        geos = [x for x in geos if x not in exclude_filter]
    return geos


def range_for_disk_sizes(disk_sizes):
    """
    >>> list(range_for_disk_sizes(dict(min='1B', max='5B', step='2B')))
    [1, 3]
    >>> list(range_for_disk_sizes(dict(min='1B', max='6B', step='2B')))
    [1, 3, 5, 6]
    >>> list(range_for_disk_sizes(dict(min='2B', max='5B', step='2B')))
    [2, 4]
    >>> list(range_for_disk_sizes(dict(min='2B', max='6B', step='2B')))
    [2, 4, 6]
    """
    if not all(key in disk_sizes for key in ('min', 'step', 'max')):
        raise RuntimeError('not all keys are present')
    start = parse_size(disk_sizes['min'], binary=True)
    stop = parse_size(disk_sizes['max'], binary=True)
    step = parse_size(disk_sizes['step'], binary=True)
    last = None
    for size in range(start, stop, step):
        last = size
        yield size
    if last != stop and stop % step == 0:
        yield stop


def gen_cluster_type(context, geo_availability, name, config, flavor_type_override, featured_geo):
    """
    Generate valid resources for given cluster type
    """
    ret = []
    for role in config['roles']:
        role_config = deepcopy(config)
        if isinstance(role, dict):
            role_config.update(role)
            role_name = role['role']
        else:
            role_name = role
        for disk_type in role_config['disk_types']:
            disk_type_config = deepcopy(role_config)
            disk_type_config.update(disk_type)
            for flag_name in list(disk_type_config.get('feature_flags', [None])):
                dsizes = disk_type_config['disk_sizes']
                if isinstance(dsizes, dict):
                    disk_type_config['disk_sizes'] = list(range_for_disk_sizes(dsizes))
                disk_type_config['feature_flag'] = flag_name
                if 'flavors' in disk_type_config:
                    gen_flavors = generate_flavor_list(
                        pool=context['flavors'],
                        key_criterion='name',
                        inclusion_subset=disk_type_config['flavors'],
                        exclusion_rules=disk_type_config.get('exclude_flavors'),
                    )
                else:
                    gen_flavors = generate_flavor_list(
                        pool=context['flavors'],
                        key_criterion='type',
                        inclusion_subset=disk_type_config['flavor_types'],
                        exclusion_rules=disk_type_config.get('exclude_flavors'),
                    )
                for flavor in gen_flavors:
                    if disk_type_config.get('platforms'):
                        if flavor['platform_id'] not in disk_type_config['platforms']:
                            continue
                    geos = get_available_geos(
                        geo_availability, flavor, disk_type_config.get('geos'), disk_type_config.get('exclude_geos')
                    )
                    for geo in geos:
                        combination = dict(
                            id=context['id'],
                            cluster_type=name,
                            role=role_name,
                            flavor=flavor['id'],
                            disk_type_id=context['disk_type_ids'][disk_type_config['type']],
                            geo_id=context['geo_ids'][geo],
                            disk_size_range=dict(int8range=disk_type_config['disk_size_range'][:])
                            if disk_type_config['disk_size_range']
                            else None,
                            disk_sizes=disk_type_config['disk_sizes'][:] if disk_type_config['disk_sizes'] else None,
                            min_hosts=disk_type_config['min_hosts'],
                            max_hosts=disk_type_config['max_hosts'],
                            feature_flag=None,
                        )
                        if disk_type_config['feature_flag'] is not None:
                            combination['feature_flag'] = disk_type_config['feature_flag']
                        if geo in featured_geo:
                            if combination['feature_flag'] is not None:
                                # ignore featured_geo with feature_flag
                                continue
                            else:
                                combination['feature_flag'] = featured_geo[geo]
                        combination.update(flavor_type_override.get(flavor['type']) or {})
                        ret.append(combination)
                        context['id'] += 1

    return ret


def main():
    parser = argparse.ArgumentParser()

    parser.add_argument('base_dir', help='base directory', type=str, nargs='?', default=os.getcwd())
    parser.add_argument('out_dir', help='output directory', type=str, nargs='?', default=os.getcwd())
    parser.add_argument('env', help='environment', type=str, nargs='?', default='unknown')
    parser.add_argument('--is-k8s', help='used in k8s', action='store_true')

    args = parser.parse_args()

    filenames = K8S_FILENAMES if args.is_k8s else get_old_filenames(args.env)

    source_file = filenames.source

    with open(os.path.join(args.base_dir, source_file)) as conf_file:
        config = yaml.load(conf_file, Loader=yaml.FullLoader)
    geo_availability = config['geo_availability']
    flavor_type_override = config['flavor_type_override']
    featured_geo = config.get('featured_geo', {})
    data = []
    context = {
        'id': 1,
        'flavors': get_flavors(args.base_dir, args.is_k8s, filenames),
        'disk_type_ids': get_disk_type_map(args.base_dir, args.is_k8s, filenames),
        'geo_ids': get_geo_id_map(args.base_dir, args.is_k8s, filenames),
        'flavor_type_override': flavor_type_override,
    }
    for cluster_type_name, cluster_type_config in config['cluster_types'].items():
        data += gen_cluster_type(
            context, geo_availability, cluster_type_name, cluster_type_config, flavor_type_override, featured_geo
        )

    out_file = filenames.output
    with open(os.path.join(args.out_dir, out_file), 'w') as out:
        out.write(json.dumps(data, indent=4, sort_keys=True))


if __name__ == '__main__':
    main()
