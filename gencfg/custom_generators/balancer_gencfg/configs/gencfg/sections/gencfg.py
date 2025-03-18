from collections import OrderedDict

import src.modules as Modules


HOSTINFO_BACKENDS = [
    'MAN_CLUSTERSTATE_HOSTINFO',
    'SAS_CLUSTERSTATE_HOSTINFO',
    'VLA_CLUSTERSTATE_HOSTINFO',
]

HOSTINFO_TAGS_BACKENDS = [
    'SAS_GENCFG_TAGS',
    'VLA_GENCFG_TAGS',
    'MAN_GENCFG_TAGS',
]

HOSTINFO_INSTANCES_TAGS = [
    'MAN_CLUSTERSTATE_INSTANCES_TAGS',
    'SAS_CLUSTERSTATE_INSTANCES_TAGS',
    'VLA_CLUSTERSTATE_INSTANCES_TAGS',
]

GENCFG_API_NG_BACKENDS = [
    'ALL_GENCFG_API_NG',
]

GENCFG_GUI_BACKENDS = [
    'MAN_ALL_GENCFG_GUI',
    'SAS_ALL_GENCFG_GUI',
    'VLA_ALL_GENCFG_GUI',
]

GENCFG_HOST = '(gencfg.yandex-team.ru|gencfg)(:\\\\d+)?'
GENCFG_API_HOST = '.*(api.gencfg.yandex-team.ru|api.gencfg)(:\\\\d+)?'
GENCFG_API_NG_HOST = '.*(aping.gencfg.yandex-team.ru|api.gencfg)(:\\\\d+)?'
GENCFG_WBE_HOST = '.*(wbe.gencfg.yandex-team.ru|wbe.gencfg)(:\\\\d+)?'
OSOL_PLAYGROUND = 'gg.clusterstate.yandex-team.ru(:\\\\d+)?'


def get_modules(https_postfix):
    return [
        ('gencfg-api', {'match_fsm': OrderedDict([('host', GENCFG_API_HOST)])}, [
            (Modules.Regexp, [
                ('trunk_description', {'match_fsm': OrderedDict([('URI', '/trunk/description')])}, [
                    (Modules.Report, {'uuid': 'gencfg-hostinfo'}),
                    (Modules.ResponseHeaders, {'create': OrderedDict([('Access-Control-Allow-Origin', '*')])}),
                    (Modules.Balancer, {
                        'balancer_type': 'active',
                        'balancer_options': OrderedDict({
                            'request': 'HEAD /trunk/tags HTTP/1.1\\n\\n',
                            'delay': '20s'
                        }),
                        'backends': HOSTINFO_BACKENDS,
                        'proxy_options': OrderedDict([
                            ('fail_on_5xx', True),
                            ('backend_timeout', '320s'),
                        ]),
                        'attempts': 3,
                    }),
                ]),
                ('trunk_slbs', {'match_fsm': OrderedDict([('URI', '/trunk/slbs')])}, [
                    (Modules.Report, {'uuid': 'gencfg-hostinfo'}),
                    (Modules.ResponseHeaders, {'create': OrderedDict([('Access-Control-Allow-Origin', '*')])}),
                    (Modules.Balancer, {
                        'balancer_type': 'active',
                        'balancer_options': OrderedDict({
                            'request': 'HEAD /trunk/tags HTTP/1.1\\n\\n',
                            'delay': '20s'
                        }),
                        'backends': HOSTINFO_BACKENDS,
                        'proxy_options': OrderedDict([
                            ('fail_on_5xx', True),
                            ('backend_timeout', '320s'),
                        ]),
                        'attempts': 3,
                    }),
                ]),
                ('tags_tag_by_commit', {'match_fsm': OrderedDict([('URI', '/tags/tag_by_commit/[0-9]+')])}, [
                    (Modules.Report, {'uuid': 'gencfg-hostinfo'}),
                    (Modules.ResponseHeaders, {'create': OrderedDict([('Access-Control-Allow-Origin', '*')])}),
                    (Modules.Balancer, {
                        'balancer_type': 'active',
                        'balancer_options': OrderedDict({
                            'request': 'HEAD /trunk/tags HTTP/1.1\\n\\n',
                            'delay': '20s'
                        }),
                        'backends': HOSTINFO_BACKENDS,
                        'proxy_options': OrderedDict([
                            ('fail_on_5xx', True),
                            ('backend_timeout', '320s'),
                        ]),
                        'attempts': 3,
                    }),
                ]),
                # GENCFG-2151
                ('trunk_hbf_macroses', {'match_fsm': OrderedDict([('URI', '/trunk/hbf_macroses')])}, [
                    (Modules.Report, {'uuid': 'gencfg-hostinfo-hbf-macroses'}),
                    (Modules.ResponseHeaders, {'create': OrderedDict([('Access-Control-Allow-Origin', '*')])}),
                    (Modules.Balancer, {
                        'balancer_type': 'active',
                        'balancer_options': OrderedDict({
                            'request': 'HEAD /trunk/tags HTTP/1.1\\n\\n',
                            'delay': '20s'
                        }),
                        'backends': HOSTINFO_BACKENDS,
                        'proxy_options': OrderedDict([
                            ('fail_on_5xx', True),
                            ('backend_timeout', '3600s'),
                        ]),
                        'attempts': 3,
                    }),
                ]),
                ('tags_xxx_groups', {'match_fsm': OrderedDict([('URI', '/tags/stable-[0-9]+-r[0-9]+/groups.*')])}, [
                    (Modules.Report, {'uuid': 'gencfg-hostinfo'}),
                    (Modules.ResponseHeaders, {'create': OrderedDict([('Access-Control-Allow-Origin', '*')])}),
                    (Modules.Balancer, {
                        'balancer_type': 'active',
                        'balancer_options': OrderedDict({
                            'request': 'HEAD /trunk/tags HTTP/1.1\\n\\n',
                            'delay': '120s'
                        }),
                        'backends': HOSTINFO_TAGS_BACKENDS,
                        'proxy_options': OrderedDict([
                            ('fail_on_5xx', True),
                            ('backend_timeout', '3600s'),
                        ]),
                        'attempts': 3,
                    }),
                ]),
                ('trunk_dns', {'match_fsm': OrderedDict([('URI', '/trunk/dns/.+')])}, [
                    (Modules.Report, {'uuid': 'gencfg-hostinfo-dns'}),
                    (Modules.ResponseHeaders, {'create': OrderedDict([('Access-Control-Allow-Origin', '*')])}),
                    (Modules.Balancer, {
                        'balancer_type': 'active',
                        'balancer_options': OrderedDict({
                            'request': 'HEAD /trunk/tags HTTP/1.1\\n\\n',
                            'delay': '20s'
                        }),
                        'backends': HOSTINFO_BACKENDS,
                        'proxy_options': OrderedDict([
                            ('fail_on_5xx', True),
                            ('backend_timeout', '3600s'),
                        ]),
                        'attempts': 3,
                    }),
                ]),
                ('trunk_tags', {'match_fsm': OrderedDict([('URI', '/trunk/tags')])}, [
                    (Modules.Report, {'uuid': 'gencfg-hostinfo'}),
                    (Modules.ResponseHeaders, {'create': OrderedDict([('Access-Control-Allow-Origin', '*')])}),
                    (Modules.Balancer, {
                        'balancer_type': 'active',
                        'balancer_options': OrderedDict({
                            'request': 'HEAD /trunk/tags HTTP/1.1\\n\\n',
                            'delay': '20s'
                        }),
                        'backends': HOSTINFO_BACKENDS,
                        'proxy_options': OrderedDict([
                            ('fail_on_5xx', True),
                            ('backend_timeout', '320s'),
                        ]),
                        'attempts': 3,
                    }),
                ]),
                ('trunk_hosts_data', {'match_fsm': OrderedDict([('URI', '/trunk/hosts_data')])}, [
                    (Modules.Report, {'uuid': 'gencfg-hostinfo'}),
                    (Modules.ResponseHeaders, {'create': OrderedDict([('Access-Control-Allow-Origin', '*')])}),
                    (Modules.Balancer, {
                        'balancer_type': 'active',
                        'balancer_options': OrderedDict({
                            'request': 'HEAD /trunk/tags HTTP/1.1\\n\\n',
                            'delay': '20s'
                        }),
                        'backends': HOSTINFO_BACKENDS,
                        'proxy_options': OrderedDict([
                            ('fail_on_5xx', True),
                            ('backend_timeout', '320s'),
                        ]),
                        'attempts': 3,
                    }),
                ]),
                ('tags_xxx_hosts_xxx_instances_tags',
                 {'match_fsm': OrderedDict([('URI', '/tags/stable-(9[0-9]-r[0-9]+|[1-9][0-9][0-9]+-r[0-9]+)/hosts/.+/instances_tags')])}, [
                     (Modules.Report, {'uuid': 'gencfg-hostinfo-instances-tags'}),
                     (Modules.ResponseHeaders, {'create': OrderedDict([('Access-Control-Allow-Origin', '*')])}),
                     (Modules.Balancer, {
                         'balancer_type': 'rr',
                         'backends': HOSTINFO_INSTANCES_TAGS,
                         'proxy_options': OrderedDict([
                             ('fail_on_5xx', True),
                             ('backend_timeout', '10s'),
                         ]),
                         'attempts': 3,
                     }),
                 ]),
                ('trunk_hosts_to_groups', {'match_fsm': OrderedDict([('URI', '/trunk/hosts/hosts_to_groups.*')])}, [
                    (Modules.Report, {'uuid': 'gencfg-hostinfo'}),
                    (Modules.ResponseHeaders, {'create': OrderedDict([('Access-Control-Allow-Origin', '*')])}),
                    (Modules.Balancer, {
                        'balancer_type': 'active',
                        'balancer_options': OrderedDict({
                            'request': 'HEAD /trunk/tags HTTP/1.1\\n\\n',
                            'delay': '20s'
                        }),
                        'backends': HOSTINFO_BACKENDS,
                        'proxy_options': OrderedDict([
                            ('fail_on_5xx', True),
                            ('backend_timeout', '320s'),
                        ]),
                        'attempts': 3,
                    }),
                ]),
                ('trunk_hosts_to_hardware', {'match_fsm': OrderedDict([('URI', '/trunk/hosts/hosts_to_hardware.*')])}, [
                    (Modules.Report, {'uuid': 'gencfg-hostinfo'}),
                    (Modules.ResponseHeaders, {'create': OrderedDict([('Access-Control-Allow-Origin', '*')])}),
                    (Modules.Balancer, {
                        'balancer_type': 'active',
                        'balancer_options': OrderedDict({
                            'request': 'HEAD /trunk/tags HTTP/1.1\\n\\n',
                            'delay': '20s'
                        }),
                        'backends': HOSTINFO_BACKENDS,
                        'proxy_options': OrderedDict([
                            ('fail_on_5xx', True),
                            ('backend_timeout', '320s'),
                        ]),
                        'attempts': 3,
                    }),
                ]),
                ('unstable_tags', {'match_fsm': OrderedDict([('URI', '/unstable/tags')])}, [
                    (Modules.Report, {'uuid': 'gencfg-hostinfo'}),
                    (Modules.ResponseHeaders, {'create': OrderedDict([('Access-Control-Allow-Origin', '*')])}),
                    (Modules.Balancer, {
                        'balancer_type': 'active',
                        'balancer_options': OrderedDict({
                            'request': 'HEAD /trunk/tags HTTP/1.1\\n\\n',
                            'delay': '20s'
                        }),
                        'backends': HOSTINFO_BACKENDS,
                        'proxy_options': OrderedDict([
                            ('fail_on_5xx', True),
                            ('backend_timeout', '320s'),
                        ]),
                        'attempts': 3,
                    }),
                ]),
                ('online_groups_xxx_instances',
                 {'match_fsm': OrderedDict([('URI', '/online/(searcherlookup/)?groups/[A-Z0-9_]+/instances')])}, [
                     (Modules.Report, {'uuid': 'gencfg-hostinfo'}),
                     (Modules.ResponseHeaders, {'create': OrderedDict([('Access-Control-Allow-Origin', '*')])}),
                     (Modules.Balancer, {
                         'balancer_type': 'active',
                         'balancer_options': OrderedDict({
                             'request': 'HEAD /trunk/tags HTTP/1.1\\n\\n',
                             'delay': '20s'
                         }),
                         'backends': HOSTINFO_BACKENDS,
                         'proxy_options': OrderedDict([
                             ('fail_on_5xx', True),
                             ('backend_timeout', '120s'),
                         ]),
                         'attempts': 3,
                     }),
                 ]),
                ('online_groups_xxx_card',
                 {'match_fsm': OrderedDict([('URI', '/online/(searcherlookup/)?groups/[A-Z0-9_]+/card')])}, [
                     (Modules.Report, {'uuid': 'gencfg-hostinfo'}),
                     (Modules.ResponseHeaders, {'create': OrderedDict([('Access-Control-Allow-Origin', '*')])}),
                     (Modules.Balancer, {
                         'balancer_type': 'active',
                         'balancer_options': OrderedDict({
                             'request': 'HEAD /trunk/tags HTTP/1.1\\n\\n',
                             'delay': '20s'
                         }),
                         'backends': HOSTINFO_BACKENDS,
                         'proxy_options': OrderedDict([
                             ('fail_on_5xx', True),
                             ('backend_timeout', '120s'),
                         ]),
                         'attempts': 3,
                     }),
                 ]),
                ('tags_XXX_search_map_XXX_up_down',
                 {'match_fsm': OrderedDict([('URI', '/tags/stable-[0-9]+-r[0-9]+/search_map/[A-Z0-9_]+/(up|down)')])}, [
                     (Modules.Report, {'uuid': 'gencfg-hostinfo'}),
                     (Modules.ResponseHeaders, {'create': OrderedDict([('Access-Control-Allow-Origin', '*')])}),
                     (Modules.Balancer, {
                         'balancer_type': 'active',
                         'balancer_options': OrderedDict({
                             'request': 'HEAD /trunk/tags HTTP/1.1\\n\\n',
                             'delay': '20s'
                         }),
                         'backends': HOSTINFO_BACKENDS,
                         'proxy_options': OrderedDict([
                             ('fail_on_5xx', True),
                             ('backend_timeout', '320s'),
                         ]),
                         'attempts': 3,
                     }),
                 ]),
                ('trunk_groups_owners', {'match_fsm': OrderedDict([('URI', '/trunk/groups_owners')])}, [
                    (Modules.Report, {'uuid': 'gencfg-hostinfo'}),
                    (Modules.ResponseHeaders, {'create': OrderedDict([('Access-Control-Allow-Origin', '*')])}),
                    (Modules.Balancer, {
                        'balancer_type': 'active',
                        'balancer_options': OrderedDict({
                            'request': 'HEAD /trunk/tags HTTP/1.1\\n\\n',
                            'delay': '20s'
                        }),
                        'backends': HOSTINFO_BACKENDS,
                        'proxy_options': OrderedDict([
                            ('fail_on_5xx', True),
                            ('backend_timeout', '320s'),
                        ]),
                        'attempts': 3,
                    }),
                ]),
                ('trunk_groups', {'match_fsm': OrderedDict([('URI', '/trunk/groups')])}, [
                    (Modules.Report, {'uuid': 'gencfg-hostinfo'}),
                    (Modules.ResponseHeaders, {'create': OrderedDict([('Access-Control-Allow-Origin', '*')])}),
                    (Modules.Balancer, {
                        'balancer_type': 'active',
                        'balancer_options': OrderedDict({
                            'request': 'HEAD /trunk/tags HTTP/1.1\\n\\n',
                            'delay': '20s'
                        }),
                        'backends': HOSTINFO_BACKENDS,
                        'proxy_options': OrderedDict([
                            ('fail_on_5xx', True),
                            ('backend_timeout', '320s'),
                        ]),
                        'attempts': 3,
                    }),
                ]),
                (
                    'trunk_xxx_searcherlookup_groups_xxx_instances',
                    {
                        'match_fsm': OrderedDict([(
                            'URI',
                            '/tags/stable-(9[0-9]-r[0-9]+|[1-9][0-9][0-9]+-r[0-9]+)/searcherlookup/groups/[A-Z0-9_]+/instances'
                        )])
                    },
                    [
                        (Modules.Report, {'uuid': 'gencfg-hostinfo'}),
                        (Modules.ResponseHeaders, {'create': OrderedDict([('Access-Control-Allow-Origin', '*')])}),
                        (Modules.Balancer, {
                            'balancer_type': 'active',
                            'balancer_options': OrderedDict({
                                'request': 'HEAD /trunk/tags HTTP/1.1\\n\\n',
                                'delay': '120s'
                            }),
                            'backends': HOSTINFO_TAGS_BACKENDS,
                            'proxy_options': OrderedDict([
                                ('fail_on_5xx', True),
                                ('backend_timeout', '320s'),
                            ]),
                            'attempts': 3,
                        }),
                    ]
                ),
                ('trunk_commits', {'match_fsm': OrderedDict([('URI', '/trunk/commits')])}, [
                    (Modules.Report, {'uuid': 'gencfg-hostinfo'}),
                    (Modules.ResponseHeaders, {'create': OrderedDict([('Access-Control-Allow-Origin', '*')])}),
                    (Modules.Balancer, {
                        'balancer_type': 'active',
                        'balancer_options': OrderedDict({
                            'request': 'HEAD /trunk/tags HTTP/1.1\\n\\n',
                            'delay': '20s'
                        }),
                        'backends': HOSTINFO_BACKENDS,
                        'proxy_options': OrderedDict([
                            ('fail_on_5xx', True),
                            ('backend_timeout', '320s'),
                        ]),
                        'attempts': 3,
                    }),
                ]),
                ('trunk_xxx_groups_xxx', {'match_fsm': OrderedDict([('URI', '/trunk[/0-9]*/groups/[A-Z0-9_]+')])}, [
                    (Modules.Report, {'uuid': 'gencfg-hostinfo'}),
                    (Modules.ResponseHeaders, {'create': OrderedDict([('Access-Control-Allow-Origin', '*')])}),
                    (Modules.Balancer, {
                        'balancer_type': 'active',
                        'balancer_options': OrderedDict({
                            'request': 'HEAD /trunk/tags HTTP/1.1\\n\\n',
                            'delay': '20s'
                        }),
                        'backends': HOSTINFO_BACKENDS,
                        'proxy_options': OrderedDict([
                            ('fail_on_5xx', True),
                            ('backend_timeout', '320s'),
                        ]),
                        'attempts': 3,
                    }),
                ]),
                ('trunk_xxx_groups_xxx_instances', {'match_fsm': OrderedDict([('URI', '/trunk[/0-9]*/groups/[A-Z0-9_]+/instances')])}, [
                    (Modules.Report, {'uuid': 'gencfg-hostinfo'}),
                    (Modules.ResponseHeaders, {'create': OrderedDict([('Access-Control-Allow-Origin', '*')])}),
                    (Modules.Balancer, {
                        'balancer_type': 'active',
                        'balancer_options': OrderedDict({
                            'request': 'HEAD /trunk/tags HTTP/1.1\\n\\n',
                            'delay': '20s'
                        }),
                        'backends': HOSTINFO_BACKENDS,
                        'proxy_options': OrderedDict([
                            ('fail_on_5xx', True),
                            ('backend_timeout', '320s'),
                        ]),
                        'attempts': 3,
                    }),
                ]),
                ('trunk_xxx_groups_xxx_card', {'match_fsm': OrderedDict([('URI', '/trunk[/0-9]*/groups/[A-Z0-9_]+/card')])}, [
                    (Modules.Report, {'uuid': 'gencfg-hostinfo'}),
                    (Modules.ResponseHeaders, {'create': OrderedDict([('Access-Control-Allow-Origin', '*')])}),
                    (Modules.Balancer, {
                        'balancer_type': 'active',
                        'balancer_options': OrderedDict({
                            'request': 'HEAD /trunk/tags HTTP/1.1\\n\\n',
                            'delay': '20s'
                        }),
                        'backends': HOSTINFO_BACKENDS,
                        'proxy_options': OrderedDict([
                            ('fail_on_5xx', True),
                            ('backend_timeout', '320s'),
                        ]),
                        'attempts': 3,
                    }),
                ]),
                ('trunk_xxx_searcherlookup_groups_xxx_bsconfig',
                 {'match_fsm': OrderedDict([('URI', '/trunk[/0-9]*/searcherlookup/groups/[A-Z0-9_]+/bsconfig')])}, [
                     (Modules.Report, {'uuid': 'gencfg-hostinfo'}),
                     (Modules.ResponseHeaders, {'create': OrderedDict([('Access-Control-Allow-Origin', '*')])}),
                     (Modules.Balancer, {
                         'balancer_type': 'active',
                         'balancer_options': OrderedDict({
                             'request': 'HEAD /trunk/tags HTTP/1.1\\n\\n',
                             'delay': '20s'
                         }),
                         'backends': HOSTINFO_BACKENDS,
                         'proxy_options': OrderedDict([
                             ('fail_on_5xx', True),
                             ('backend_timeout', '320s'),
                         ]),
                         'attempts': 3,
                     }),
                 ]),
                ('tags_xxx_searcherlookup_groups_xxx_bsconfig', {'match_fsm': OrderedDict(
                    [('URI', '/tags/stable-[0-9]+-r[0-9]+/searcherlookup/groups/[A-Z0-9_]+/bsconfig')])}, [
                        (Modules.Report, {'uuid': 'gencfg-hostinfo'}),
                        (Modules.ResponseHeaders, {'create': OrderedDict([('Access-Control-Allow-Origin', '*')])}),
                        (Modules.Balancer, {
                            'balancer_type': 'active',
                            'balancer_options': OrderedDict({
                                'request': 'HEAD /trunk/tags HTTP/1.1\\n\\n',
                                'delay': '120s'
                            }),
                            'backends': HOSTINFO_TAGS_BACKENDS,
                            'proxy_options': OrderedDict([
                                ('fail_on_5xx', True),
                                ('backend_timeout', '320s'),
                            ]),
                            'attempts': 3,
                        }),
                ]),
                ('trunk_xxx_searcherlookupi_groups_xxx_instances',
                 {'match_fsm': OrderedDict([('URI', '/trunk[/0-9]*/searcherlookup/groups/[A-Z0-9_]+/instances')])}, [
                     (Modules.Report, {'uuid': 'gencfg-hostinfo'}),
                     (Modules.ResponseHeaders, {'create': OrderedDict([('Access-Control-Allow-Origin', '*')])}),
                     (Modules.Balancer, {
                         'balancer_type': 'active',
                         'balancer_options': OrderedDict({
                             'request': 'HEAD /trunk/tags HTTP/1.1\\n\\n',
                             'delay': '20s'
                         }),
                         'backends': HOSTINFO_BACKENDS,
                         'proxy_options': OrderedDict([
                             ('fail_on_5xx', True),
                             ('backend_timeout', '320s'),
                         ]),
                         'attempts': 3,
                     }),
                 ]),
                ('cpu_models', {'match_fsm': OrderedDict([('URI', '/cpumodels')])}, [
                    (Modules.Report, {'uuid': 'gencfg-hostinfo'}),
                    (Modules.ResponseHeaders, {'create': OrderedDict([('Access-Control-Allow-Origin', '*')])}),
                    (Modules.Balancer, {
                        'balancer_type': 'active',
                        'balancer_options': OrderedDict({
                            'request': 'HEAD /trunk/tags HTTP/1.1\\n\\n',
                            'delay': '20s'
                        }),
                        'backends': HOSTINFO_BACKENDS,
                        'proxy_options': OrderedDict([
                            ('fail_on_5xx', True),
                            ('backend_timeout', '320s'),
                        ]),
                        'attempts': 3,
                    }),
                ]),
                ('default', {}, [
                    (Modules.Static, {'file' : '404.html'}),
                ]),
            ]),
        ]),
        ('gencfg-backend', {'match_fsm': OrderedDict([('host', GENCFG_WBE_HOST)])}, [
            (Modules.Regexp, [
                ('tags', {'match_fsm': OrderedDict([('URI', '/tags(/.*)?')])}, [
                    (Modules.Balancer, {
                        'backends': ['ALL_GENCFG_WBE_TAGS_NEW'],
                        'proxy_options': OrderedDict([
                            ('fail_on_5xx', True),
                            ('backend_timeout', '320s'),
                        ]),
                        'attempts': 3,
                    }),
                ]),
                ('unstable', {'match_fsm': OrderedDict([('URI', '/unstable(/.*)?')])}, [
                    (Modules.Balancer, {
                        'balancer_type': 'hashing',
                        'backends': ['ALL_GENCFG_WBE_UNSTABLE_NEW'],
                        'proxy_options': OrderedDict([
                            ('fail_on_5xx', True),
                            ('backend_timeout', '320s'),
                        ]),
                        'attempts': 3
                    }),
                ]),
                ('default', {}, [
                    (Modules.Balancer, {
                        'backends': ['ALL_GENCFG_WBE_NEW'],
                        'proxy_options': OrderedDict([
                            ('fail_on_5xx', True),
                            ('backend_timeout', '320s'),
                        ]),
                        'attempts': 3,
                    }),
                ]),
            ]),
        ]),
        ('gencfg', {'match_fsm': OrderedDict([('host', GENCFG_HOST)])}, [
            (Modules.Regexp, [
                ('gencfg_redirect', {'match_fsm': OrderedDict([('host', 'gencfg(:\\\\d+)?')])}, [
                    (Modules.Headers, {
                        'create_func': OrderedDict([('Location', 'url')])
                    }),
                    (Modules.Rewrite, {
                        'actions': [{
                            'header_name': 'Location',
                            'regexp': '(.*)',
                            'rewrite': 'http%s://gencfg.yandex-team.ru%%1' % https_postfix
                        }]
                    }),
                    (Modules.ErrorDocument, {
                        'status': 302,
                        'remain_headers': 'Location'
                    }),
                ]),
                ('api_trunk_tags', {'match_fsm': OrderedDict([('URI', '/api/trunk/tags')])}, [
                    (Modules.Report, {'uuid': 'gencfg-hostinfo'}),
                    (Modules.Rewrite, {'actions': [
                        {'split': 'url', 'regexp': '/api/trunk/tags', 'rewrite': '/trunk/tags'}
                    ]}),
                    (Modules.Balancer, {
                        'balancer_type': 'active',
                        'balancer_options': OrderedDict({
                            'request': 'HEAD /trunk/tags HTTP/1.1\\n\\n',
                            'delay': '120s'
                        }),
                        'backends': HOSTINFO_TAGS_BACKENDS,
                        'proxy_options': OrderedDict([
                            ('fail_on_5xx', True),
                            ('backend_timeout', '320s'),
                        ]),
                        'attempts': 3,
                    }),
                ]),
                ('default', {}, [
                    (Modules.Report, {'uuid': 'gencfg-gui'}),
                    (Modules.Balancer, {
                        'backends': GENCFG_GUI_BACKENDS,
                        'proxy_options': OrderedDict([
                            ('backend_timeout', '60s'),
                            ('fail_on_5xx', False),
                        ]),
                    }),
                ]),
            ]),
        ]),
        ('gg-clusterstate', {'match_fsm': OrderedDict([('host', OSOL_PLAYGROUND)])}, [
            (Modules.Regexp, [
                ('default', {}, [
                    (Modules.Balancer, {
                        'balancer_type': 'hashing',
                        'backends': ['electra.search.yandex.net:12345'],
                        'proxy_options': OrderedDict([
                            ('backend_timeout', '90s'),
                            ('fail_on_5xx', False),
                        ]),
                    }),
                ]),
            ]),
        ]),
        ('gencfg_api_ng', {'match_fsm': OrderedDict([('host', GENCFG_API_NG_HOST)])}, [
            (Modules.Regexp, [
                ('default', {}, [
                    (Modules.Balancer, {
                        'balancer_type': 'rr',
                        'backends': GENCFG_API_NG_BACKENDS,
                        'proxy_options': OrderedDict([
                            ('backend_timeout', '30s'),
                            ('fail_on_5xx', False),
                        ]),
                    }),
                ]),
            ]),
        ]),
    ]
