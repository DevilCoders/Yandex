#!/skynet/python/bin/python

import argparse
import collections
import copy
import math
import os
import requests
import sys
import time
import json

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
from src.lua_globals import LuaGlobal, LuaAnonymousKey, LuaFuncCall, lua_global_to_string
from src.groupmap import TGroupToTagMapping

TRANSPORTS = [
    "curdb",
    "api",
]

CGI_PARAMETERS_RE_STR = r'(&.*)?'

error_document_404 = (
    Modules.ErrorDocument, {
        'status': 404,
        'content': 'Thumbnail not found\\n'
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
        (Modules.ErrorDocument, {'status': 404, 'content': ''}),
    ]),
]


class Location:
    SAS = 'sas'
    MAN = 'man'
    VLA = 'vla'


class PortNames:
    SERVICE = 'ImproxyTunPort'
    SERVICE_SSL = 'ImproxySSLTunPort'
    IMPROXY = 'ImproxyPort'
    IMPROXY_SSL = 'ImproxySSLPort'
    CACHER = 'CacherPort'
    QUERY_SEARCH = 'QuerySearchPort'


def merge_dicts(first, second):
    # FIXME: bad implementation
    if first and second:
        res = OrderedDict()
        for key, value in first.items() + second.items():
            if key not in res:
                res[key] = value
        return res
    else:
        return first


class Project(object):
    CDN_LOCATIONS = []

    def _set_group_topology(self, backends):
        if self.transport.name() == 'curdb':
            return backends

        if isinstance(backends, collections.Sequence) and not isinstance(backends, basestring):
            return ['{}:{}'.format(x, self.topology) for x in backends]
        else:
            return '{}:{}'.format(backends, self.topology)

    def _port_for_group(self, group_name):
        return self.transport.get_group_instances(group_name)[0].port

    def _getBalancerPort(self, location):
        improxy_group = self._set_group_topology(self._set_improxy_backends()[location]['improxy']['name'])
        return self._port_for_group(improxy_group)

    def _computeIntlookups(self, intlookup):
        """
        Replace intlookup names with actual intlookups
        """
        intlookup_list = []
        if self.transport.name() == 'curdb':
            intlookup_list = [CURDB.intlookups.get_intlookup(x) for x in intlookup]
        else:
            for x in intlookup:
                intlookup_list.append(Utils.ErsatzIntlookup(x, self.topology))
        if not intlookup_list:
            print >> sys.stderr, "No intlookups configured for bundle %s" % intlookup
            exit(1)
        shard_count_list = [x.get_shards_count() for x in intlookup_list]
        if not all([x == shard_count_list[0] for x in shard_count_list]):
            print >> sys.stderr, "Different number of shards in intlookups for bundle %s" % intlookup
            exit(1)
        return intlookup_list

    def _set_experiment_backends(self):
        pass

    def _fixedShardNumber(self, shard_number, total_shards):
        ln = 1 + int(math.log10(total_shards))
        shard_number_str = str(shard_number)
        return shard_number_str.rjust(ln, '0')

    def _generateBalancerOptions(
            self,
            shard_number,
            shard_type,
            intlookup_list,
            is_shift_ports,
            tier_balancer):
        backend_list = []
        for intlookup in intlookup_list:
            backend_list = backend_list + map(
                lambda x: '%s:%s:%s(domain=.search.yandex.net,resolve=6)' %
                (x.host.name, x.port, x.power), intlookup.get_base_instances_for_shard(shard_number))
        balancerOptions = {
            'backends': backend_list,
            'attempts': len(backend_list),
            '2tier_balancer': tier_balancer,
            'resolve_protocols': [6],
            'no_statistics': True,
            'proxy_options': OrderedDict([
                ('connect_timeout', LuaGlobal('connect_timeout', self.CONNECT_TIMEOUT)),
                ('backend_timeout', LuaGlobal('backend_timeout', self.BACKEND_TIMEOUT)),
                ('keepalive_count', LuaGlobal('keepalive_count', 1)),
                ('buffering', LuaGlobal('buffering', False)),
                ('need_resolve', LuaGlobal('resolveBackends', True)),
            ]),
            'shift_ports': is_shift_ports
        }

        return balancerOptions

    def _generateSingleShardConfig(
            self,
            shard_type,
            shard_number,
            total_shards,
            intlookup_list,
            is_shift_ports,
            tier_balancer=False):
        balancerOptions = self._generateBalancerOptions(
            shard_number, shard_type, intlookup_list, is_shift_ports, tier_balancer)
        submodules = [(Modules.Balancer, balancerOptions)]

        return submodules

    def _generateConsistentHashingLocations(
            self,
            shard_type,
            intlookup_list,
            is_shift_ports,
            regexp_template,
            rewrite,
            tier_balancer=False):

        N = intlookup_list[0].get_shards_count()

        # Add cache for thumb_consistent_hash in order to not generate same
        # things multiple times
        if not hasattr(self, 'consistentHashingCache'):
            self.consistentHashingCache = dict()
        cache_id = (shard_type,
                    tuple(map(lambda x: x.file_name,
                              intlookup_list)),
                    is_shift_ports,
                    regexp_template['url'],
                    tier_balancer)
        if cache_id not in self.consistentHashingCache:
            thumb_locations = [
                (self._fixedShardNumber(
                    i,
                    N),
                    self._generateSingleShardConfig(
                    shard_type,
                    i,
                    N,
                    intlookup_list,
                    is_shift_ports,
                    tier_balancer)) for i in range(N)]

            self.consistentHashingCache[cache_id] = [
                (Modules.ThumbConsistentHash, {
                    'id_regexp': r'id=([a-fA-F0-9]+)',
                    'locations': thumb_locations,
                    'default': [error_document_404]
                })]

        submodules = self.consistentHashingCache[cache_id]

        if rewrite:
            submodules = [
                (Modules.Rewrite, {'actions': [({
                    'regexp': regexp_template['url'],
                    'rewrite': rewrite,
                    'split': 'url'
                })]}
                )] + submodules

        return submodules

    def _generateShardRegexpsLocations(
            self,
            shard_type,
            intlookup_list,
            is_shift_ports,
            regexp_template,
            rewrite,
            tier_balancer=False):
        result = []

        N = intlookup_list[0].get_shards_count()
        for i in range(N):
            shard_result = self._generateSingleShardConfig(
                shard_type, i, N, intlookup_list, is_shift_ports)
            regexp = regexp_template['url'] % {
                'shard_number': i, 'total_shards': N}
            if rewrite:
                shard_result = [
                    (Modules.Rewrite, {'actions': [
                        ({'regexp': regexp, 'rewrite': rewrite, 'split': 'url'})]}),
                    shard_result]
            shard_result = (
                'shard-%s-%s-%s' %
                (shard_type, N, self._fixedShardNumber(
                    i, N)), {
                    'match_fsm': OrderedDict(
                        [
                            ('url', regexp)])}, shard_result)
            result.append(shard_result)

        return [(Modules.Regexp, result)]

    def _generatePlainLocation(
            self,
            index_type,
            backends_list,
            is_shift_ports,
            regexp_template,
            rewrite,
            tier_balancer=False):
        result = [
            (Modules.Balancer, {
                'backends': backends_list,
                'attempts': LuaGlobal('backend_attempt', 2),
                '2tier_balancer': tier_balancer,
                'resolve_protocols': [6],
                'proxy_options': OrderedDict([
                    ('connect_timeout', LuaGlobal('connect_timeout', self.CONNECT_TIMEOUT)),
                    ('backend_timeout', LuaGlobal('backend_timeout', self.BACKEND_TIMEOUT)),
                    ('keepalive_count', LuaGlobal('keepalive_count', 1)),
                    ('buffering', LuaGlobal('buffering', True)),
                    ('need_resolve', LuaGlobal('resolveBackends', True)),
                ])
            })]
        return result

    def _get_cache_client(self, id_regexp, backends, stats_attr, child_config):
        return (Modules.CacheClient, {
            'id_regexp': id_regexp,
            'server': [
                (Modules.Report, {
                    'uuid': stats_attr,
                    'ranges': Constants.ALL_REPORT_TIMELINES,
                    'disable_robotness': True,
                    'disable_sslness': True,
                }),
                (Modules.CgiHasher, {
                    'parameters': ['id', 'n', 'enc', 'q', 'w', 'h'],
                    'randomize_empty_match': False
                }),
                (Modules.Balancer, {
                    'balancer_type': 'consistent_hashing',
                    'backends': backends,
                    'attempts': 2,
                    'resolve_protocols': [6],
                    'proxy_options': OrderedDict([
                        ('keepalive_count', LuaGlobal('cacher_keepalive_count', 16)),
                        ('connect_timeout', LuaGlobal('cacher_connect_timeout', '25ms')),
                        ('backend_timeout', LuaGlobal('cacher_backend_timeout', '90ms')),
                    ]),
                }),
            ],
            'module': child_config,
        })

    def _generate_cacher_subcollections(
            self,
            collections,
            subcollections,
            id_regexp,
            child_config):
        res = [
            ('default', {}, [
                error_document_404,
            ]),
        ]
        for collection_name in subcollections:
            backends = self._set_group_topology(collections[collection_name]['backends'])
            regexp = self._set_index_types()[collection_name]['regexp']
            stats_attr = self._set_index_types()[collection_name]['stats_attr']
            res.append(('{}_cacher'.format(collection_name),
                        {'match_fsm': regexp},
                        [(Modules.Report,
                          {'uuid': stats_attr,
                           'ranges': Constants.ALL_REPORT_TIMELINES,
                           'disable_robotness': True,
                           'disable_sslness': True,
                           }),
                         self._get_cache_client(id_regexp,
                                                backends,
                                                '{}_{}'.format(self.CACHER_STATS_ATTR,
                                                               collection_name),
                                                child_config)]))
        return (Modules.Regexp, res)

    def _generateCacherConfig(self, child_config, locations, opts, shard_type):
        '''
            Return config with distributed cache
        '''
        priority = 1 if self.location not in opts else 0
        res = [
            (priority, 'disable_cacher',
             child_config,
             ),
        ]
        for loc in locations:
            if loc in opts:
                collections = opts[loc]['collections']
                id_regexp = opts[loc]['id_regexp']
                if 'subcollections' in collections[shard_type]:
                    subcollections = collections[shard_type][
                        'subcollections'] + [shard_type]
                    cache_module = self._generate_cacher_subcollections(
                        collections, subcollections, id_regexp, child_config)
                else:
                    backends = self._set_group_topology(collections[shard_type]['backends'])
                    stats_attr = "{}_{}".format(
                        opts[loc]['stats_attr'], shard_type)
                    cache_module = self._get_cache_client(
                        id_regexp, backends, stats_attr, child_config)
                priority = 1 if self.location == loc else 0
                res.append(
                    (priority, loc + '_cacher', [
                        cache_module
                    ]),
                )
        res = [
            (Modules.Balancer, {
                'balancer_type': 'rr',
                'attempts': 1,
                'balancer_options': OrderedDict(
                    [('weights_file', LuaGlobal('cacherWeightFile', self.ITS_PATH + 'cacher.weight'))]),
                'custom_backends': res
            })
        ]
        return res

    def _genSSL(self, ssl, sha1=True):
        '''
            Return SSL block
        '''
        if 'prefix' in ssl:
            prefix = ssl['prefix']
        else:
            prefix = ''
        result = {
            'cert': LuaGlobal('{0}SSLCert'.format(prefix), ssl['cert']),
            'priv': LuaGlobal('{0}SSLCertKey'.format(prefix), ssl['certKey']),
            'ciphers': Constants.SSL_CIPHERS_SUITES_SHA2,
            'http2_alpn_file': self.ITS_PATH + 'http2_enable.ratefile',
            'http2_alpn_freq': 0.0,
            'disable_tlsv1_3': False,
        }
        events = []
        if 'ocsp' in ssl:
            result['ocsp'] = LuaGlobal('{0}OCSP'.format(prefix), ssl['ocsp'])
            events = events + [('reload_ocsp_response', 'reload_ocsp')]
        if 'tls_tickets' in ssl:
            result['ticket_keys_list'] = OrderedDict([
                ('tls_1stkey', OrderedDict([
                    ('keyfile', LuaGlobal('{0}Ticket1stKey'.format(prefix), ssl['tls_tickets']['ticket1stKey'])),
                    ('priority', 1000),
                ])),
                ('tls_2stkey', OrderedDict([
                    ('keyfile', LuaGlobal('{0}Ticket2stKey'.format(prefix), ssl['tls_tickets']['ticket2stKey'])),
                    ('priority', 999),
                ])),
            ])
            events = events + [('reload_ticket_keys', 'reload_ticket')]
        if events:
            result['events'] = OrderedDict(events)
        # sha1
        if sha1:
            result['secondary'] = OrderedDict([
                ('cert', LuaGlobal('{0}SSLCertSHA1'.format(prefix), ssl['cert_sha1'])),
                ('priv', LuaGlobal('{0}SSLCertKeySHA1'.format(prefix), ssl['certKey_sha1'])),
            ])
        return [(Modules.SslSni, result)]

    def _generateBackendsWeight(
            self,
            IndexTypes,
            ThumbBackends,
            ExperimentalThumbBackends,
            locations,
            primal_location,
            enable_backup=True,
            prefix='',
            tier_balancer=False,
            index_filter=None,
            default_priority=100):
        '''
            Return backends blocks with full mesh balancing
        '''
        backends = {}
        devnull_backend = [(0, 'disable', [error_document_502])]
        for location in locations:
            shard_types = ThumbBackends[location]
            for shard_type, opts in shard_types.iteritems():
                if index_filter and index_filter != shard_type:
                    continue
                if opts['enable']:
                    if isinstance(opts['name'], list):
                        intlookup_list = copy.deepcopy(opts['name'])
                    elif isinstance(opts['name'], str):
                        intlookup_list = [opts['name']]
                    else:
                        raise('Bad type of backend list')
                    if IndexTypes[shard_type]['sharded']:
                        intlookup = self._computeIntlookups(intlookup_list)
                    else:
                        intlookup = intlookup_list
                    if shard_type not in backends:
                        backends[shard_type] = intlookup
                    else:
                        backends[shard_type] = backends[shard_type] + intlookup
        for shard_type, intlookup in backends.items():
            shardLocationsFunc = IndexTypes[shard_type]['shardFunc']
            regexp = IndexTypes[shard_type]['regexp']
            stats_attr = IndexTypes[shard_type]['stats_attr']
            weight_file = LuaGlobal(
                '{0}{1}WeightFile'.format(
                    prefix, shard_type), IndexTypes[shard_type][
                    '{0}weight_file'.format(prefix)])
            regexp_backends = shardLocationsFunc(
                shard_type,
                intlookup,
                opts['is_shift_ports'],
                regexp,
                IndexTypes[shard_type].get('rewrite', False),
                tier_balancer)
            regexp_backends = [
                (100, 'enable'.format(shard_type),
                 regexp_backends
                 )
            ]
            regexp_config = [
                (Modules.Report, {
                    'uuid': '{0}{1}'.format(prefix, stats_attr),
                    'ranges': Constants.ALL_REPORT_TIMELINES,
                    'disable_robotness': True,
                    'disable_sslness': True,
                }),
                (Modules.Balancer, {
                    'balancer_type': 'rr',
                    'attempts': 1,
                    'balancer_options': OrderedDict([('weights_file', weight_file)]),
                    'custom_backends': devnull_backend + regexp_backends,
                })
            ]
            yield shard_type, regexp_config

    def _backends_rr(
            self,
            index_type,
            index_opts,
            all_backends,
            locations,
            primal_location,
            default_priority):
        '''
            Return backends blocks for provided circuit and geo balancing
        '''
        # devnull location
        devnull_backend = (0, index_type + '_disable', [error_document_502])
        res = [devnull_backend]
        # traverse by each location
        for location in locations:
            if location not in all_backends:
                continue
            # set default priority for home location
            if primal_location:
                priority = default_priority if primal_location == location else 0
            else:
                priority = default_priority / len(locations)
            try:
                backend_opts = all_backends[location][index_type]
            except KeyError as e:
                continue
            if backend_opts['enable']:
                stats_attr = backend_opts['stats_attr']
                shardLocationsFunc = index_opts['shardFunc']
                regexp = index_opts['regexp']
                if isinstance(backend_opts['name'], list):
                    intlookup_list = copy.deepcopy(backend_opts['name'])
                elif isinstance(backend_opts['name'], str):
                    intlookup_list = [backend_opts['name']]
                else:
                    raise('Bad type of backend list')
                if index_opts['sharded']:
                    intlookup = self._computeIntlookups(intlookup_list)
                else:
                    intlookup = intlookup_list
                shard_locations = shardLocationsFunc(
                    index_type,
                    intlookup,
                    backend_opts['is_shift_ports'],
                    regexp,
                    index_opts.get('rewrite', False),
                    )
                res.append(
                    (priority, index_type + '_' + location, [
                        (Modules.Report, {
                            'uuid': stats_attr,
                            'ranges': Constants.ALL_REPORT_TIMELINES,
                            'disable_robotness': True,
                            'disable_sslness': True,
                        }),
                    ] + shard_locations)
                )
        return res

    def _generateBackendsRR(
            self,
            IndexTypes,
            Backends,
            ExperimentalBackends,
            locations,
            primal_location,
            enable_backup=True,
            prefix='',
            tier_balancer=False,
            index_filter=None,
            default_priority=100):
        '''
            Return thumbnail backends blocks for each provided circuit and geo balancing
        '''
        # backup
        fallback_block = {}
        if enable_backup:
            fallback_locations = [
                x for x in locations if (
                    x != primal_location and (
                        not ExperimentalBackends or x not in ExperimentalBackends))]
            for index_name, fallback_stuff in self._generateBackendsWeight(IndexTypes, Backends, [],
                                                                           fallback_locations, primal_location=None,
                                                                           enable_backup=False, prefix='fallback_',
                                                                           tier_balancer=False,
                                                                           index_filter=index_filter):
                fallback_block[index_name] = fallback_stuff
        for index_name, backends_opts in Backends[primal_location].iteritems():
            if index_filter and index_filter != index_name:
                continue
            index_opts = IndexTypes[index_name]
            # where index type is exists
            index_locations = locations
            # add experiment locations
            if not self.DISABLE_ALL_EXPERIMENTS and ExperimentalBackends:
                for location, backends in ExperimentalBackends.iteritems():
                    if index_name in backends and location not in index_locations:
                        index_locations.append(location)
                        if location not in Backends:
                            Backends[location] = backends
                        else:
                            raise "Couldn't add experimental location %s: the same name already used"
            # get custom backends
            custom_backends = self._backends_rr(
                index_name,
                index_opts,
                Backends,
                index_locations,
                primal_location,
                default_priority)
            stats_attr = '{0}{1}'.format(prefix, index_opts['stats_attr'])
            weight_file = LuaGlobal(
                '{0}{1}WeightFile'.format(
                    prefix, index_name), index_opts[
                    '{0}weight_file'.format(prefix)])
            balancer_config = {
                'balancer_type': 'rr',
                'attempts': 1,
                'balancer_options': OrderedDict([('weights_file', weight_file)]),
                'custom_backends': custom_backends,
            }

            # backup location
            if index_name in fallback_block and not Backends[
                    primal_location][index_name]['disable_fallback']:
                balancer_config['on_error'] = fallback_block[index_name]
            regexp_config = [
                (Modules.Report, {
                    'uuid': stats_attr,
                    'ranges': Constants.ALL_REPORT_TIMELINES,
                    'disable_robotness': True,
                    'disable_sslness': True,
                }),
                (Modules.Balancer, balancer_config)
            ]
            yield index_name, regexp_config

    def _generateBanConfig(self, config):
        '''
            Return block with DMCA system
        '''
        stats_attr = self.BACKEND_STATS_ATTR + '_ban'
        backends = self._set_group_topology('{}_{}_IMPROXY_BAN'.format(location.upper(), self.name.upper()))
        querySearchPortLuaGlobal = LuaGlobal(
            PortNames.QUERY_SEARCH, self._port_for_group(backends))
        ban_config = [
            (Modules.ThumbsBan, {
                'id_regexp': r'\\?.*id=([a-fA-F0-9]+)',
                'checker': [
                    (Modules.Report, {
                        'uuid': stats_attr,
                        'ranges': Constants.ALL_REPORT_TIMELINES,
                        'disable_robotness': True,
                        'disable_sslness': True,
                    }),
                    (Modules.Balancer, {
                        'stats_attr': "{}_ban".format(self.BACKEND_STATS_ATTR),
                        'attempts': 1,
                        'balancer_type': 'rr',
                        'custom_backends': [
                            (100.0, 'localhost', [
                                (Modules.Proxy, {
                                    'host': 'localhost',
                                    'port': querySearchPortLuaGlobal,
                                    'connect_timeout': '10ms',
                                    'backend_timeout': '10ms',
                                    'keepalive_count': 4
                                }),
                            ]),
                        ],
                        'attempts': 1,
                        'on_error': [
                            (Modules.Balancer, {
                                'backends': [backends],
                                'attempts': LuaGlobal('ban_backend_attempt', 2),
                                'balancer_type': 'rr',
                                'resolve_protocols': [6],
                                'stats_attr': "{}_ban".format(self.BACKEND_STATS_ATTR),
                                'proxy_options': OrderedDict([
                                    ('connect_timeout', LuaGlobal('ban_connect_timeout', '50ms')),
                                    ('backend_timeout', LuaGlobal('ban_backend_timeout', '100ms')),
                                    ('keepalive_count', LuaGlobal('ban_keepalive_count', 8)),
                                    ('buffering', LuaGlobal('ban_buffering', False)),
                                    ('need_resolve', LuaGlobal('resolveBackends', True)),
                                ])
                            })
                        ],
                    }),
                ],
                'ban_handler': [
                    (Modules.ErrorDocument, {'status': 410}),
                ],
                'module': config
            }),
        ]
        return [
            (Modules.Balancer, {
                'balancer_type': 'rr',
                'attempts': 1,
                'balancer_options': OrderedDict(
                    [('weights_file', LuaGlobal('banWeightFile', self.ITS_PATH + 'ban.weight'))]
                ),
                'custom_backends': [
                    (100, 'ban_enable', ban_config),
                    (0, 'ban_disable', config)
                ]
            })
        ]

    def _generateHttpConfig(
            self,
            config,
            access_log,
            error_log,
            debug_log,
            stats_attr,
            https_config=False,
            http2=False):
        '''
            Return config with HTTP module
        '''
        http_config = [
            (Modules.Http, {
                'maxlen': 16 * 1024,
                'maxreq': 8 * 1024,
                'stats_attr': stats_attr,
                'keepalive': self.KEEPALIVE
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

        if http2:
            http_config = [
                (Modules.Http2, {
                    'goaway_debug_data_enabled': 0,
                    'debug_log_enabled': 1,
                    'debug_log_name': debug_log,
                    'debug_log_freq': 0.0,
                    'debug_log_freq_file': self.ITS_PATH + 'http2_debug.ratefile'
                })
            ] + http_config

        if https_config:
            http_config = https_config + http_config


        http_config = [
            (Modules.Errorlog, {'log': error_log})
        ] + http_config

        return http_config

    def _generate_ipDispatch(
            self,
            name,
            config,
            location,
            stats_attr,
            internal_port,
            service_port=None,
            domain='*'):
        '''
            Generate config for each ip
        '''
        if not domain == '*':
            _ips = [str(x) for x in self.transport.db.slbs.get(domain).ips]

            exclude_addrs = self.IGNORE_ADDRS.get(location, list())
            _ips = set(_ips) - set(exclude_addrs)
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

    def _config(
            self,
            primal_location,
            production_locations,
            cache,
            ban,
            rr=True,
            enable_backup=True,
            prefix=''):
        IndexTypes = self._set_index_types()
        Backends = self._set_backends()
        ExperimentalBackends = self._set_experiment_backends()

        # thumb daemons
        if rr:
            thdb_func = self._generateBackendsRR
        else:
            thdb_func = self._generateBackendsWeight

        for index_type, regexp_config in thdb_func(IndexTypes, Backends, ExperimentalBackends, production_locations,
                                                   primal_location, enable_backup=enable_backup, prefix=prefix):
            index_opts = IndexTypes[index_type]
            regexp = index_opts['regexp']
            if 'reply_file' in index_opts.keys():
                for _, sink_regexp_config in thdb_func(IndexTypes, Backends, ExperimentalBackends, production_locations,
                                                       primal_location, enable_backup=False, prefix='sink_', index_filter=index_type, default_priority=0):
                    sink_location = sink_regexp_config
                regexp_config = [
                    (Modules.RequestReplier, {
                        'rate_file': './controls/request_replier_{}.ratefile'.format(index_opts['reply_file']),
                        'sink': sink_location
                    })
                ] + regexp_config

            # distributed cache farm
            if cache:
                CacherBackends = self._set_cacher_backends()
                regexp_config = self._generateCacherConfig(
                    regexp_config, self.PRODUCTION_LOCATIONS, CacherBackends, index_type)
            # thumb bans
            if ban:
                regexp_config = self._generateBanConfig(regexp_config)

            # response headers
            webHeadersOpts = dict()
            if 'thdb_file' in index_opts.keys():
                webHeadersOpts['remove_X-Thdb-Version'] = True
                thdb_version_config = [
                    (Modules.ThdbVersion, {
                        'file_name': LuaGlobal(index_type + 'ThdbFile',
                                               index_opts['thdb_file']),
                        'file_read_timeout': '1s',
                    })
                ]

            webHeadersOpts['Vary'] = copy.deepcopy(index_opts.get('vary_headers', list()))

            regexp_config = [
                (Macroses.WebHeaders, webHeadersOpts),
                (Modules.ResponseHeaders, {'create': OrderedDict([('Access-Control-Allow-Origin', '*')])}),
            ] + thdb_version_config + regexp_config

            balancer_config = [
                (index_type, {'match_fsm': regexp}, [
                    (Modules.Report, {
                        'uuid': index_type,
                        'ranges': Constants.ALL_REPORT_TIMELINES,
                        'disable_robotness': True,
                        'disable_sslness': True,
                    })
                ] + regexp_config)
            ]
            yield balancer_config

    def _balancer(self):
        ImproxyBackends = self._set_improxy_backends()
        project_domains = ['*']
        if self.DOMAINS != ['*']:
            project_domains += self.DOMAINS
        config = self.regexpCommonLocations
        for balancer_config in self._config(
                cache=self.CACHE,
                ban=self.THUMBS_BAN,
                production_locations=self.PRODUCTION_LOCATIONS,
                primal_location=self.location,
                rr=self.RR,
                enable_backup=True):
            config = config + balancer_config

        config = [
            (Modules.Report, {
                'uuid': "{}".format(self.BALANCER_STATS_ATTR),
                'ranges': Constants.ALL_REPORT_TIMELINES,
                'disable_robotness': True,
                'disable_sslness': True,
                'matcher_map': self.PRJ_REFERENCES,
            }),
            (Modules.Regexp, config)
        ]

        # webp support (IMAGES-12425)
        if self.WEBP:
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
        # admin location
        for domain in project_domains:
            # HTTP
            if domain in project_domains:
                http_config = self._generateHttpConfig(
                    config, self.ACCESS_LOG, self.ERROR_LOG, self.DEBUG_LOG, self.BALANCER_STATS_ATTR)
                for ip_config in self._generate_ipDispatch(
                    'remote',
                    http_config,
                    location=self.location,
                    stats_attr=self.BALANCER_STATS_ATTR,
                    internal_port=self.INTERNAL_PORT,
                    service_port=self.HTTP_PORT,
                        domain=domain):
                    dispachers.append(ip_config)
            # HTTPS
            if self.SSL:
                if domain in project_domains:
                    https_section = self._genSSL(self.ssl, self.ENABLE_SHA1)
                    https_config = self._generateHttpConfig(
                        config,
                        self.ACCESS_LOG,
                        self.ERROR_LOG,
                        self.DEBUG_LOG,
                        self.BALANCER_STATS_ATTR_SSL,
                        https_section,
                        http2=True)
                    for ip_config in self._generate_ipDispatch(
                        'remote_ssl',
                        https_config,
                        location=self.location,
                        stats_attr=self.BALANCER_STATS_ATTR_SSL,
                        internal_port=self.INTERNAL_PORT_SSL,
                        service_port=self.HTTPS_PORT,
                            domain=domain):
                        dispachers.append(ip_config)

        unistat_port = LuaGlobal('port_localips', self.RAW_PORT) + 2
        unistat_addrs = None

        ipv4 = LuaFuncCall('GetIpByIproute', {'key': 'v4'})
        ipv6 = LuaFuncCall('GetIpByIproute', {'key': 'v6'})
        unistat_addrs = OrderedDict((
            (LuaAnonymousKey(), OrderedDict([('ip', ipv4), ('port', unistat_port)])),
            (LuaAnonymousKey(), OrderedDict([('ip', ipv6), ('port', unistat_port)])),
        ))

        config = [
            (Modules.Main, {
                'workers': self.WORKERS,
                'maxconn': 50000,
                'log': self.MASTER_LOG,
                'config_tag': self.transport.describe_gencfg_version(),
                'enable_reuse_port': LuaGlobal('enable_reuse_port', self.REUSE_PORT),
                'admin_port': self.INTERNAL_PORT,
                'cpu_limiter': OrderedDict([
                    ('cpu_usage_coeff', 0.05),
                    ('disable_file', self.ITS_PATH + 'cpu_limiter_disabled'),
                    ('enable_http2_drop', True),
                    ('disable_http2_file', self.ITS_PATH + 'cpu_limiter_http2_disabled'),
                    ('http2_drop_hi', 0.75),
                    ('http2_drop_lo', 0.5),
                    ('enable_keepalive_close', True),
                    ('keepalive_close_lo', 0.75),
                    ('keepalive_close_hi', 1.0),
                    ('enable_conn_reject', True),
                    ('conn_reject_lo', 0.75),
                    ('conn_reject_hi', 1.0),
                    ('conn_hold_count', 10000),
                    ('conn_hold_duration', '10s')
                ]),
                'unistat': OrderedDict([
                    ('addrs', LuaGlobal('instance_unistat_addrs', unistat_addrs)),
                    ('hide_legacy_signals', False)
                ])
            }),
            (Modules.Ipdispatch, dispachers),
        ]
        return config

    def __init__(self, transport, topology='trunk'):
        self.transport = transport
        self.topology = topology
        # workers ammount
        self.WORKERS = 4
        # to which addresses to bind
        self.DOMAINS = ['*']
        # static weights files
        self.ITS_PATH = 'controls/'
        # LogDir
        self.LOG_DIR = LuaGlobal('LogDir', "/place/db/www/logs/")
        # stats attr prefixes
        self.BALANCER_STATS_ATTR = 'improxy_balancer'
        self.BALANCER_STATS_ATTR_SSL = 'improxy_balancer_ssl'
        self.BACKEND_STATS_ATTR = 'improxy_backend'
        self.CACHER_STATS_ATTR = self.BACKEND_STATS_ATTR + '_cacher'
        self.RS = {
            Location.SAS: 'sas2-0088.search.yandex.net',
            Location.MAN: 'man1-4303-man-imgs-improxy-10080.gencfg-c.yandex.net',
            Location.VLA: 'vla1-4527.search.yandex.net'
        }
        # some embedded lua functions that run during start instance
        self.PREFIX = Constants.GetIpByIproute
        self.IGNORE_ADDRS = {}
        self.KEEPALIVE = 1
        self.SSL = True
        self.ENABLE_SHA1 = True
        self.CACHE = True
        self.THUMBS_BAN = False
        # TUN SLB scheme
        self.HTTP_PORT = LuaGlobal(PortNames.SERVICE, 80)
        self.HTTPS_PORT = LuaGlobal(PortNames.SERVICE_SSL, 443)
        # disable all experiment locations for some reason
        self.DISABLE_ALL_EXPERIMENTS = False
        # TIMEOUTS
        self.CONNECT_TIMEOUT = '100ms'
        self.BACKEND_TIMEOUT = '1s'
        self.RR = True
        self.WEBP = False
        self.REUSE_PORT = False
        self.PRJ_REFERENCES = OrderedDict([])
        # Common regexps
        self.DEFAULT_LOCATION = (
            'default', {}, [
                error_document_404
            ])
        self.regexpCommonLocations = COMMON_REGEXP_LOCATIONS + [
            ('root', {'match_fsm': OrderedDict([('path', '/')])}, [
                (Modules.ResponseHeaders, {'create': OrderedDict([('Location', 'http://www.yandex.ru/')])}),
                (Modules.ErrorDocument, {'status': 302, 'content': '<title>302 Moved Temporarily</title></head>\\n'
                                                                   '<body bgcolor=\\"white\\">\\n'
                                                                   '<center><h1>302 Moved Temporarily</h1></center>\\n'
                                                                   '</body>\\n'
                                                                   '</html>\\n'}),
            ]),
        ]


class Images(Project):
    PRODUCTION_LOCATIONS = [Location.SAS, Location.MAN, Location.VLA]

    def _sslVars(self, ssl_port):
        self.ssl = {
            'certKey': 'secrets/key',
            'cert': 'secrets/cert',
            'tls_tickets': {
                'ticket1stKey': 'secrets/1st_images.sha256.tls.key',
                'ticket2stKey': 'secrets/2st_images.sha256.tls.key',
            },
            'certKey_sha1': 'secrets/rsa_key',
            'cert_sha1': 'secrets/rsa_cert',
        }
        # FIXME: ssl port is hardcored
        self.INTERNAL_PORT_SSL = LuaGlobal(PortNames.IMPROXY_SSL, ssl_port)

    def _set_index_types(self):
        return OrderedDict([
            ('imgsth', {
                'name': 'main',
                'regexp': OrderedDict([('url', r'/i\\?(.*&)?id=([a-fA-F0-9]+)(-\\d+-\\d+)?(-sr)?' + CGI_PARAMETERS_RE_STR)]),
                'stats_attr': self.BALANCER_STATS_ATTR + '_images_main',
                'weight_file': self.ITS_PATH + 'heavy.weight',
                'fallback_weight_file': self.ITS_PATH + 'imgsth_fallback.weight',
                'internal_weight_file': self.ITS_PATH + 'imgsth_internal.weight',
                'thdb_file': self.ITS_PATH + 'imgsth.version',
                'shardFunc': self._generateConsistentHashingLocations,
                'prefix': 'new',
                'sharded': True,
                'rewrite': '/i?id=%2&%1%5',
                'vary_headers': ['Accept-Encoding']
            }),
            ('imglth', {
                'name': 'large',
                'regexp': OrderedDict([('url', r'/i\\?(.*&)?id=([a-fA-F0-9]+)-(\\d+-\\d+)?(l)-?\\d*' + CGI_PARAMETERS_RE_STR)]),
                'stats_attr': self.BALANCER_STATS_ATTR + '_images_large',
                'weight_file': self.ITS_PATH + 'heavy.weight',
                'fallback_weight_file': self.ITS_PATH + 'imglth_fallback.weight',
                'internal_weight_file': self.ITS_PATH + 'imglth_internal.weight',
                'thdb_file': self.ITS_PATH + 'imglth.version',
                'shardFunc': self._generateConsistentHashingLocations,
                'sharded': True,
                'rewrite': '/i?id=%2&%1%5',
                'vary_headers': ['Accept-Encoding']
            }),
            ('imgsrlth', {
                'name': 'sr-large',
                'regexp': OrderedDict([('url', r'/i\\?(.*&)?id=([a-fA-F0-9]+)-(\\d+-\\d+)?(srl)-?\\d*' + CGI_PARAMETERS_RE_STR)]),
                'stats_attr': self.BALANCER_STATS_ATTR + '_images_sr-large',
                'weight_file': self.ITS_PATH + 'heavy.weight',
                'fallback_weight_file': self.ITS_PATH + 'imgsrlth_fallback.weight',
                'internal_weight_file': self.ITS_PATH + 'imgsrlth_internal.weight',
                'thdb_file': self.ITS_PATH + 'imgsrlth.version',
                'shardFunc': self._generateConsistentHashingLocations,
                'sharded': True,
                'rewrite': '/i?id=%2&%1%5',
                'vary_headers': ['Accept-Encoding']
            }),
            ('improxy', {
                'name': 'thumb',
                'regexp': OrderedDict([('URI', r'/(i|pinger)')]),
                'stats_attr': self.BACKEND_STATS_ATTR,
                'weight_file': self.ITS_PATH + 'heavy.weight',
                'fallback_weight_file': self.ITS_PATH + 'improxy_fallback.weight',
                'internal_weight_file': self.ITS_PATH + 'improxy_internal.weight',
                'shardFunc': self._generatePlainLocation,
                'sharded': False,
            }),
        ])

    def _set_experiment_backends(self):
        return OrderedDict([
            (Location.SAS + '_prism', {
                'imgfth': {
                    'name': 'SAS_IMGS_QUICK_THUMB_PRISM',
                    'disable_fallback': False,
                    'stats_attr': self.BACKEND_STATS_ATTR + '_imgfth_prism',
                    'enable': True,
                    'is_shift_ports': False,
                },
            }),
        ])

    def _set_backends(self):
        return OrderedDict([
            (Location.SAS, {
                'imgsth': {
                    'name': 'SAS_IMGS_THUMB_NEW',
                    'disable_fallback': False,
                    'stats_attr': self.BACKEND_STATS_ATTR + '_imgsth_sas',
                    'enable': True,
                    'is_shift_ports': False,
                },
                'imglth': {
                    'name': 'SAS_IMGS_LARGE_THUMB',
                    'disable_fallback': False,
                    'stats_attr': self.BACKEND_STATS_ATTR + '_imglth_sas',
                    'enable': True,
                    'is_shift_ports': False,
                },
                'imgsrlth': {
                    'name': 'SAS_IMGS_LARGE_THUMB',
                    'disable_fallback': False,
                    'stats_attr': self.BACKEND_STATS_ATTR + '_imgsrlth_sas',
                    'enable': True,
                    'is_shift_ports': False,
                },
            }),
            (Location.MAN, {
                'imgsth': {
                    'name': 'MAN_IMGS_THUMB_NEW',
                    'disable_fallback': False,
                    'stats_attr': self.BACKEND_STATS_ATTR + '_imgsth_man',
                    'enable': True,
                    'is_shift_ports': False,
                },
                'imglth': {
                    'name': 'MAN_IMGS_LARGE_THUMB',
                    'disable_fallback': False,
                    'stats_attr': self.BACKEND_STATS_ATTR + '_imglth_man',
                    'enable': True,
                    'is_shift_ports': False,
                },
                'imgsrlth': {
                    'name': 'MAN_IMGS_LARGE_THUMB',
                    'disable_fallback': False,
                    'stats_attr': self.BACKEND_STATS_ATTR + '_imgsrlth_sas',
                    'enable': True,
                    'is_shift_ports': False,
                },
            }),
            (Location.VLA, {
                'imgsth': {
                    'name': 'VLA_IMGS_THUMB_NEW',
                    'disable_fallback': False,
                    'stats_attr': self.BACKEND_STATS_ATTR + '_imgsth_vla',
                    'enable': True,
                    'is_shift_ports': False,
                },
                'imglth': {
                    'name': 'VLA_IMGS_LARGE_THUMB',
                    'disable_fallback': False,
                    'stats_attr': self.BACKEND_STATS_ATTR + '_imglth_vla',
                    'enable': True,
                    'is_shift_ports': False,
                },
                'imgsrlth': {
                    'name': 'VLA_IMGS_LARGE_THUMB',
                    'disable_fallback': False,
                    'stats_attr': self.BACKEND_STATS_ATTR + '_imgsrlth_sas',
                    'enable': True,
                    'is_shift_ports': False,
                },
            }),
        ])

    def _set_improxy_backends(self):
        return OrderedDict([
            (Location.SAS, {
                'improxy': {
                    'name': 'SAS_IMGS_IMPROXY',
                    'disable_fallback': False,
                    'stats_attr': self.BACKEND_STATS_ATTR + '_sas',
                    'enable': True,
                    'is_shift_ports': False,
                },
            }),
            (Location.MAN, {
                'improxy': {
                    'name': 'MAN_IMGS_IMPROXY',
                    'disable_fallback': False,
                    'stats_attr': self.BACKEND_STATS_ATTR + '_man',
                    'enable': True,
                    'is_shift_ports': False,
                },
            }),
            (Location.VLA, {
                'improxy': {
                    'name': 'VLA_IMGS_IMPROXY',
                    'disable_fallback': False,
                    'stats_attr': self.BACKEND_STATS_ATTR + '_vla',
                    'enable': True,
                    'is_shift_ports': False,
                },
            }),
        ])

    def _set_cacher_backends(self):
        return OrderedDict([
            (Location.SAS, {
                'stats_attr': self.CACHER_STATS_ATTR,
                'id_regexp': r'(/i\\?(.*&)?id=[-0-9a-z]+)' + CGI_PARAMETERS_RE_STR,
                'enable': True,
                'collections': {
                    'imgsth': {
                        'backends': ['SAS_IMGS_THUMB_CACHER_NEW'],
                    },
                    'imglth': {
                        'backends': ['SAS_IMGS_LARGE_THUMB_CACHER_NEW'],
                    },
                    'imgsrlth': {
                        'backends': ['SAS_IMGS_LARGE_THUMB_CACHER_NEW'],
                    }
                },
            }),
            (Location.MAN, {
                'stats_attr': self.CACHER_STATS_ATTR,
                'id_regexp': r'(/i\\?(.*&)?id=[-0-9a-z]+)' + CGI_PARAMETERS_RE_STR,
                'enable': True,
                'collections': {
                    'imgsth': {
                        'backends': ['MAN_IMGS_THUMB_CACHER'],
                    },
                    'imglth': {
                        'backends': ['MAN_IMGS_LARGE_THUMB_CACHER'],
                    },
                    'imgsrlth': {
                        'backends': ['MAN_IMGS_LARGE_THUMB_CACHER'],
                    },
                },
            }),
            (Location.VLA, {
                'stats_attr': self.CACHER_STATS_ATTR,
                'id_regexp': r'(/i\\?(.*&)?id=[-0-9a-z]+)' + CGI_PARAMETERS_RE_STR,
                'enable': True,
                'collections': {
                    'imgsth': {
                        'backends': ['VLA_IMGS_THUMB_CACHER', 'VLA_IMGS_THUMB_CACHER_NEW'],
                    },
                    'imglth': {
                        'backends': ['VLA_IMGS_LARGE_THUMB_CACHER', 'VLA_IMGS_LARGE_THUMB_CACHER_NEW'],
                    },
                    'imgsrlth': {
                        'backends': ['VLA_IMGS_LARGE_THUMB_CACHER', 'VLA_IMGS_LARGE_THUMB_CACHER_NEW'],
                    },
                },
            }),
        ])

    def __init__(self, location, transport, topology='trunk'):
        super(Images, self).__init__(transport, topology)
        self.name = 'imgs'
        self.WORKERS = LuaGlobal('WorkersAmmount', 16)
        self.location = location
        self.RAW_PORT = self._getBalancerPort(self.location)
        self.INTERNAL_PORT = LuaGlobal(PortNames.IMPROXY, self.RAW_PORT)
        self.SSL_PORT = 10443
        self.MASTER_LOG = self.LOG_DIR + 'current-master_log-balancer-' + lua_global_to_string(self.INTERNAL_PORT)
        self.ACCESS_LOG = self.LOG_DIR + 'current-access_log-balancer-' + lua_global_to_string(self.INTERNAL_PORT)
        self.ERROR_LOG = self.LOG_DIR + 'current-error_log-balancer-' + lua_global_to_string(self.INTERNAL_PORT)
        self.DEBUG_LOG = self.LOG_DIR + 'current-debug_log-balancer-' + lua_global_to_string(self.INTERNAL_PORT)
        # TUN SLB scheme
        self.DOMAINS = ['im-tub.yandex.ru', 'im-tub.yandex.com.tr']
        self.IGNORE_ADDRS = {
            Location.MAN: ['87.250.250.60', '77.88.21.61', '2a02:6b8::3:61'],
            Location.SAS: ['172.22.0.74', '172.22.0.75', '87.250.250.60', '77.88.21.61', '2a02:6b8::3:61'],
            Location.VLA: ['172.22.0.74', '172.22.0.75', '87.250.250.60', '77.88.21.61', '2a02:6b8::3:61'],
        }
        self.THUMBS_BAN = True
        self.REUSE_PORT = False
        self.WEBP = True
        self.PRJ_REFERENCES = OrderedDict([
            ('serp', {'match_fsm': OrderedDict([('cgi', '.*ref=oo_serp.*')])}),
            ('imgsnip', {'match_fsm': OrderedDict([('cgi', '.*ref=imgsnip.*')])}),
            ('shiny_discovery', {'match_fsm': OrderedDict([('cgi', '.*ref=shiny_discovery.*')])}),
            ('rq', {'match_fsm': OrderedDict([('cgi', '.*ref=rq.*')])}),
            ('adult_bno', {'match_fsm': OrderedDict([('cgi', '.*ref=adult_bno.*')])}),
            ('itditp', {'match_fsm': OrderedDict([('cgi', '.*ref=itditp.*')])}),
            ('rim', {'match_fsm': OrderedDict([('cgi', '.*ref=rim.*')])}),
        ])

        self.LOCATION_NAME = 'thumb'
        self.LOCATION_REGEXP = OrderedDict([('URI', '/i')])
        self.regexpCommonLocations.append(
            ('gif-check', {'match_fsm': OrderedDict([('URI', '/check.gif')])}, [
                (Modules.Balancer, {
                    'balancer_type': 'rr',
                    'attempts': 1,
                    'balancer_options': OrderedDict([('weights_file', LuaGlobal('slbWeightFile', 'controls/slb-ng.weight'))]),
                    'custom_backends': [
                        (1, 'online_{}'.format(self.location), [
                            (Modules.ErrorDocument,
                             {'status': 200, 'base64': 'R0lGODlhAQABAIABAAAAAP///yH5BAEAAAEALAAAAAABAAEAAAICTAEAOw=='}),
                        ]),
                    ],
                })
            ])
        )
        self.regexpCommonLocations.append(
            ('active-check', {'match_fsm': OrderedDict([('URI', '/active_check')])}, [
                (Modules.Balancer, {
                    'balancer_type': 'rr',
                    'attempts': 1,
                    'balancer_options': OrderedDict([('weights_file', LuaGlobal('slbWeightFile', 'controls/slb-ng.weight'))]),
                    'custom_backends': [
                        (1, 'online_{}'.format(self.location), [
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
        )
        self._sslVars(self.SSL_PORT)


class Video(Project):
    PRODUCTION_LOCATIONS = [Location.SAS, Location.MAN, Location.VLA]

    def _sslVars(self, ssl_port):
        self.ssl = {
            'certKey': 'secrets/key',
            'cert': 'secrets/cert',
            'tls_tickets': {
                'ticket1stKey': 'secrets/1st_video.sha256.tls.key',
                'ticket2stKey': 'secrets/2st_video.sha256.tls.key',
            },
            'ocsp': 'ocsp/video.sha256.der',
            'certKey_sha1': 'secrets/rsa_key',
            'cert_sha1': 'secrets/rsa_cert',
        }
        # FIXME: ssl port is hardcored
        self.INTERNAL_PORT_SSL = LuaGlobal(PortNames.IMPROXY_SSL, ssl_port)

    def _set_index_types(self):
        return OrderedDict([
            ('vidsth', {
                'name': 'main',
                'regexp': OrderedDict([('url', r'/i\\?id=([a-fA-F0-9]{32})(-\\d+-\\d+)?()?' + CGI_PARAMETERS_RE_STR)]),
                'stats_attr': self.BALANCER_STATS_ATTR + '_video_main',
                'weight_file': self.ITS_PATH + 'heavy.weight',
                'fallback_weight_file': self.ITS_PATH + 'vidsth_fallback.weight',
                'thdb_file': self.ITS_PATH + 'vidsth.version',
                'shardFunc': self._generateConsistentHashingLocations,
                'sharded': True,
                'rewrite': '/i?id=%1%4'
            }),
            ('viduth', {
                'name': 'ultra',
                'regexp': OrderedDict([('url', r'/i\\?id=([a-fA-F0-9]{32})-(\\d+-\\d+)?(u)' + CGI_PARAMETERS_RE_STR)]),
                'stats_attr': self.BALANCER_STATS_ATTR + '_video_ultra',
                'weight_file': self.ITS_PATH + 'heavy.weight',
                'fallback_weight_file': self.ITS_PATH + 'viduth_fallback.weight',
                'thdb_file': self.ITS_PATH + 'viduth.version',
                'shardFunc': self._generateConsistentHashingLocations,
                'sharded': True,
                'rewrite': '/i?id=%1%4'
            }),
            ('improxy', {
                'name': 'thumb',
                'regexp': OrderedDict([('URI', r'/(i|pinger)')]),
                'stats_attr': self.BACKEND_STATS_ATTR,
                'weight_file': self.ITS_PATH + 'heavy.weight',
                'fallback_weight_file': self.ITS_PATH + 'improxy_fallback.weight',
                'shardFunc': self._generatePlainLocation,
                'sharded': False,
            }),
        ])

    def _set_backends(self):
        return OrderedDict([
            (Location.SAS, {
                'vidsth': {
                    'name': 'SAS_VIDEO_THUMB',
                    'disable_fallback': False,
                    'stats_attr': self.BACKEND_STATS_ATTR + '_vidsth_sas',
                    'enable': True,
                    'is_shift_ports': False,
                },
                'viduth': {
                    'name': 'SAS_VIDEO_ULTRA_THUMB',
                    'disable_fallback': False,
                    'stats_attr': self.BACKEND_STATS_ATTR + '_viduth_sas',
                    'enable': True,
                    'is_shift_ports': False,
                },
            }),
            (Location.VLA, {
                'vidsth': {
                    'name': 'VLA_VIDEO_THUMB',
                    'disable_fallback': False,
                    'stats_attr': self.BACKEND_STATS_ATTR + '_vidsth_vla',
                    'enable': True,
                    'is_shift_ports': False,
                },
                'viduth': {
                    'name': 'VLA_VIDEO_ULTRA_THUMB',
                    'disable_fallback': False,
                    'stats_attr': self.BACKEND_STATS_ATTR + '_viduth_vla',
                    'enable': True,
                    'is_shift_ports': False,
                },
            }),
            (Location.MAN, {
                'vidsth': {
                    'name': 'MAN_VIDEO_THUMB',
                    'disable_fallback': False,
                    'stats_attr': self.BACKEND_STATS_ATTR + '_vidsth_man',
                    'enable': True,
                    'is_shift_ports': False,
                },
                'viduth': {
                    'name': 'MAN_VIDEO_ULTRA_THUMB',
                    'disable_fallback': False,
                    'stats_attr': self.BACKEND_STATS_ATTR + '_viduth_man',
                    'enable': True,
                    'is_shift_ports': False,
                },
            }),
        ])

    def _set_improxy_backends(self):
        return OrderedDict([
            (Location.SAS, {
                'improxy': {
                    'name': 'SAS_VIDEO_IMPROXY',
                    'disable_fallback': False,
                    'stats_attr': self.BACKEND_STATS_ATTR + '_sas',
                    'enable': True,
                    'is_shift_ports': False,
                },
            }),
            (Location.MAN, {
                'improxy': {
                    'name': 'MAN_VIDEO_IMPROXY',
                    'disable_fallback': False,
                    'stats_attr': self.BACKEND_STATS_ATTR + '_man',
                    'enable': True,
                    'is_shift_ports': False,
                },
            }),
            (Location.VLA, {
                'improxy': {
                    'name': 'VLA_VIDEO_IMPROXY',
                    'disable_fallback': False,
                    'stats_attr': self.BACKEND_STATS_ATTR + '_vla',
                    'enable': True,
                    'is_shift_ports': False,
                },
            }),
        ])

    def _set_cacher_backends(self):
        return OrderedDict([
            (Location.SAS, {
                'stats_attr': self.BACKEND_STATS_ATTR + '_cacher',
                'id_regexp': r'(/i\\?id=[-0-9a-z]+)' + CGI_PARAMETERS_RE_STR,
                'enable': True,
                'collections': {
                    'vidsth': {
                        'backends': ['SAS_VIDEO_THUMB_CACHER'],
                    },
                    'viduth': {
                        'backends': ['SAS_VIDEO_THUMB_CACHER'],
                    },
                },
            }),
            (Location.MAN, {
                'stats_attr': self.BACKEND_STATS_ATTR + '_cacher',
                'id_regexp': r'(/i\\?id=[-0-9a-z]+)' + CGI_PARAMETERS_RE_STR,
                'enable': True,
                'collections': {
                    'vidsth': {
                        'backends': ['MAN_VIDEO_THUMB_CACHER'],
                    },
                    'viduth': {
                        'backends': ['MAN_VIDEO_THUMB_CACHER'],
                    },
                },
            }),
            (Location.VLA, {
                'stats_attr': self.BACKEND_STATS_ATTR + '_cacher',
                'id_regexp': r'(/i\\?id=[-0-9a-z]+)' + CGI_PARAMETERS_RE_STR,
                'enable': True,
                'collections': {
                    'vidsth': {
                        'backends': ['VLA_VIDEO_THUMB_CACHER'],
                    },
                    'viduth': {
                        'backends': ['VLA_VIDEO_THUMB_CACHER'],
                    },
                },
            }),
        ])

    def __init__(self, location, transport, topology='trunk'):
        super(Video, self).__init__(transport, topology)
        self.name = 'video'
        self.location = location
        self.RS = {
            Location.SAS: 'sas2-0088.search.yandex.net',
            Location.MAN: 'man1-4303.search.yandex.net',
            Location.VLA: 'vla1-1181.search.yandex.net'
        }
        # ports
        self.RAW_PORT = self._getBalancerPort(self.location)
        self.INTERNAL_PORT = LuaGlobal(PortNames.IMPROXY, str(self.RAW_PORT))
        self.SSL_PORT = 1081
        # logs
        self.MASTER_LOG = self.LOG_DIR + 'current-master_log-balancer-' + self.INTERNAL_PORT
        self.ACCESS_LOG = self.LOG_DIR + 'current-access_log-balancer-' + self.INTERNAL_PORT
        self.ERROR_LOG = self.LOG_DIR + 'current-error_log-balancer-' + self.INTERNAL_PORT
        self.DEBUG_LOG = self.LOG_DIR + 'current-debug_log-balancer-' + self.INTERNAL_PORT
        self.LOCATION_NAME = 'thumb'
        self.LOCATION_REGEXP = OrderedDict([('URI', '/i')])
        self.regexpCommonLocations.append(
            ('gif-check', {'match_fsm': OrderedDict([('URI', '/check.gif')])}, [
                (Modules.Balancer, {
                    'balancer_type': 'rr',
                    'attempts': 1,
                    'balancer_options': OrderedDict([('weights_file', LuaGlobal('slbWeightFile', 'controls/slb-ng.weight'))]),
                    'custom_backends': [
                        (1, 'online_{}'.format(self.location), [
                            (Modules.ErrorDocument,
                             {'status': 200, 'base64': 'R0lGODlhAQABAIABAAAAAP///yH5BAEAAAEALAAAAAABAAEAAAICTAEAOw=='}),
                        ]),
                    ],
                })
            ])
        )
        # TUN SLB scheme
        if self.location in [Location.MAN, Location.SAS, Location.VLA]:
            self.DOMAINS = ['video-tub.yandex.ru']
        self._sslVars(self.SSL_PORT)
        if self.location in self.CDN_LOCATIONS:
            self.WORKERS = 8
            self._regproxy()


if __name__ == '__main__':
    images_subprojects = [
        'images_%s' %
        x for x in Images.PRODUCTION_LOCATIONS]
    video_subprojects = [
        'video_%s' %
        x for x in Video.PRODUCTION_LOCATIONS]
    all_subprojects = images_subprojects + video_subprojects

    parser = argparse.ArgumentParser(description='Generate improxy config')
    parser.add_argument(
        '-r',
        '--raw',
        action='store_true',
        default=False,
        help='Generate intermediate python representation, useful for reclustering')
    parser.add_argument('-t', '--transport', type=str, required=True,
                        choices=TRANSPORTS,
                        help="Obligatory. Transport for generation")
    parser.add_argument('--topology', type=str, required=False,
                        default='trunk',
                        help="Topology version")
    parser.add_argument('subproject', choices=all_subprojects)
    parser.add_argument('output', metavar='<output file>')

    options = parser.parse_args()

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

        transport = CurdbTransport()
    elif options.transport == "api":
        from src.transports.gencfg_api_transport import GencfgApiTransport

        transport = GencfgApiTransport(req_timeout=50, attempts=3)

    cfg = None
    project, location = options.subproject.split('_')
    if project == 'images':
        project = Images(location, transport, options.topology)
    elif project == 'video':
        project = Video(location, transport, options.topology)
    else:
        raise Exception(
            'Unknown improxy project %s, location %s' %
            (project, location))

    cfg = project._balancer()
    if cfg is None:
        raise Exception(
            'Generator return empty config for %s, location %s' %
            (project, location))

    generate(
        cfg,
        options.output,
        options.raw,
        instance_db_transport=project.transport,
        user_config_prefix=project.PREFIX)
