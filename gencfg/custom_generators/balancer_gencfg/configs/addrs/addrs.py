#!/skynet/python/bin/python

import json
import os
from collections import OrderedDict

import src.constants as Constants
import src.macroses as Macroses
import src.modules as Modules
from src.generator import generate
from src.optionschecker import OptionsChecker


@OptionsChecker([
    ('data', None, dict, True),
    ('data2', None, dict, True),
    ('config_tag', 'trunk', str, True),
])
def templateAddrsTesting(options):

    regexp_modules = []

    for key, value in options['data2'].iteritems():
        for sub_key, sub_value in value.iteritems():
            regexp_modules.append(
                (
                    '{}_{}'.format(key, sub_key),
                    {
                        'match_fsm': OrderedDict([
                            ('url', '/{}(-|/){}((/|\\\\?).*)?'.format(key, sub_key))
                        ])
                    },
                    [
                        (Modules.Rewrite, {
                            'actions': [
                                {
                                    'regexp': '/{}(-|/){}(/)?(.*)?'.format(key, sub_key),
                                    'rewrite': '/%3',
                                    'split': 'url',
                                }
                            ]
                        }),
                        (Modules.Balancer, {
                            'backends': ['{}'.format(group) for group in sub_value],
                            'proxy_options': OrderedDict([
                                ('keepalive_count', 10),
                            ])
                        })
                    ]
                )
            )

    default_backends = [
        '{}'.format(group) for group in options['data']['synonyms']['backend']['default']
    ]
    for item in ('url', 'cgi'):
        for key, value in options['data']['configurations'].iteritems():
            if key == 'default':
                pass
            else:
                if value.get('backend'):
                    backends = [
                        '{}'.format(group) for group in options['data']['synonyms']['backend'][value['backend']]
                    ]
                else:
                    backends = default_backends

                location_modules = [
                    (Modules.Balancer, {
                        'backends': backends,
                        'proxy_options': OrderedDict([
                            ('keepalive_count', 10),
                        ])
                    })
                ]

                cgi_rewrite = {}
                if value.get('cgi'):
                    cgi_rewrite = {
                        'regexp': '(.*)',
                        'rewrite': '%1{}'.format(
                            ''.join(['&source={}'.format(c) for c in value['cgi']['source']])
                        )
                    }

                url_rewrite = {}
                if item == 'url':
                    regexp = {'match_fsm': OrderedDict([
                        ('url', '/search/{}((/|\\\\?).+)?'.format(key))
                    ])}
                    url_rewrite = {
                        'regexp': '/search/{}(/)?(.*)?'.format(key),
                        'rewrite': '/%2',
                        'split': 'url',
                    }
                else:
                    if 'urlfilter' in value:
                        if 'collection' in value['urlfilter']:
                            regexp = {'match_fsm': OrderedDict([
                                ('url', '/{}?'.format(value['urlfilter']['collection']))
                            ])}
                        else:
                            regexp = {'match_fsm': OrderedDict([
                                ('CGI', '{}={}'.format(
                                    value['urlfilter'].keys()[0],
                                    value['urlfilter'].values()[0]
                                )),
                                ('surround', 'true')
                            ])}
                    else:
                        regexp = {'match_fsm': OrderedDict([
                            ('CGI', 'search={}'.format(key)),
                            ('surround', 'true')
                        ])}
                # fill actions for rewrite module
                if cgi_rewrite or url_rewrite:
                    actions = list()
                    if cgi_rewrite:
                        actions.append(cgi_rewrite)

                    if url_rewrite:
                        actions.append(url_rewrite)

                    rewrite = (Modules.Rewrite, {
                        'actions': actions
                    })
                    location_modules.insert(0, rewrite)

                regexp_modules.append(('{}_{}'.format(key, item), regexp, location_modules))

    '''
    Usage: MAN_ADDRS_BASE(itag=OPT_shardid=001) (OPT_shardid MUST be enabled)

    /business/stable/0 -> [SAS,MAN,VLA]_ADDRS_BASE_S1 OPT_shardid=000
    /business/stable/1 -> [SAS,MAN,VLA]_ADDRS_BASE_S1 OPT_shardid=001
    /business/stable/2 -> [SAS,MAN,VLA]_ADDRS_BASE_S1 OPT_shardid=002
    /business/stable/3 -> [SAS,MAN,VLA]_ADDRS_BASE_S1 OPT_shardid=003
    '''

    regexp_modules.append(
        ('default', {}, [
            (Modules.Balancer, {
                'backends': default_backends,
                'proxy_options': OrderedDict([
                    ('keepalive_count', 10)
                ])
            })
        ])
    )

    def get_section(ssl=False):
        return [
            (Macroses.ExtendedHttpV2, {
                'port': 14580,
                'maxlen': 700 * 1024,
                'maxreq': 700 * 1024,
                'ssl_enabled': ssl,
                'fqdn': 'addrs-testing.search.yandex.net',
                'ticket_keys_enabled': True,
                'force_ssl': True,
            }),
            (Modules.Headers, {'create_func': OrderedDict([
                ('X-Forwarded-For-Y', 'realip'),
                ('X-Source-Port-Y', 'realport'),
                ('X-Start-Time', 'starttime'),
                ('X-Req-Id', 'reqid')
            ])}),
            (Modules.HeadersForwarder, {
                'actions': [({
                    'request_header': 'Origin',
                    'response_header': 'Access-Control-Allow-Origin',
                    'erase_from_request': True,
                    'erase_from_response': True,
                    'weak': False
                })]
            }),
            (Modules.Regexp, [
                (Macroses.SlbPing, {
                    'errordoc': True,
                    'check_pattern': '/yandsearch\\\\?info=getstatus',
                }),
                ('default', {}, [
                    (Modules.Rpcrewrite, {
                        'url': '/proxy',
                        'host': 'bolver.yandex-team.ru',
                        'dry_run': False,
                        'rpc_success_header': 'X-Metabalancer-Answered',
                        'rpc': [
                            (Modules.Balancer2, {
                                'backends': ['bolver.yandex-team.ru:80'],
                                'attempts': 3,
                                'balancer_type': 'rr',
                                'stats_attr': 'rpcrewrite',
                                'policies': OrderedDict([
                                    ('retry_policy', {
                                        'unique_policy': {}
                                    })
                                ])
                            })
                        ]
                    }),
                    (Modules.Regexp, regexp_modules)
                ])
            ])
        ]

    return [
        (Modules.Main, {
            'workers': 0,
            'enable_reuse_port': True,
            'port': 14580,
            'admin_port': 14580,
            'config_tag': options['config_tag'],
            'logs_path': '/place/db/www/logs'
        }),
        (Modules.Ipdispatch, [
            (Macroses.BindAndListen, {
                'port': 14580,
                'outerports': [17140, 80],
                'domain': 'addrs-testing.search.yandex.net',
                'modules': get_section()
            }),
            (Macroses.BindAndListen, {
                'port': 14580,
                'outerports': [443],
                'no_localips': True,
                'domain': 'addrs-testing.search.yandex.net',
                'modules': get_section(ssl=True)
            })
        ])
    ]


def process(options):
    params = options.params
    output_file = options.output_file
    transport = options.transport
    prjs = ('testing')
    assert len(params) == 1 and params[0] in prjs, "Wrong parameters"

    try:
        fname = os.path.join(
            os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
            'addrs', 'testing_configurations.json'
        )
        fname2 = os.path.join(
            os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
            'addrs', 'balancer_tags.json'
        )
        with open(fname) as cfile, open(fname2) as cfile2:
            cfg = templateAddrsTesting({
                'data': json.load(cfile),
                'data2': json.load(cfile2)
            })

    except IOError as err:
        print "File error:  {}".format(err)

    tun_prefix = Constants.GetIpByIproute
    generate(cfg, output_file, instance_db_transport=transport, user_config_prefix=tun_prefix)
