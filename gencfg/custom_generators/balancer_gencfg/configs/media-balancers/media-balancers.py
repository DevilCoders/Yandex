#!/skynet/python/bin/python

import argparse
import os
import sys
import json
import yaml
import collections
import copy
import math
import requests
import time

sys.path.append(
    os.path.dirname(
        os.path.dirname(
            os.path.dirname(
                os.path.abspath(__file__)))))

# add venv packages path to sys.path
if 1:
    root = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..')
    site_dir = os.path.join(
        root,
        'venv',
        'venv',
        'lib',
        'python2.7',
        'site-packages')
    sys.path.insert(1, site_dir)

    # one more hack for processing .pth files from virtualenv site-package
    import site

    site.addsitedir(site_dir)

import src.modules as Modules
import src.macroses as Macroses
import src.constants as Constants
import src.utils as Utils
from src.generator import generate
from collections import OrderedDict
from src.lua_globals import LuaGlobal, lua_global_to_string
from src.lua_globals import LuaAnonymousKey
from src.lua_globals import LuaFuncCall


TRANSPORTS = [
    "curdb",
    "api",
]

error_document_404 = (
    Modules.ErrorDocument, {
        'status': 404,
        'content': 'Content not found\\n'
    })

error_document_502 = (
    Modules.ErrorDocument, {
        'status': 502,
        'content': 'Bad gateway\\n'
    })

COMMON_REGEXP_LOCATIONS = [
    ('default', {}, [
        error_document_404,
    ]),
    ('robots', {'match_fsm': OrderedDict([('URI', '/robots.txt')])}, [
        (Modules.ErrorDocument, {'status': 200, 'content': 'User-agent: *\\nDisallow: /\\n'}),
    ]),
    ('root', {'match_fsm': OrderedDict([('path', '/')])}, [
        (Modules.ResponseHeaders, {'create': OrderedDict([('Location', 'http://www.yandex.ru/')])}),
        (Modules.ErrorDocument, {'status': 302, 'content': '<title>302 Moved Temporarily</title></head>\\n'
                                                           '<body bgcolor=\\"white\\">\\n'
                                                           '<center><h1>302 Moved Temporarily</h1></center>\\n'
                                                           '</body>\\n'
                                                           '</html>\\n'}),
    ])
]


class Location(object):
    SAS = 'sas'
    MAN = 'man'
    VLA = 'vla'
    PRIEMKA = 'priemka'


class PortNames(object):
    TUN = 'TunPort'
    TUN_SSL = 'TunPortSSL'
    INSTANCE = 'InstancePort'
    INSTANCE_SSL = 'InstancePortSSL'
    CACHER = 'CacherPort'
    QUERY_SEARCH = 'QuerySearchPort'


def get_api_searcherlookup(gname, tag):
    tout = 0.1
    last_e = None

    url = 'http://api.gencfg.yandex-team.ru/{path}/searcherlookup/groups/{group}/instances'.format(
        path=('trunk' if tag == 'trunk' else 'tags/{}'.format(tag)),
        group=gname,
    )

    while tout <= 11:
        try:
            r = requests.get(url)
            r.raise_for_status()
            return json.loads(r.content)
        except Exception as e:
            last_e = e
            time.sleep(tout)
            tout *= 10

    raise Exception('Failed to fetch %s searcherlookup from tag %s: %s' % (gname, tag, str(last_e)))


class ErsatzHostName:
    def __init__(self, hostname):
        self.name = hostname


class ErsatzIntGroup:
    def __init__(self, fqdn, port, power):
        self.host = ErsatzHostName(fqdn)
        self.port = port
        self.power = power


class ErsatzIntlookup:
    def __init__(self, group_name, tag):
        (meta, sorted_shard_instances) = intlookup_from_searcherlookup(get_api_searcherlookup(group_name, tag))
        self.shards_count = len(sorted_shard_instances) * meta["hosts_per_group"]
        self.group_name = meta["file_name"]
        self.shard_instances = sorted_shard_instances
        self.file_name = group_name

    def get_shards_count(self):
        return self.shards_count

    def get_base_instances_for_shard(self, shard_number):
        return self.shard_instances[shard_number]


class YpEndpointsetTemplate:
    def __init__(self, template, shards_count, cluster):
        self.shards_count = shards_count
        self.template = template
        self.cluster = cluster
        self.file_name = '{}#'.format(cluster) + template.format(num = 0) + '#{}'.format(shards_count)

    def get_shards_count(self):
        return self.shards_count

    def get_endpointset_for_shard(self, shard_number):
        return self.template.format(num = shard_number)

    def get_cluster(self):
        return self.cluster


def shardnumber_from_shardname(shardname):
    return int(shardname.split('-')[1])


def intlookup_from_searcherlookup(searcherlookup):
    brigades = collections.defaultdict(list)
    tiers = set([])
    switch = "" # FIXME
    hosts_per_group = 1 # FIXME
    group_name = ""
    for instance in searcherlookup['instances']:
        tags = instance['tags']
        for tag in tags:
            if tag.startswith('a_topology_group-'):
                group_name = tag[len('a_topology_group-'):]
            elif tag.startswith('a_tier_'):
                tiers.add(tag[len('a_tier_'):])
        fqdn = instance['hostname']
        port = instance['port']
        power = instance['power']
        dc = instance['dc']
        queue = instance['queue']
        brigades[shardnumber_from_shardname(instance['shard_name'])].append(ErsatzIntGroup(fqdn, port, power))
    meta = {"file_name":group_name,"hosts_per_group":hosts_per_group,"brigade_groups_count":len(brigades),"base_type":group_name,"tiers":[tier for tier in tiers]}
    return (meta, [v for k, v in sorted(brigades.items())])


class Project(object):
    def _set_group_topology(self, backends):
        if self.TRANSPORT.name() == 'curdb':
            return backends

        if isinstance(backends, collections.Sequence) and not isinstance(backends, basestring):
            return ['{}:{}'.format(x, self.TOPOLOGY) for x in backends]
        else:
            return '{}:{}'.format(backends, self.TOPOLOGY)

    def _port_for_group(self, group_name):
        ports = set(instance.port for instance in self.TRANSPORT.get_group_instances(group_name))
        if len(ports) > 1:
            raise Exception("Group {} has non-unique ports".format(group_name))
        elif len(ports) < 1:
            raise Exception("Group {} is empty".format(group_name))
        return ports.pop()

    @staticmethod
    def _read_json(json_file):
        try:
            with open("{}/{}".format(os.path.join(os.path.dirname(os.path.abspath(__file__))), json_file), 'r') as f:
                data = yaml.safe_load(f)
            return data
        except IOError, err:
            raise Exception("Couldn't read json file {0}: {1}".format(json_file, err))
        except ValueError, err:
            raise Exception("Couldn't parse json file {0}: {1}".format(json_file, err))

    def _compute_intlookups(self, intlookup):
        """
        Replace intlookup names with actual intlookups
        """
        intlookup_list = list()
        if self.TRANSPORT.name() == 'curdb':
            if intlookup and isinstance(intlookup[0], basestring):
                intlookup_list = [CURDB.intlookups.get_intlookup(x) for x in intlookup]
            else:
                for x in intlookup:
                    group_type = x.get('type', '')
                    if group_type == 'gencfg':
                        intlookup_list.append(CURDB.intlookups.get_intlookup(x.get('group')))
                    elif group_type == 'yp_endpointset':
                        intlookup_list.append(YpEndpointsetTemplate(x.get('template'), x.get('shards'), x.get('cluster')))
        else:
            for x in intlookup:
                if isinstance(x, basestring):
                    intlookup_list.append(ErsatzIntlookup(x, self.TOPOLOGY))
                else:
                    group_type = x.get('type', '')
                    if group_type == 'gencfg':
                        intlookup_list.append(ErsatzIntlookup(x.get('group'), self.TOPOLOGY))
                    elif group_type == 'yp_endpointset':
                        intlookup_list.append(YpEndpointsetTemplate(x.get('template'), x.get('shards'), x.get('cluster')))
        if not intlookup_list:
            print >> sys.stderr, "No intlookups configured for bundle %s" % intlookup
            exit(1)
        shard_count_list = [x.get_shards_count() for x in intlookup_list]
        if not all([x == shard_count_list[0] for x in shard_count_list]):
            print >> sys.stderr, "Different number of shards in intlookups for bundle %s" % intlookup
            exit(1)
        return intlookup_list

    @staticmethod
    def _fixed_shard_number(shard_number, total_shards):
        ln = 1 + int(math.log10(total_shards))
        shard_number_str = str(shard_number)
        return shard_number_str.rjust(ln, '0')

    @staticmethod
    def _extract_regexp(regexp_obj):
        regexp = dict()
        regexp_url = dict()
        if len(regexp_obj) > 1:
            regexp_content = list()
            for elem in regexp_obj:
                regexp_content.append(
                    (LuaAnonymousKey(), {
                        'match_fsm': OrderedDict([(
                            elem["match"], elem["template"]
                        )])
                    })
                )
                regexp = {
                    "match_and": OrderedDict(regexp_content)
                }
                if elem["match"] == "url":
                    regexp_url = elem["template"]
        else:
            regexp = {
                'match_fsm': OrderedDict([(
                    regexp_obj[0]["match"],
                    regexp_obj[0]["template"]
                )])
            }
            regexp_url = regexp_obj[0]["template"]
        return regexp, regexp_url

    def _balancer_options(self, shard_number, backends, intlookup_list, is_shift_ports, tier_balancer):
        if backends is None:
            backends_list = []
            for intlookup in intlookup_list:
                backends_list = backends_list + map(
                    lambda x: '%s:%s:%s(domain=.search.yandex.net,resolve=6)' %
                    (x.host.name, x.port, x.power), intlookup.get_base_instances_for_shard(shard_number))
            backends = self._set_group_topology(backends_list)

        balancerOptions = {
            'backends': backends,
            'attempts': len(backends),
            '2tier_balancer': tier_balancer,
            'resolve_protocols': [6],
            'proxy_options': OrderedDict([
                ('connect_timeout', LuaGlobal('connect_timeout', self.CONNECT_TIMEOUT)),
                ('backend_timeout', LuaGlobal('backend_timeout', self.BACKEND_TIMEOUT)),
                ('keepalive_count', 16),
                ('buffering', True),
                ('need_resolve', LuaGlobal('resolveBackends', True)),
            ]),
            'shift_ports': is_shift_ports
        }

        return balancerOptions

    def _balancer_module(self, shard_number, intlookup_list, is_shift_ports, tier_balancer):
        is_yp_endpoint_list = [isinstance(x, YpEndpointsetTemplate) for x in intlookup_list]
        if any(is_yp_endpoint_list):
            if all(is_yp_endpoint_list):
                backends = [ OrderedDict([
                    ('cluster_name', x.get_cluster()),
                    ('endpoint_set_id', x.get_endpointset_for_shard(shard_number))
                ]) for x in intlookup_list ]
                options = self._balancer_options(shard_number, backends, intlookup_list, is_shift_ports, tier_balancer)
                options['policies'] = OrderedDict([('unique_policy', {})])
                return (Modules.Balancer2, options)
            else:
                print >> sys.stderr, "No support for mixed yp/gencfg backends yet"
                exit(1)  # TODO FIXME
        else:
            return (Modules.Balancer, self._balancer_options(shard_number, None, intlookup_list, is_shift_ports, tier_balancer))

    # TODO: merge with balancer_options
    def plain_location(
            self,
            **kwargs):
        prefix = kwargs.get('prefix', None)
        return (Modules.Balancer, {
            'backends': self._set_group_topology(kwargs['intlookup_list']),
            'attempts': LuaGlobal(
                '{}backend_attempt'.format(prefix), 2
            ),
            '2tier_balancer': kwargs.get("tier_balancer", False),
            'resolve_protocols': [6],
            'proxy_options': OrderedDict([
                ('connect_timeout', LuaGlobal(
                    "{}connect_timeout".format(prefix),
                    kwargs['connect_timeout'])),
                ('backend_timeout', LuaGlobal(
                    "{}backend_timeout".format(prefix),
                    kwargs['backend_timeout'])),
                ('keepalive_count', LuaGlobal(
                    "{}keepalive_count".format(prefix),
                    kwargs.get('keepalive_count', 16))),
                ('buffering', LuaGlobal(
                    '{}buffering'.format(prefix),
                        kwargs.get('buffering', True))),
                ('need_resolve', LuaGlobal('resolveBackends', kwargs.get('need_resolve', True))),
            ]),
            'shift_ports': kwargs.get("is_shift_ports", False)
        })

    def plain_host(
            self,
            **kwargs):
        prefix = kwargs.get('prefix', None)
        res = (Modules.Proxy, {
                        'host': kwargs['host'],
                        'port': kwargs['port'],
                        'connect_timeout': LuaGlobal(
                            "{}connect_timeout".format(prefix),
                            kwargs['connect_timeout']),
                        'backend_timeout': LuaGlobal(
                            "{}backend_timeout".format(prefix),
                            kwargs['backend_timeout']),
                        'keepalive_count': LuaGlobal(
                            "{}keepalive_count".format(prefix),
                            kwargs.get('keepalive_count', 128))
        })

        if 'rewrite' in kwargs:
            res = [(Modules.Rewrite, {'actions': [
                ({
                    'regexp': kwargs['regexp_template'],
                    'rewrite': kwargs['rewrite'],
                    'split': 'url'
                }),
            ]}), res]
        return res

    def _single_shard_config(
            self,
            index_type,
            shard_number,
            total_shards,
            intlookup_list,
            is_shift_ports,
            regexp_template,
            rewrite,
            tier_balancer=False):
        regexp = regexp_template % {
            'shard_number': shard_number,
            'total_shards': total_shards
        }

        balancer_module = self._balancer_module(shard_number, intlookup_list, is_shift_ports, tier_balancer)

        if rewrite:
            submodules = [(Modules.Rewrite, {'actions': [
                ({'regexp': regexp, 'rewrite': rewrite, 'split': 'url'})]}), balancer_module]
        else:
            submodules = [balancer_module]

        return submodules

    def consistent_hashing(
            self,
            **kwargs):

        N = kwargs['intlookup_list'][0].get_shards_count()
        # Add cache for consistent_hash in order to not generate same
        # things multiple times
        cache_id = (kwargs['index_type'],
                    tuple(map(lambda x: x.file_name,
                              kwargs['intlookup_list'])),
                    kwargs['is_shift_ports'],
                    kwargs['regexp_template'],
                    kwargs.get('tier_balancer', False))
        if cache_id not in self.consistentHashingCache:
            thumb_locations = [
                (self._fixed_shard_number(
                    i,
                    N),
                    self._single_shard_config(
                    kwargs['index_type'],
                    i,
                    N,
                    kwargs['intlookup_list'],
                    kwargs['is_shift_ports'],
                    kwargs['regexp_template'],
                    kwargs['rewrite'],
                    kwargs['tier_balancer'])) for i in range(N)]
            self.consistentHashingCache[cache_id] = (Modules.ThumbConsistentHash,
                                                     {'id_regexp': kwargs['id_regexp'],
                                                      'locations': thumb_locations,
                                                      'default': [error_document_404]})

        submodules = self.consistentHashingCache[cache_id]

        return submodules

    def _backends_rr(
            self,
            backend_stats_attr,
            index_name,
            index_opts,
            backends,
            primal_location,
            default_priority,
            global_shift_port=False,
            tier_balancer=False,
            global_prefix=''):
        '''
            Return backends blocks for provided circuit and geo balancing
        '''
        production_locations = backends.keys()
        # devnull location
        devnull_backend = (0, index_name + '_disable', [error_document_502])
        res = [devnull_backend]
        regexp = dict()
        # traverse by each location
        for location in production_locations:
            if location not in backends:
                continue
            # set default priority for home location
            if primal_location:
                priority = default_priority if primal_location == location else 0
            else:
                priority = default_priority / len(production_locations)
            try:
                backend_opts = backends[location]
            except KeyError as e:
                print "WARNING: {}".format(e)
                continue
            if backend_opts.get('enable', True):
                regexp, regexp_template = self._extract_regexp(index_opts["regexp"])
                intlookup = None
                host, port = None, None
                if index_opts.get('sharded', False):
                    intlookup = self._compute_intlookups(backend_opts['name'])
                elif 'name' in backend_opts:
                    intlookup = backend_opts['name']
                elif 'host' in backend_opts:
                    host = backend_opts['host']
                    port = backend_opts['port']
                else:
                    raise Exception("no valid backends")
                backend_prefix = backend_opts.get('prefix', '')
                index_locations_func_name = backend_opts.get('shardFunc', index_opts['shardFunc'])
                index_locations_func = getattr(self, index_locations_func_name)
                index_locations = index_locations_func(
                    prefix='{}{}'.format(
                        global_prefix,
                        backend_prefix + '_' if len(backend_prefix) > 0 else ''),
                    index_type=index_name,
                    intlookup_list=intlookup,
                    host=host,
                    port=port,
                    is_shift_ports=backend_opts.get('is_shift_ports', global_shift_port),
                    rewrite=index_opts.get('rewrite', None),
                    regexp_template=regexp_template,
                    id_regexp=index_opts.get('id_regexp', False),
                    tier_balancer=tier_balancer,
                    connect_timeout=backend_opts.get('connect_timeout', self.CONNECT_TIMEOUT),
                    backend_timeout=backend_opts.get('backend_timeout', self.BACKEND_TIMEOUT))
                if not isinstance(index_locations, list):
                    index_locations = [index_locations]

                section_name = "{}_{}".format(index_name, location)
                stats_attr = "{0}_{1}_{2}".format(backend_stats_attr, index_name, location)
                res.append(
                    (priority, section_name, [
                        (Modules.Report, {
                            'uuid': stats_attr,
                            'ranges': Constants.ALL_REPORT_TIMELINES,
                            'disable_robotness': True,
                            'disable_sslness': True,
                        }),
                        (Modules.ResponseHeaders, {'create_weak': OrderedDict([('X-Ya-I', location)])})
                    ] + index_locations)
                )
        return res, regexp

    def _backends_weighted(self,
                           index_name,
                           index_opts,
                           opts,
                           primal_location,
                           global_shift_port,
                           tier_balancer,
                           global_prefix):
        if not opts.get('enable', True):
            raise Exception('Primal location {} is disabled for {} index type'.format(primal_location, index_name))
        regexp, regexp_template = self._extract_regexp(index_opts["regexp"])
        index_prefix = index_opts.get('prefix', '')

        intlookup = None
        host, port = None, None
        if index_opts.get('sharded', False):
            intlookup = self._compute_intlookups(opts['name'])
        elif 'name' in opts:
            intlookup = opts['name']
        elif 'host' in opts:
            host = opts['host']
            port = opts['port']
        else:
            raise Exception("no valid backends")

        index_locations_func_name = opts.get('shardFunc', index_opts['shardFunc'])
        index_locations_func = getattr(self, index_locations_func_name)
        index_locations = index_locations_func(
            index_type=index_name,
            intlookup_list=intlookup,
            host=host,
            port=port,
            is_shift_ports=opts.get('is_shift_ports', global_shift_port),
            rewrite=index_opts.get('rewrite', None),
            regexp_template=regexp_template,
            id_regexp=index_opts.get('id_regexp', None),
            tier_balancer=opts.get('tier_balancer', tier_balancer),
            prefix='{}{}'.format(
                global_prefix,
                index_prefix + '_' if len(index_prefix) > 0 else ''),
            connect_timeout=opts.get('connect_timeout', self.CONNECT_TIMEOUT),
            backend_timeout=opts.get('backend_timeout', self.BACKEND_TIMEOUT))
        if not isinstance(index_locations, list):
            index_locations = [index_locations]

        return index_locations, regexp

    def _fallback_location(self,
                           stats_attr,
                           index_name,
                           index_opts,
                           fallback_opts,
                           primal_location,
                           shift_port):
        if primal_location not in fallback_opts['prod']:
            raise Exception("Fallback is enabled, but related fallback section isn't exists for {} index type",
                            index_name)
        thdb_func = self._generate_backends_rr if fallback_opts['prod'][primal_location].get('rr', False) else \
            self._generate_backends_weight
        prefix = fallback_opts['prod'][primal_location].get('prefix', 'fallback')
        res, _ = thdb_func(stats_attr,
                           index_name,
                           index_opts,
                           backends_opts=fallback_opts,
                           primal_location=primal_location,
                           enable_fallback=True if fallback_opts.get('on_error', False) else False,
                           global_prefix=prefix + '_' if len(prefix) > 0 else '',
                           global_shift_port=shift_port)
        return res

    def _generate_backends_weight(
            self,
            backend_stats_attr,
            index_name,
            index_opts,
            backends_opts,
            primal_location,
            enable_fallback,
            global_prefix='',
            tier_balancer=False,
            default_priority=100,
            global_shift_port=False):
        '''
            Return backends blocks with full mesh balancing
        '''
        if not primal_location:
            raise Exception('Unknown primal location(s) for weighted backends for {} index type'.format(index_name))

        index_prefix = backends_opts['prod'][primal_location].get('prefix', '')
        global_prefix += index_prefix + '_' if len(index_prefix) > 0 else ''

        devnull_backend = [(0, 'disable', [error_document_502])]

        index_locations, regexp = self._backends_weighted(index_name, index_opts,
                                                          backends_opts['prod'][primal_location],
                                                          primal_location, global_shift_port, tier_balancer,
                                                          global_prefix)

        stats_attr = '{0}{1}_{2}'.format(global_prefix, backend_stats_attr, index_opts['stats_attr'])
        weight_file = LuaGlobal(
            '{0}{1}WeightFile'.format(
                global_prefix, index_name), "{0}{1}".format(self.ITS_PATH,
                                                            index_opts['{0}weight_file'.format(global_prefix)]))
        regexp_backends = [
            (default_priority, 'enable'.format(index_name), index_locations)
        ]

        balancer_config = dict(
                balancer_type='rr',
                attempts=1,
                balancer_options=OrderedDict([('weights_file', weight_file)]),
                custom_backends=devnull_backend + regexp_backends
        )

        # fallback
        fallback_opts = backends_opts.get('on_error', dict())
        if len(fallback_opts) > 0:
            balancer_config['on_error'] = self._fallback_location(backend_stats_attr,
                                                                  index_name,
                                                                  index_opts,
                                                                  fallback_opts,
                                                                  primal_location,
                                                                  global_shift_port)

        regexp_config = [
            (Modules.Report, {
                'uuid': stats_attr,
                'ranges': Constants.ALL_REPORT_TIMELINES,
                'disable_robotness': True,
                'disable_sslness': True,
            }),
            (Modules.Balancer, balancer_config)
        ]

        return regexp_config, regexp

    def _generate_backends_rr(
            self,
            backend_stats_attr,
            index_name,
            index_opts,
            backends_opts,
            primal_location,
            enable_fallback,
            global_prefix='',
            tier_balancer=False,
            default_priority=100,
            global_shift_port=False):
        '''
            Return backends blocks for each provided circuit and geo balancing
        '''
        custom_backends, regexp = self._backends_rr(
            backend_stats_attr,
            index_name,
            index_opts,
            backends_opts['prod'],
            primal_location,
            default_priority,
            global_shift_port=global_shift_port,
            tier_balancer=tier_balancer,
            global_prefix=global_prefix)

        stats_attr = '{0}{1}_{2}'.format(global_prefix, backend_stats_attr, index_opts['stats_attr'])
        weight_file = LuaGlobal(
            '{0}{1}WeightFile'.format(
                global_prefix, index_name), '{0}{1}'.format(self.ITS_PATH,
                                                            index_opts['{0}weight_file'.format(global_prefix)]))
        balancer_config = dict(
            balancer_type='rr',
            attempts=1,
            balancer_options=OrderedDict([('weights_file', weight_file)]),
            custom_backends=custom_backends
        )

        # fallback
        fallback_opts = backends_opts.get('on_error', dict())
        if len(fallback_opts) > 0:
            balancer_config['on_error'] = self._fallback_location(backend_stats_attr,
                                                                  index_name,
                                                                  index_opts,
                                                                  fallback_opts,
                                                                  primal_location,
                                                                  global_shift_port)

        regexp_config = [
            (Modules.Report, {
                'uuid': stats_attr,
                'ranges': Constants.ALL_REPORT_TIMELINES,
                'disable_robotness': True,
                'disable_sslness': True,
            }),
            (Modules.Balancer, balancer_config)
        ]
        return regexp_config, regexp

    def _generate_cacher_config(self,
                                primal_location,
                                config,
                                opts,
                                stats_attr,
                                index_type,
                                cgi_hashing_params):
        '''
            Return block with distributed cache
        '''
        stats_attr = "{}_cacher_{}".format(stats_attr, index_type)
        id_regexp = opts['id_regexp']
        res = [
            (0, 'disable_cacher', config)
        ]
        if len(cgi_hashing_params) > 0:
            hasher_module = (Modules.CgiHasher, {
                'parameters': [x for x in cgi_hashing_params],
                'randomize_empty_match': False
            })
        else:
            hasher_module = (Modules.Hasher, {
                'mode': 'request'
            })
        for loc in opts["backends"].keys():
            backends_opts = opts["backends"][loc][index_type]
            if not backends_opts["enable"]:
                continue
            cache_module = (Modules.CacheClient, {
                    'id_regexp': id_regexp,
                    'server': [
                        (Modules.Report, {
                            'uuid': stats_attr,
                            'ranges': Constants.ALL_REPORT_TIMELINES,
                            'disable_robotness': True,
                            'disable_sslness': True,
                        }),
                        hasher_module,
                        (Modules.Balancer, {
                            'balancer_type': 'consistent_hashing',
                            'backends': self._set_group_topology(backends_opts["groups"]),
                            'attempts': 2,
                            'resolve_protocols': [6],
                            'proxy_options': OrderedDict([
                                ('keepalive_count', opts["keepalive_count"]),
                                ('connect_timeout', LuaGlobal('cacher_connect_timeout', opts["connect_timeout"])),
                                ('backend_timeout', LuaGlobal('cacher_backend_timeout', opts["backend_timeout"])),
                            ]),
                        }),
                    ],
                    'module': config,
                })
            priority = 1 if primal_location == loc else 0
            res.append(
                (priority, '{}_cacher'.format(loc), [
                    cache_module
                ]),
            )
        res = [
            (Modules.Balancer, {
                'balancer_type': 'rr',
                'attempts': opts["attempts"],
                'balancer_options': OrderedDict(
                    [('weights_file', LuaGlobal(
                        'cacherWeightFile',
                        "{0}{1}".format(self.ITS_PATH, opts["weights_file"])
                    ))]
                ),
                'custom_backends': res
            })
        ]
        if opts.get('use_dbid', False):
            res = [
                (Modules.Hdrcgi, {
                    'cgi_from_hdr': OrderedDict([('dbid', 'X-Thdb-Version')]),
                })
            ] + res
        return res

    def _ban_config(self, backend_opts, primal_location):
        prod_opts = backend_opts['prod'][primal_location]
        res, fallback = dict(), dict()

        # fallback
        fallback_opts = backend_opts.get('on_error', dict())
        if len(fallback_opts) > 0 and primal_location in fallback_opts['prod']:
            fallback = self._ban_config(fallback_opts, primal_location)

        if 'host' in prod_opts:
            host = prod_opts['host']
            if 'port' in prod_opts:
                port = prod_opts['port']
            elif 'port_group' in prod_opts:
                port = self._port_for_group(prod_opts["port_group"])
            else:
                raise Exception("Port doesn't set for ban group in {} location".format(primal_location.upper()))
            port = LuaGlobal(PortNames.QUERY_SEARCH, port)
            res = self.plain_host(
                prefix='ban_{}_'.format(host),
                host=host,
                port=port,
                connect_timeout=prod_opts['connect_timeout'],
                backend_timeout=prod_opts['backend_timeout'],
                keepalive_count=prod_opts['keepalive_count']
            )
            res = (Modules.Balancer, {
                'custom_backends': [
                    (100.0, 'enable', [
                        res
                    ])
                ]
            })
        elif 'name' in prod_opts:
            intlookup_prefix = "_".join(prod_opts['name'])
            res = self.plain_location(
                prefix='ban_{}_'.format(intlookup_prefix),
                intlookup_list=prod_opts['name'],
                connect_timeout=prod_opts['connect_timeout'],
                backend_timeout=prod_opts['backend_timeout'],
                keepalive_count=prod_opts['keepalive_count'],
            )
        else:
            raise Exception("No ban instances is defined for {} location".format((primal_location.upper())))

        if len(fallback) > 0:
            res[1]["on_error"] = [fallback]
        return res

    def _generate_ban_config(self, primal_location, config, opts, stats_attr):
        '''
            Return block with DMCA system
        '''
        stats_attr = "{}_ban".format(stats_attr)

        balancer_module = self._ban_config(opts['backends'], primal_location)
        config = [
            (Modules.ThumbsBan, {
                'id_regexp': opts["id_regexp"],
                'checker': [
                    (Modules.Report, {
                        'uuid': stats_attr,
                        'ranges': Constants.ALL_REPORT_TIMELINES,
                        'disable_robotness': True,
                        'disable_sslness': True,
                    }),
                    balancer_module,
                ],
                'ban_handler': [
                    (Modules.ErrorDocument, {'status': 410}),
                ],
                'module': config
            }),
        ]
        return config

    def _config(
            self,
            index_types,
            instances,
            stats_attr,
            cache,
            ban,
            primal_location,
            prefix='',
            shift_port=False,
            fallback=False):
        prefix = prefix + '_' if len(prefix) > 0 else ''
        for index_name, index_opts in index_types.iteritems():
            backend_opts = copy.deepcopy(instances["backends"][index_name])
            if 'custom_rr' in backend_opts['prod'][primal_location]:
                for loc in backend_opts['prod'].keys():
                    if loc != primal_location:
                        backend_opts['prod'].pop(loc)
                for loc, backends in backend_opts['prod'][primal_location]['custom_rr'].iteritems():
                    backend_opts['prod'][loc] = backends
                backend_opts['prod'][primal_location].pop('custom_rr')
                thdb_func = self._generate_backends_rr
            elif backend_opts['prod'][primal_location].get('rr', False):
                thdb_func = self._generate_backends_rr
            else:
                thdb_func = self._generate_backends_weight

            regexp_config, regexp = thdb_func(stats_attr,
                                              index_name,
                                              index_opts,
                                              backend_opts,
                                              primal_location,
                                              backend_opts['prod'][primal_location].get('fallback', fallback),
                                              prefix,
                                              global_shift_port=shift_port)

            # reply file for debug
            if 'reply_file' in index_opts.keys():
                rate_file = index_opts['reply_file']
                sink_regexp_config, _ = thdb_func(stats_attr,
                                                  index_name,
                                                  index_opts,
                                                  backend_opts,
                                                  primal_location,
                                                  False,
                                                  global_prefix='sink_',
                                                  default_priority=0,
                                                  global_shift_port=shift_port)
                regexp_config = [
                    (Modules.RequestReplier, {
                        'rate_file': '{0}{1}'.format(self.ITS_PATH, rate_file),
                        'sink': sink_regexp_config
                    })
                ] + regexp_config

            # use caching farm
            if index_opts.get('cache', cache):
                cacher_backends = instances["cachers"]
                cgi_hashable_params = index_opts.get('cgi_hashable_params', list())
                regexp_config = self._generate_cacher_config(
                    primal_location, regexp_config, cacher_backends, stats_attr, index_name, cgi_hashable_params)

            # X-Thdb-Version header support
            if 'thdb_file' in index_opts.keys():
                file_name = "{}{}".format(self.ITS_PATH, index_opts["thdb_file"])
                regexp_config = [
                    (Modules.ResponseHeaders, {
                        'delete': 'X-Thdb-Version'
                    }),
                    (Modules.ThdbVersion, {
                        'file_name': LuaGlobal(index_name + 'ThdbFile', file_name),
                        'file_read_timeout': '1s'
                    }),
                ] + regexp_config

            # use ban backends
            if index_opts.get('ban', ban):
                ban_backends = instances["bans"]
                regexp_config = self._generate_ban_config(primal_location,
                                                          regexp_config,
                                                          ban_backends,
                                                          stats_attr)

            balancer_config = [
                (index_name, regexp, [
                    (Macroses.WebHeaders, {}),
                    (Modules.Report, {
                        'uuid': index_name,
                        'ranges': Constants.ALL_REPORT_TIMELINES,
                        'disable_robotness': True,
                        'disable_sslness': True,
                    })
                ] + regexp_config)
            ]

            yield balancer_config

    def _http_config(
            self,
            config,
            access_log,
            error_log,
            stats_attr,
            http_opts,
            https_config=None,
            debug_log=None):
        '''
            Return config with HTTP module
        '''
        http_config = [
            (Modules.Http, {
                'maxlen': http_opts.get('maxlen', self.DEFAULT_MAX_LEN),
                'maxreq': http_opts.get('maxreq', self.DEFAULT_MAX_REQ),
                'stats_attr': stats_attr,
                'keepalive': http_opts.get('keepalive', self.DEFAULT_KEEPALIVE)
            })
        ]
        if access_log:
            http_config.append(
                (Modules.Accesslog, {
                    'log': access_log
                }),
            )
        http_config.extend((
            (Modules.Report, {
                'uuid': 'service_total',
                'ranges': Constants.ALL_REPORT_TIMELINES,
                'disable_robotness': True,
                'disable_sslness': True,
            }),
        ))
        http_config = http_config + config
        if https_config:
            if http_opts.get('http2', False):
                http_config = [(Modules.Http2, {
                    'goaway_debug_data_enabled': 0,
                    'debug_log_name': debug_log,
                })] + http_config
            http_config = https_config + http_config
        http_config = [
                          (Modules.Errorlog, {'log': error_log}),
                      ] + http_config
        return http_config

    def _generate_ip_dispatch(
            self,
            name,
            config,
            rs_hosts,
            _ips_only_filter,
            _ips_exclude_filter,
            location,
            stats_attr,
            internal_port,
            service_port=None,
            domain='*'):
        '''
            Generate config for each ip
        '''
        if not domain == '*':
            if self.TRANSPORT.name() == 'curdb':
                _ips = [str(x) for x in self.TRANSPORT.db.slbs.get(domain).ips]
            else:
                _ips = Utils.GetIpFromRT({
                    'balancer': rs_hosts[location],
                    'slb': domain,
                })
            if not _ips:
                raise "Error: couldn't get any ips from Racktables for {} balancer in {} slb farm".format(
                    rs_hosts[location], domain
                )
            _ips = set(_ips)
            if len(_ips_only_filter) > 0:
                _ips = _ips & set(_ips_only_filter)
            if len(_ips_exclude_filter) > 0:
                _ips = _ips - set(_ips_exclude_filter)
            section_name = '%s_%s_%s' % (name, domain, 'service')
            res = [
                (section_name, {
                    'ips': list(_ips),
                    'port': service_port,
                    'stats_attr': stats_attr,
                    'disabled': LuaGlobal('SkipBindToServiceAddrs', False)
                }, config)
            ]
            for r in res:
                yield r
        else:
            yield (name + '_wildcard', {
                'ip': domain,
                'port': internal_port,
                'stats_attr': stats_attr,
                'disabled': LuaGlobal('SkipBindToWildcard', False)
            }, config)

    def _ssl(self, ssl_obj, port):
        '''
            Return SSL block
        '''
        if 'prefix' in ssl_obj:
            prefix = ssl_obj['prefix']
        else:
            prefix = ''
        result = {
            'cert': LuaGlobal('{0}SSLCert'.format(prefix), ssl_obj['cert']),
            'priv': LuaGlobal('{0}SSLCertKey'.format(prefix), ssl_obj['certKey']),
            'ciphers': Constants.SSL_CIPHERS_SUITES_SHA2,
        }
        events = []
        if 'ocsp' in ssl_obj:
            result['ocsp'] = LuaGlobal('{0}OCSP'.format(prefix), ssl_obj['ocsp'])
            events = events + [('reload_ocsp_response', 'reload_ocsp')]
        if 'tls_tickets' in ssl_obj:
            result['ticket_keys_list'] = OrderedDict([
                ('tls_1stkey', OrderedDict([
                    ('keyfile', LuaGlobal('{0}Ticket1stKey'.format(prefix), ssl_obj['tls_tickets'][0])),
                    ('priority', 1000),
                ])),
                ('tls_2stkey', OrderedDict([
                    ('keyfile', LuaGlobal('{0}Ticket2stKey'.format(prefix), ssl_obj['tls_tickets'][1])),
                    ('priority', 999),
                ])),
            ])
            events = events + [('reload_ticket_keys', 'reload_ticket')]
        if events:
            result['events'] = OrderedDict(events)
        # sha1
        if "sha1" in ssl_obj:
            result['secondary'] = OrderedDict([
                ('cert', LuaGlobal('{0}SSLCertSHA1'.format(prefix), ssl_obj["sha1"]["cert"])),
                ('priv', LuaGlobal('{0}SSLCertKeySHA1'.format(prefix), ssl_obj["sha1"]["certKey"])),
            ])
        if ssl_obj.get('secrets_log', False):
            result['secrets_log'] = self.LOG_DIR + 'current-secrets_log-balancer-' + port
        return [(Modules.SslSni, result)]

    def balancer(self, json_object, location):
        #try:
        name = json_object["opts"]["name"]
        rs_hosts = json_object["opts"].get("rs_hosts", None)
        rewrite_ports = {
            'commercial': 8041,
            'commercial-hamster': 25120,
            'media-rim': 80
        }
        if name in rewrite_ports:
            instance_port = rewrite_ports[name]
        else:
            instance_port = self._port_for_group(json_object["instances"]["balancers"][location]["group"][0])
        port = LuaGlobal(PortNames.INSTANCE, instance_port)
        port_ssl = LuaGlobal(PortNames.INSTANCE_SSL, instance_port + 1)
        only_ips = json_object["opts"].get("ips_only", list())
        exclude_ips = json_object["opts"].get("ips_exclude", list())
        index_types = json_object["index_types"]
        instances = json_object["instances"]
        domains = ["*"]
        project_domain = json_object["opts"]["domain"]
        if project_domain != "*":
            domains.append(project_domain)
        apphost_enable = json_object["opts"].get("apphost", False)
        ban_enable = json_object["opts"].get("ban", False)
        cors_enable = json_object["opts"].get("cors", False)
        webp_enable = json_object["opts"].get("webp", False)
        cache_enable = json_object["opts"].get("cache", False)
        global_fallback_enable = json_object["opts"].get("fallback", False)
        prj_refs = json_object["opts"].get("prj_references", dict())
        http_opts = json_object["opts"].get("http", dict())
        stats_attr_balancer = json_object["opts"]["stats_attr_balancer"]
        stats_attr_balancer_ssl = "{}_ssl".format(stats_attr_balancer)
        stats_attr_backend = json_object["opts"]["stats_attr_backend"]
        sd_client_name = json_object["opts"].get("sd_client_name", None)
        sd_update_frequency = json_object["opts"].get("sd_update_frequency", None)
        #except:
        #    raise Exception("Couldn't get obligatory option from json file: {}".format(sys.exc_info()[0]))

        master_log = self.LOG_DIR + 'current-master_log-balancer-' + lua_global_to_string(port)
        access_log = self.LOG_DIR + 'current-access_log-balancer-' + lua_global_to_string(port)
        error_log = self.LOG_DIR + 'current-error_log-balancer-' + lua_global_to_string(port)
        debug_log = self.LOG_DIR + 'current-debug_log-balancer-' + lua_global_to_string(port)

        config = self.regexpCommonLocations

        for x in self._config(
                index_types=index_types,
                instances=instances,
                stats_attr=stats_attr_backend,
                cache=cache_enable,
                ban=ban_enable,
                primal_location=location,
                fallback=global_fallback_enable):
            config = config + x

        apphost_dispachers = list()
        if apphost_enable:
            apphost_stats_attr_balancer = 'apphost_{}'.format(stats_attr_balancer)
            for balancer_config in self._config(
                    index_types=index_types,
                    instances=instances,
                    stats_attr=stats_attr_backend,
                    cache=cache_enable,
                    ban=ban_enable,
                    primal_location=location,
                    prefix="apphost",
                    shift_port=True,
                    fallback=global_fallback_enable):
                if len(balancer_config) < 1:
                   raise Exception("Couldn't generate apphost config")

                index_name = balancer_config[0][0]
                apphost_config = [
                    (Modules.Regexp, self.regexpCommonLocations + balancer_config)
                ]
                # http config
                apphost_http_config = self._http_config(apphost_config,
                                                        access_log,
                                                        error_log,
                                                        apphost_stats_attr_balancer,
                                                        http_opts=http_opts)
                # port
                apphost_port = int(instance_port) + int(index_types[index_name]['apphost_shift_port'])
                lua_apphost_port = LuaGlobal('ApphostBalancerPort_{}'.format(index_name), str(apphost_port))
                # dispatch
                apphost_dispach_name = "apphost_{}".format(index_name)
                for domain in domains:
                    for ip_config in self._generate_ip_dispatch(apphost_dispach_name,
                                                                apphost_http_config,
                                                                rs_hosts,
                                                                only_ips,
                                                                exclude_ips,
                                                                location=location,
                                                                stats_attr=apphost_stats_attr_balancer,
                                                                internal_port=lua_apphost_port,
                                                                service_port=self.HTTP_PORT,
                                                                domain=domain):
                        apphost_dispachers.append(ip_config)

        if len(prj_refs) > 0:
            prj_config = [
                ('default', {}, [
                    (Modules.Report, {
                        'uuid': "{}_ref-default".format(stats_attr_balancer),
                        'ranges': Constants.ALL_REPORT_TIMELINES,
                        'disable_robotness': True,
                        'disable_sslness': True,
                    }),
                    (Modules.Regexp, config)
                ])
            ]

            for name, cgi in prj_refs.iteritems():
                prj_config.append(
                    (name, {'match_fsm': OrderedDict([('CGI', '.*ref={}.*'.format(cgi))])}, [
                        (Modules.Report, {
                            'uuid': "{}_ref-{}".format(stats_attr_balancer, name),
                            'ranges': Constants.ALL_REPORT_TIMELINES,
                            'disable_robotness': True,
                            'disable_sslness': True,
                        }),
                        (Modules.Regexp, config)
                    ])
                )
            config = [
                (Modules.Regexp, prj_config)
            ]
        elif cors_enable:
            config = [
                (Modules.Regexp, [
                    ('cors',
                     {'match_fsm': OrderedDict(
                         [('header', {'name': 'Origin', 'value': '(http[s]?://)?{}'.format(Constants.YANDEX_HOST)}),
                          ('case_insensitive', 'true')])}, [
                         (Modules.ResponseHeaders, {
                             'create': OrderedDict([
                                 ('Access-Control-Allow-Methods', 'GET, HEAD')
                             ])
                         }),
                         (Modules.HeadersForwarder, {
                             'actions': [({
                                 'request_header': 'Origin',
                                 'response_header': 'Access-Control-Allow-Origin',
                                 'erase_from_request': True,
                                 'erase_from_response': True,
                                 'weak': False
                             })]
                         }),
                         (Modules.Regexp, config)
                     ]),
                    ('default', { }, [
                        (Modules.Regexp, config)
                    ])
                ])
            ]
        else:
            config = [
                (Modules.Regexp, config)
            ]

        # webp support (IMAGES-12425)
        if webp_enable:
            webp_matcher = OrderedDict([
                (LuaAnonymousKey(), {
                    'match_fsm': OrderedDict([
                        ('header', {
                            'name': 'Accept',
                            'value': '(.*,)?image/webp.*'
                        })
                    ])
                }),
                (LuaAnonymousKey(), {
                    'match_not': OrderedDict([
                        ('match_fsm', OrderedDict([('CGI', 'enc=[0-9a-z]+'), ('surround', 'true')]))
                    ])
                })
            ])
            # MEDIASRE-557
            ignore_quality_mismatch_matcher = OrderedDict([
                (LuaAnonymousKey(), {
                    'match_fsm': OrderedDict([
                        ('CGI', 'n=13'), ('surround', 'true')
                    ]),
                }),
                (LuaAnonymousKey(), {
                    'match_not': OrderedDict([
                        ('match_fsm', OrderedDict([('CGI', 'q=[0-9]+'), ('surround', 'true')]))
                    ])
                })
            ])
            config = [
                (Modules.Regexp, [
                    ('webp', {'match_and': webp_matcher}, [
                        (Modules.Regexp, [
                            ('ignore_quality_mismatch', {'match_and': ignore_quality_mismatch_matcher}, [
                                (Modules.Rewrite, {'actions': [
                                    ({
                                        'regexp': '(.*)',
                                        'rewrite': '%1&enc=webp&ignore_quality_mismatch=yes',
                                        'split': 'url'
                                    })
                                ]})
                            ] + config),
                            ('default', {}, [
                                (Modules.Rewrite, {'actions': [
                                    ({
                                        'regexp': '(.*)',
                                        'rewrite': '%1&enc=webp',
                                        'split': 'url'
                                    })
                                ]})
                            ] + config)
                        ])
                    ]),
                    ('default', {}, config)
                ])
            ]

        dispachers = []

        if len(apphost_dispachers) > 0:
            dispachers += apphost_dispachers

        for domain in domains:
            # HTTP
            http_config = self._http_config(
                config, access_log, error_log, stats_attr_balancer, http_opts=http_opts)
            for ip_config in self._generate_ip_dispatch(
                    'remote',
                    http_config,
                    rs_hosts,
                    only_ips,
                    exclude_ips,
                    location=location,
                    stats_attr=stats_attr_balancer,
                    internal_port=port,
                    service_port=self.HTTP_PORT,
                    domain=domain):
                dispachers.append(ip_config)
            # HTTPS
            if "ssl" in json_object and json_object["ssl"]["enable"]:
                https_section = self._ssl(json_object["ssl"], port)
                https_config = self._http_config(
                    config,
                    access_log,
                    error_log,
                    stats_attr_balancer_ssl,
                    http_opts=http_opts,
                    https_config=https_section,
                    debug_log=debug_log)
                for ip_config in self._generate_ip_dispatch(
                        'remote_ssl',
                        https_config,
                        rs_hosts,
                        only_ips,
                        exclude_ips,
                        location=location,
                        stats_attr=stats_attr_balancer_ssl,
                        internal_port=port_ssl,
                        service_port=self.HTTPS_PORT,
                        domain=domain):
                    dispachers.append(ip_config)

        unistat_port = LuaGlobal('InstancePort', instance_port) + 2
        config = [
            (Modules.Main, {
                'workers': self.WORKERS,
                'maxconn': 50000,
                'log': master_log,
                'admin_port': port,
                'config_tag': self.TRANSPORT.describe_gencfg_version(),
                'enable_reuse_port': json_object["opts"].get("reuse_port", False),
                'sd_client_name': sd_client_name,
                'sd_update_frequency': sd_update_frequency,
                'unistat': OrderedDict([
                    ('addrs', LuaGlobal('instance_unistat_addrs', OrderedDict([
                            (LuaAnonymousKey(), OrderedDict([('ip', '127.0.0.1'), ('port', unistat_port)])),
                            (LuaAnonymousKey(), OrderedDict([('ip', '::1'), ('port', unistat_port)])),
                            (LuaAnonymousKey(), OrderedDict([('ip', LuaFuncCall('GetIpByIproute', {'key': 'v4'})), ('port', unistat_port)])),
                            (LuaAnonymousKey(), OrderedDict([('ip', LuaFuncCall('GetIpByIproute', {'key': 'v6'})), ('port', unistat_port)]))]))),
                    ('hide_legacy_signals', False)
                ])
            }),
            (Modules.Ipdispatch, dispachers),
        ]
        return config

    def __init__(self, transport, topology):
        self.TRANSPORT = transport
        self.TOPOLOGY = topology
        # workers amount
        self.WORKERS = LuaGlobal('WorkersAmmount', 8)
        # static weights files
        self.ITS_PATH = 'controls/'
        # LogDir
        self.LOG_DIR = LuaGlobal('LogDir', "/place/db/www/logs/")
        # some embedded lua functions that run during start instance
        self.PREFIX = Constants.GetIpByIproute
        # enable keep-alive by default
        self.DEFAULT_KEEPALIVE = 1
        self.DEFAULT_MAX_LEN = 16 * 1024
        self.DEFAULT_MAX_REQ = 8 * 1024
        # TIMEOUTS
        self.CONNECT_TIMEOUT = '100ms'
        self.BACKEND_TIMEOUT = '1s'
        # TLS defaults
        # TUN SLB scheme
        self.HTTP_PORT = LuaGlobal(PortNames.TUN, 80)
        self.HTTPS_PORT = LuaGlobal(PortNames.TUN_SSL, 443)
        # Common regexps
        self.consistentHashingCache = dict()


class Rim(Project):
    JSON_FILE = "rim.json"
    PRODUCTION_LOCATIONS = [Location.SAS, Location.MAN, Location.VLA]

    def __init__(self, location, gencfg_transport, topology):
        super(Rim, self).__init__(gencfg_transport, topology)
        self.PROJECT_OPTS = self._read_json(self.JSON_FILE)
        self.NAME = self.PROJECT_OPTS["opts"]["name"]
        self.LOCATION = location
        self.regexpCommonLocations = COMMON_REGEXP_LOCATIONS + [
            ('slb-check', {'match_fsm': OrderedDict([('URI', '/ping')])}, [
                (Modules.Balancer, {
                    'balancer_type': 'rr',
                    'attempts': 1,
                    'balancer_options': OrderedDict([(
                        'weights_file',
                        LuaGlobal('slbWeightFile', 'controls/slb-ng.weight')
                    )]),
                    'custom_backends': [
                        (1, 'online_{}'.format(self.LOCATION), [
                            (Modules.ErrorDocument, {'status': 200}),
                        ]),
                    ],
                })
            ])
        ]


class RimPriemka(Rim):
    JSON_FILE = "rim-priemka.json"
    PRODUCTION_LOCATIONS = [Location.SAS]

    def __init__(self, location, gencfg_transport, topology):
        super(Rim, self).__init__(gencfg_transport, topology)
        self.PROJECT_OPTS = self._read_json(self.JSON_FILE)
        self.NAME = self.PROJECT_OPTS["opts"]["name"]
        self.LOCATION = location
        self.regexpCommonLocations = COMMON_REGEXP_LOCATIONS + [
            ('slb-check', {'match_fsm': OrderedDict([('URI', '/check.gif')])}, [
                (Modules.Balancer, {
                    'balancer_type': 'rr',
                    'attempts': 1,
                    'balancer_options': OrderedDict([(
                        'weights_file',
                        LuaGlobal('slbWeightFile', 'controls/slb-ng.weight')
                    )]),
                    'custom_backends': [
                        (1, 'online_{}'.format(self.LOCATION), [
                            (Modules.ErrorDocument, {'status': 200}),
                        ]),
                    ],
                })
            ])
        ]


class Improxy(Project):
    JSON_FILE = "improxy.json"
    PRODUCTION_LOCATIONS = [Location.SAS, Location.MAN, Location.VLA]

    def __init__(self, location, gencfg_transport, topology):
        super(Improxy, self).__init__(gencfg_transport, topology)
        self.PROJECT_OPTS = self._read_json(self.JSON_FILE)
        self.NAME = self.PROJECT_OPTS["opts"]["name"]
        self.LOCATION = location
        self.regexpCommonLocations = COMMON_REGEXP_LOCATIONS + [
            ('slb-check', {'match_fsm': OrderedDict([('URI', '/check.gif')])}, [
                (Modules.Balancer, {
                    'balancer_type': 'rr',
                    'attempts': 1,
                    'balancer_options': OrderedDict([(
                        'weights_file',
                        LuaGlobal('slbWeightFile', 'controls/slb-ng.weight')
                    )]),
                    'custom_backends': [
                        (1, 'online_{}'.format(self.LOCATION), [
                            (Modules.ErrorDocument, {
                                'status': 200,
                                'base64': 'R0lGODlhAQABAIABAAAAAP///yH5BAEAAAEALAAAAAABAAEAAAICTAEAOw=='
                            }),
                        ]),
                    ],
                })
            ]),
            ('active-check', {'match_fsm': OrderedDict([('URI', '/active_check')])}, [
                (Modules.Balancer, {
                    'balancer_type': 'rr',
                    'attempts': 1,
                    'balancer_options': OrderedDict([('weights_file', LuaGlobal('slbWeightFile', 'controls/slb-ng.weight'))]),
                    'custom_backends': [
                        (1, 'online_{}_weighted'.format(self.LOCATION), [
                            (Modules.ActiveCheckReply, {
                                'default_weight': 10,
                                'weight_file': './controls/l7_noc_check_weight',
                                'use_header': True,
                                'use_body': True
                            }),
                       ])
                   ]
               })
            ]),
        ]

class ImproxyPrestable(Project):
    JSON_FILE = "improxy-prestable.json"
    PRODUCTION_LOCATIONS = [Location.SAS, Location.MAN, Location.VLA]

    def __init__(self, location, gencfg_transport, topology):
        super(ImproxyPrestable, self).__init__(gencfg_transport, topology)
        self.PROJECT_OPTS = self._read_json(self.JSON_FILE)
        self.NAME = self.PROJECT_OPTS["opts"]["name"]
        self.LOCATION = location
        self.regexpCommonLocations = COMMON_REGEXP_LOCATIONS + [
            ('slb-check', {'match_fsm': OrderedDict([('URI', '/check.gif')])}, [
                (Modules.Balancer, {
                    'balancer_type': 'rr',
                    'attempts': 1,
                    'balancer_options': OrderedDict([(
                        'weights_file',
                        LuaGlobal('slbWeightFile', 'controls/slb-ng.weight')
                    )]),
                    'custom_backends': [
                        (1, 'online_{}'.format(self.LOCATION), [
                            (Modules.ErrorDocument, {
                                'status': 200,
                                'base64': 'R0lGODlhAQABAIABAAAAAP///yH5BAEAAAEALAAAAAABAAEAAAICTAEAOw=='
                            }),
                        ]),
                    ],
                })
            ]),
            ('active-check', {'match_fsm': OrderedDict([('URI', '/active_check')])}, [
                (Modules.Balancer, {
                    'balancer_type': 'rr',
                    'attempts': 1,
                    'balancer_options': OrderedDict([('weights_file', LuaGlobal('slbWeightFile', 'controls/slb-ng.weight'))]),
                    'custom_backends': [
                        (1, 'online_{}_weighted'.format(self.LOCATION), [
                            (Modules.ActiveCheckReply, {
                                'default_weight': 10,
                                'weight_file': './controls/l7_noc_check_weight',
                                'use_header': True,
                                'use_body': True
                            }),
                       ])
                   ]
               })
            ]),
        ]

class ImproxyPriemka(Project):
    JSON_FILE = "improxy-priemka.json"
    PRODUCTION_LOCATIONS = [Location.SAS]

    def __init__(self, location, gencfg_transport, topology):
        super(ImproxyPriemka, self).__init__(gencfg_transport, topology)
        self.PROJECT_OPTS = self._read_json(self.JSON_FILE)
        self.NAME = self.PROJECT_OPTS["opts"]["name"]
        self.LOCATION = location
        self.regexpCommonLocations = COMMON_REGEXP_LOCATIONS + [
            ('slb-check', {'match_fsm': OrderedDict([('URI', '/check.gif')])}, [
                (Modules.Balancer, {
                    'balancer_type': 'rr',
                    'attempts': 1,
                    'balancer_options': OrderedDict([(
                        'weights_file',
                        LuaGlobal('slbWeightFile', 'controls/slb-ng.weight')
                    )]),
                    'custom_backends': [
                        (1, 'online_{}'.format(self.LOCATION), [
                            (Modules.ErrorDocument, {
                                'status': 200,
                                'base64': 'R0lGODlhAQABAIABAAAAAP///yH5BAEAAAEALAAAAAABAAEAAAICTAEAOw=='
                            }),
                        ]),
                    ],
                })
            ]),
            ('active-check', {'match_fsm': OrderedDict([('URI', '/active_check')])}, [
                (Modules.Balancer, {
                    'balancer_type': 'rr',
                    'attempts': 1,
                    'balancer_options': OrderedDict([('weights_file', LuaGlobal('slbWeightFile', 'controls/slb-ng.weight'))]),
                    'custom_backends': [
                        (1, 'online_{}_weighted'.format(self.LOCATION), [
                            (Modules.ActiveCheckReply, {
                                'default_weight': 10,
                                'weight_file': './controls/l7_noc_check_weight',
                                'use_header': True,
                                'use_body': True
                            }),
                       ])
                   ]
               })
            ]),
        ]


class Cbrd(Project):
    JSON_FILE = "cbrd.json"
    PRODUCTION_LOCATIONS = [Location.SAS, Location.MAN, Location.VLA]

    def __init__(self, location, gencfg_transport, topology):
        super(Cbrd, self).__init__(gencfg_transport, topology)
        self.PROJECT_OPTS = self._read_json(self.JSON_FILE)
        self.NAME = self.PROJECT_OPTS["opts"]["name"]
        self.LOCATION = location
        self.CONNECT_TIMEOUT = '100ms'
        self.BACKEND_TIMEOUT = '1s'
        self.regexpCommonLocations = [
                                         ('index.html', {'match_fsm': OrderedDict([('URI', '/index.html',)])}, [
                                             (Modules.ErrorDocument, {'status': 200,
                                                                      'content': '<html><head><title>Feature Extraction Test</title></head>\\n'
                                                                                 '<body>\\n'
                                                                                 '<h4>Feature Extraction Test. More info on <a href=\\"http://wiki.yandex-team.ru/computervision/projects/DuplicatesByExample\\">project page</a></h4>\\n'
                                                                                 '<form method=\\"POST\\" enctype=\\"multipart/form-data\\" action=\\"\\">\\n'
                                                                                 '<input type=\\"radio\\" name=\\"sigtype\\" value=\\"info\\" checked/> image info\\n'
                                                                                 '<input type=\\"radio\\" name=\\"sigtype\\" value=\\"crc\\"/> image crc\\n'
                                                                                 '<input type=\\"radio\\" name=\\"sigtype\\" value=\\"dscr\\"/> image dscr<br/>\\n'
                                                                                 'File to upload: <input type=file name=upfile>\\n'
                                                                                 '<input type=submit value=Upload>\\n'
                                                                                 '</form>\\n'
                                                                                 '</body>\\n'
                                                                                 '</html>\\n'}),
                                         ])
                                     ]


class Commercial(Project):
    JSON_FILE = "commercial.json"
    PRODUCTION_LOCATIONS = [Location.VLA, Location.SAS, Location.MAN]

    def __init__(self, location, gencfg_transport, topology):
        super(Commercial, self).__init__(gencfg_transport, topology)
        self.PROJECT_OPTS = self._read_json(self.JSON_FILE)
        self.NAME = self.PROJECT_OPTS["opts"]["name"]
        self.LOCATION = location
        self.CONNECT_TIMEOUT = '10ms'
        self.BACKEND_TIMEOUT = '50ms'
        self.regexpCommonLocations = COMMON_REGEXP_LOCATIONS


class CommercialHamster(Project):
    JSON_FILE = "commercial-hamster.json"
    PRODUCTION_LOCATIONS = [Location.SAS, Location.MAN]

    def __init__(self, location, gencfg_transport, topology):
        super(CommercialHamster, self).__init__(gencfg_transport, topology)
        self.PROJECT_OPTS = self._read_json(self.JSON_FILE)
        self.NAME = self.PROJECT_OPTS["opts"]["name"]
        self.LOCATION = location
        self.CONNECT_TIMEOUT = '10ms'
        self.BACKEND_TIMEOUT = '50ms'
        self.regexpCommonLocations = COMMON_REGEXP_LOCATIONS


if __name__ == '__main__':
    rim_subprojects = [
        'rim_%s' %
        x for x in Rim.PRODUCTION_LOCATIONS]
    rim_priemka_subproject = [ "rim_priemka" ]
    cbrd_subprojects = [
        'cbrd_%s' %
        x for x in Cbrd.PRODUCTION_LOCATIONS]
    commercial_subprojects = [
        'commercial_%s' %
        x for x in Commercial.PRODUCTION_LOCATIONS]
    commercial_hamster_subprojects = [
        'commercial-hamster_%s' %
        x for x in CommercialHamster.PRODUCTION_LOCATIONS]

    all_subprojects = rim_subprojects + rim_priemka_subproject + cbrd_subprojects + \
        commercial_subprojects + commercial_hamster_subprojects

    parser = argparse.ArgumentParser(description='Generate config')
    parser.add_argument(
        '-r',
        '--raw',
        action='store_true',
        default=False,
        help='Generate intermediate python representation, useful for reclustering')
    parser.add_argument('--topology', type=str, required=False, default='trunk', help="Topology version")
    parser.add_argument('-t', '--transport', type=str, required=True,
                        choices=TRANSPORTS,
                        help="Obligatory. Transport for generation")
    parser.add_argument('subproject', choices=all_subprojects)
    parser.add_argument('output', metavar='<output file>')

    options = parser.parse_args()

    gencfg_transport = ""
    if options.transport == "curdb":
        sys.path.append(
            os.path.dirname(
                os.path.dirname(
                    os.path.dirname(
                        os.path.dirname(
                            os.path.dirname(
                                os.path.abspath(__file__)))))))
        import gencfg
        from core.db import CURDB
        from src.transports.curdb_transport import CurdbTransport

        gencfg_transport = CurdbTransport()
    elif options.transport == "api":
        from src.transports.gencfg_api_transport import GencfgApiTransport

        gencfg_transport = GencfgApiTransport(req_timeout=50, attempts=3)

    cfg = None
    project, location = options.subproject.split('_')

    if project == 'rim':
        if location == Location.PRIEMKA:
            location = Location.SAS
            project = RimPriemka(location, gencfg_transport, options.topology)
        else:
            project = Rim(location, gencfg_transport, options.topology)
    elif project == 'commercial':
        project = Commercial(location, gencfg_transport, options.topology)
    elif project == 'commercial-hamster':
        project = CommercialHamster(location, gencfg_transport, options.topology)
    elif project == 'cbrd':
        project = Cbrd(location, gencfg_transport, options.topology)
    else:
        raise Exception(
            'Unknown project %s, location %s' %
            (project.NAME, location))

    #try:
    cfg = project.balancer(project.PROJECT_OPTS, location)
    #except:
    #    err = sys.exc_info()[0]
    #    raise Exception(
    #        "Some errors while generating config {} for location {}: {}".format(project, location, err)
    #    )
    if cfg is None:
        raise Exception(
            'Generator return empty config for %s, location %s' %
            (project, location)
        )

    generate(
        cfg,
        options.output,
        options.raw,
        instance_db_transport=project.TRANSPORT,
        user_config_prefix=project.PREFIX)

