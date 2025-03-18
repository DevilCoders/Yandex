from collections import OrderedDict

import src.macroses as Macroses
import src.modules as Modules

import helpers


RESOLVER_HOST = 'resolver.clusterstate.yandex-team.ru(:\\\\d+)?'
SAS_TRACKER_HOST = 'sas-tracker.clusterstate.yandex-team.ru(:\\\\d+)?'
PSI_DISPENSER_HOST = 'psi.yandex-team.ru(:\\\\d+)?'


def get_modules(https_postfix):
    return [
        (Macroses.SlbPing, {
            'errordoc': True,
            'check_pattern':  '/slb_ping'
        }),
        ('resolver', {'match_fsm': OrderedDict([('host', RESOLVER_HOST)])}, [
            (Modules.Report, {'uuid': 'resolver'}),
            (Modules.Balancer, {
                'balancer_type': 'hashing',
                'backends': [
                    'MAN_RESOLVED',
                    'MAN_SG_RESOLVER',
                    'SAS_RESOLVED',
                    'SAS_SG_RESOLVER',
                    'VLA_RESOLVED',
                    'VLA_SG_RESOLVER',
                ],
            }),
        ]),
        ('psidispenser', {'match_fsm': OrderedDict([('host', PSI_DISPENSER_HOST)])}, [
            (Modules.Regexp, [
                ('default', {}, [
                    (Modules.Balancer, {
                        'balancer_type': 'hashing',
                        'backends': [
                            'MAN_PSI_DISPENSER',
                            'SAS_PSI_DISPENSER',
                            'VLA_PSI_DISPENSER',
                        ],
                    }),
                ]),
            ]),
        ]),
        ('sas-tracker', {'match_fsm': OrderedDict([('host', SAS_TRACKER_HOST)])}, [
            (Modules.Regexp, [
                ('default', {}, [
                    (Modules.Balancer2, {
                        'balancer_type': 'rr',
                        'policies': OrderedDict([
                            ('unique_policy', {})
                        ]),
                        'proxy_options': OrderedDict([
                            ('backend_timeout', '60s'),
                            ('fail_on_5xx', False),
                        ]),
                        'backends': helpers.endpointsets('sas-cajuper-tracker', {'sas'}),
                    }),
                ]),
            ]),
        ]),
    ]
