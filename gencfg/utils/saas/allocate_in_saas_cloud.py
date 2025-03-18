#!/skynet/python/bin/python
import os
import sys
import functools
import argparse
import json
import logging

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))
import gencfg
from core.db import CURDB
import gaux.aux_volumes
from core.argparse.parser import ArgumentParserExt
import core.argparse.types as argparse_types
from utils.common.update_igroups import jsmain as update_igroups
from utils.common.manipulate_volumes import main_put as put_volumes
from utils.saas.collect_saas_hosts import get_host_info
from utils.saas.saas_byte_size import SaaSByteSize
from utils.saas.common import get_allocated_volume
from utils.saas.common import ALL_GEO, DEFAULT_IO_LIMITS
from utils.saas.common import gencfg_io_limits_from_json
from utils.saas.common import VolumesOptions


def get_parser():
    parser = ArgumentParserExt(
        description="Allocate group in SaaS cloud")

    parser.add_argument('-b', '--base-group-name', type=str, default=None, required=True,
                        help='Base name for groups, e. g. TODOSUGGEST')
    parser.add_argument('-t', '--tags', type=argparse_types.jsondict, default=None, required=True,
                        help='group tags as json dict, e. g. "{ "ctype":"prod", "itype":"balancer", "prj" : ["gencfg-group-service"], "metaprj" : "web"}')
    parser.add_argument('-il', '--io-limits', type=json.loads, required=False, default=DEFAULT_IO_LIMITS,
                        help='IO limits json, default: {}'.format(json.dumps(DEFAULT_IO_LIMITS)))
    parser.add_argument('-v', '--volumes', type=json.loads, default=None, required=True,
                        help='group volumes as json dict')
    parser.add_argument('-n', '--instance-number', type=int, required=True,
                        help='Target slot count per location')
    parser.add_argument('-c', '--slot-size-cpu', type=int, required=True,
                        help='CPU requirements in Kimkim power units per instance')
    parser.add_argument('-sc', '--max-instances-per-switch', type=int, required=False, default=1,
                        help='Max number of hosts in one switch')
    parser.add_argument('-nc', '--network-critical', action='store_true',
                        help='Mark group as network critical')
    parser.add_argument('-nm', '--network-macro', default=None,
                        help='Network macro, ex: _SAAS_STABLE_KV_BASE_NETS_')
    parser.add_argument('-m', '--slot-size-mem', type=int, required=True,
                        help='Memory requirements in GB per instance')
    parser.add_argument('-i', '--no-indexing', action='store_true',
                        help='Mark group as no-indexing')
    parser.add_argument('-T', '--template-group', type=argparse_types.group, default=None,
                        help='override default template group')
    parser.add_argument('-o', '--owners', dest="owners", type=argparse_types.comma_list, default=None,
                        help='override default group owners')

    parser.add_argument(
        '--geo', type=functools.partial(argparse_types.comma_list_ext, deny_repeated=True, allow_list=ALL_GEO),
        help='Optional. Comma separated list of locations if not all locations required'
    )
    parser.add_argument('-f', '--file', type=argparse.FileType('w'), default=sys.stdout,
                        help='File for result json')

    return parser


if __name__ == '__main__':
    success = True
    logging.basicConfig(level=logging.DEBUG)

    args = get_parser().parse_cmd()

    geo = [loc.upper() for loc in args.geo] if args.geo else ALL_GEO

    result_groups = []
    errors = {}
    volumes = [gaux.aux_volumes.TVolumeInfo.from_json(x) for x in args.volumes]
    req_ssd = get_allocated_volume(volumes)
    owners = args.owners if args.owners else ['yandex_search_tech_quality_robot_saas', 'saas-robot']

    for loc in geo:
        parent_group = '{}_SAAS_CLOUD'.format(loc)
        group = '{}_{}'.format(parent_group, args.base_group_name)

        if CURDB.groups.get_group(group, raise_notfound=False) is not None:
            logging.critical('Group %s already exists', group)
            success = False
            result_groups.append({
                'group': group,
                'error': 'GROUP_ALREADY_EXISTS',
                'geo': loc
            })
            continue

        hosts = get_host_info(CURDB.groups.get_group(parent_group), SaaSByteSize('{} Gb'.format(args.slot_size_mem)), args.slot_size_cpu,
                              req_ssd, args.no_indexing, args.network_critical, max_hosts_per_switch=args.max_instances_per_switch)[:args.instance_number]

        if len(hosts) < args.instance_number:
            logging.critical('Not enough hosts in %s. %d/%d', loc, len(hosts), args.instance_number)
            success = False
            result_groups.append({
                'group': group,
                'error': 'NOT_ENOUGH_HOSTS',
                'available_hosts': len(hosts),
                'required_hosts': args.instance_number,
                'geo': loc
            })
            continue

        if args.io_limits:
            io_limits = ['{}={}'.format('.'.join(k), v) for k, v in gencfg_io_limits_from_json(args.io_limits).items()]
        else:
            io_limits = []

        js_params = {
            'action': 'addgroup',
            'group': group,
            'parent_group': parent_group,
            'template_group': args.template_group or parent_group,
            'owners': owners,
            'tags': args.tags,
            'hosts': [h.host.name for h in hosts],
            'properties': ','.join([
                'reqs.instances.memory_guarantee={mem}Gb'.format(mem=args.slot_size_mem),
                'reqs.hosts.max_per_switch={sc}'.format(sc=args.max_instances_per_switch),
                'legacy.funcs.instancePower=exactly{power}'.format(power=args.slot_size_cpu),
            ] + io_limits)
        }
        if 'saas_no_indexing' in js_params['tags'].get('itag', []) or args.no_indexing:
            js_params['properties'] += ',reqs.instances.affinity_category=saas_no_indexing'
            itags = set(args.tags.get('itag', []))
            itags.add('saas_no_indexing')
            js_params['tags']['itag'] = list(itags)
        if args.network_critical:
            itags = set(args.tags.get('itag', []))
            itags.add('saas_network_critical')
            js_params['tags']['itag'] = list(itags)
        if args.network_macro:
            js_params['properties'] += ',properties.hbf_parent_macros={}'.format(args.network_macro)
        update_igroups(js_params)
        put_volumes(VolumesOptions(groups=[group, ], vs=args.volumes))
        js_params['json_volumes'] = args.volumes
        result_groups.append({k: v for k, v in js_params.items() if k != 'action'})

    args.file.write(json.dumps(result_groups))

    if not success:
        raise RuntimeError('Allocation failure')
