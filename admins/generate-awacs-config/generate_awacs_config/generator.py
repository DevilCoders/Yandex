import requests
import sys
import json
import os
import logging
import logging.config
import stat
import time
from collections import defaultdict
from jinja2 import Template
from yaml import load, dump
from pkg_resources import resource_string
from copy import deepcopy
from .merge import merge_dicts
from .l3tt import L3TT
from .nanny import Nanny
from .gencfg import Gencfg


def resolve_conductor(group: str) -> dict:
    """
    Resolve a group in conductor

    Returns a map { dc -> [ host, host ] }
    """
    r = requests.get('https://c.yandex-team.ru/api/groups2hosts/{}?format=json&fields=fqdn,root_datacenter_name'.format(group))
    if r.status_code != 200:
        raise RuntimeError('Error resolving conductor')
    data = json.loads(r.text)
    result = defaultdict(list)

    for item in data:
        result[item['root_datacenter_name']].append({'fqdn': item['fqdn'], 'weight': 1})

    return result


class Generator:
    def __init__(self):
        logging.config.dictConfig(load(resource_string(__name__, 'logging.yaml')))
        config = load(open(os.path.expanduser('~/.runtime.yaml')))
        token = config['token']

        self.log = logging.getLogger(self.__class__.__name__)

        self.l3tt = L3TT(token)
        self.nanny = Nanny(token)
        self.gencfg = Gencfg(token)

        self.data = {
            'defaults':  load(resource_string(__name__, 'templates/defaults.yaml').decode('utf-8')),
            'upstream':  Template(resource_string(__name__, 'templates/upstream.yaml').decode('utf-8')),
            'namespace': Template(resource_string(__name__, 'templates/namespace.yaml').decode('utf-8')),
            'balancer':  Template(resource_string(__name__, 'templates/balancer.yaml').decode('utf-8')),
        }

    def generate_slb_configs(self, conf: dict, name: str, gencfg: dict, nanny: dict, dc_list: set, cache: dict):
        """ Generate configs for one datacenter """
        self.log.info('Generating configs for %s', name)
        os.makedirs('backends', exist_ok=True)
        os.makedirs('balancers', exist_ok=True)
        os.makedirs('upstreams', exist_ok=True)

        project = conf['global']['project']
        default_state = merge_dicts(self.data['defaults']['default'], self.data['defaults'][project])
        global_state = {}
        for key, value in conf['global'].items():
            global_state[key] = value

        new_state = merge_dicts(default_state, global_state)
        namespace_data = self.data['namespace'].render(new_state) + '\n'
        with open('namespace.yml', 'w') as f:
            f.write(namespace_data)

        for dc in dc_list:
            global_state['service'] = nanny[dc]
            global_state['dc'] = dc

            if gencfg[dc] in cache['ports']:
                global_state['port'] = cache['ports'][gencfg[dc]]
            else:
                global_state['port'] = self.gencfg.get_group_port(gencfg[dc])
                cache['ports'][gencfg[dc]] = global_state['port']

            new_state = merge_dicts(default_state, global_state)

            balancer_data = self.data['balancer'].render(new_state) + '\n'

            with open('balancers/{name}_{dc}.yml'.format(dc=dc, name=name), 'w') as f:
                f.write(balancer_data)

        for section, state_diff in conf.items():

            # skip special sections
            if section in ('default', 'global'):
                continue

            self.log.info('Generating upstream %s', section)

            state = merge_dicts(default_state, state_diff)
            if 'group' in state:
                state['mode'] = 'conductor'
                conductor_info = resolve_conductor(state['group'])
                if len(conductor_info.keys()) != len(dc_list) and ('external' not in state or not state['external']):
                    extra_dcs = set(conductor_info.keys()) - dc_list
                    self.log.warning('Extra DCs found: {}'.format(extra_dcs))

                    base_weight = len(dc_list)

                    result = {}
                    for dc in dc_list:
                        result[dc] = deepcopy(conductor_info[dc])
                        if not conductor_info[dc]:
                            result = deepcopy(conductor_info)
                        else:
                            for host in result[dc]:
                                host['weight'] = base_weight
                            for extra_dc in extra_dcs:
                                result[dc] += deepcopy(conductor_info[extra_dc])

                        result[dc] = sorted(result[dc], key=lambda x: x['fqdn'])

                    state['dcs'] = result
                else:
                    state['dcs'] = conductor_info

            elif 'gencfg' in state:
                state['mode'] = 'gencfg'
                state['dcs'] = {}

                gencfg_state = state['gencfg']
                for gencfg_dc in gencfg_state['dcs']:
                    gencfg_name = gencfg_dc.upper() + '_' + gencfg_state['name']
                    state['dcs'][gencfg_dc] = {"name": gencfg_name, "tag": gencfg_state['tag']}
                    if 'port' in gencfg_state:
                        state['dcs'][gencfg_dc]['port'] = gencfg_state['port']
                    if 'mtn' in gencfg_state:
                        state['dcs'][gencfg_dc]['mtn'] = gencfg_state['mtn']

            if 'default' in conf:
                for item in conf['default']:
                    if item not in state:
                        state[item] = conf['default'][item]

            section_data = self.data['upstream'].render(state) + '\n'
            with open('upstreams/{section}'.format(section=section), 'w') as f:
                f.write(section_data)

    @staticmethod
    def _extract_datacenters(items: list, from_right: bool) -> dict:
        """ Extract datacenters and a prefix from the list """
        """ Returns { str[dc] -> str[name] } """
        result = {}
        for item in items:
            if from_right:
                dc = item.rsplit('_')[-1]
            else:
                dc = item.split('_')[0]
                if dc == 'MSK':
                    dc = item.split('_')[1]
            result[dc.lower()] = item
        return result

    def run(self):
        """ Generate the balancer configs from templates """

        conf = load(open('config.yaml'))
        name = conf['global']['name']

        cache_file = '/tmp/generate-awacs-config/{name}.yaml'.format(name=name)
        using_cache = False
        try:
            cache_stat = os.stat(cache_file)
            mtime = cache_stat[stat.ST_MTIME]
            if mtime + 600 > time.time():
                cache = load(open(cache_file))
                using_cache = True
        except FileNotFoundError:
            cache = None

        if using_cache:
            self.log.info('Using cache file: {}'.format(cache_file))
            gencfg = cache['gencfg']
            nanny = cache['nanny']
        else:
            # extract l3tt info
            if 'force' in conf['global'] and 'l3' in conf['global']['force']:
                gencfg_groups = conf['global']['force']['l3']
            else:
                gencfg_groups = self.l3tt.find_service_groups(name)
            gencfg = self._extract_datacenters(gencfg_groups, False)

            # extract nanny info
            services = self.nanny.find_services_with_groups(gencfg_groups)
            nanny = self._extract_datacenters(services, True)

            # dc sanity check
            if sorted(gencfg.keys()) != sorted(nanny.keys()):
                raise RuntimeError('Gencfg and services have different DCs')

            cache = {
                'gencfg': gencfg,
                'nanny': nanny,
                'ports': {},
            }
        dc_list = set(gencfg.keys())
        self.generate_slb_configs(conf, name, gencfg, nanny, dc_list, cache)

        if not using_cache:
            os.makedirs('/tmp/generate-awacs-config', exist_ok=True)
            dump(cache, open(cache_file, 'w'))
