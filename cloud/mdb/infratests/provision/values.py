"""
Populate values.yaml
"""

import os
import yaml

from cloud.mdb.infratests.config import InfratestConfig
from cloud.mdb.internal.python.lockbox import Secret


def fill_values(config: InfratestConfig, public_api_cert_secret: Secret, private_api_cert_secret: Secret):
    base_domain = config.base_domain
    values = {
        'api-adapter': {
            'balancer': f'api-adapter.private-api.{base_domain}',
        },
        'global': {
            'environment': {
                'mdb': {
                    'v1': {
                        'services': {
                            'dataproc_manager': {
                                'balancer': f'dataproc-manager.private-api.{base_domain}',
                                'public_address': f'dataproc-manager.api.{base_domain}',
                            },
                            'go_api': {
                                'balancer': f'mdb-internal-api.private-api.{base_domain}',
                            },
                            'py_api': {
                                'address': f'mdb.private-api.{base_domain}',
                            },
                        },
                    },
                },
                'public_api': {
                    'address': f'api.{base_domain}:443',
                },
            },
        },
        'public-api': {
            'base_domain': f'api.{base_domain}',
            'tls': {
                'key_lockbox_secret': {
                    'id': public_api_cert_secret.id,
                    'version': public_api_cert_secret.current_version.id,
                    'key': 'key',
                },
                'crt_lockbox_secret': {
                    'id': public_api_cert_secret.id,
                    'version': public_api_cert_secret.current_version.id,
                    'key': 'crt',
                },
            },
        },
        'tls': {
            'external': {
                'key_lockbox_secret': {
                    'id': private_api_cert_secret.id,
                    'version': private_api_cert_secret.current_version.id,
                    'key': 'key',
                },
                'crt_lockbox_secret': {
                    'id': private_api_cert_secret.id,
                    'version': private_api_cert_secret.current_version.id,
                    'key': 'crt',
                },
            },
        },
    }

    values_path = "stand_values.yaml"
    filename = os.path.join(config.helmfile_dir_path, values_path)
    with open(filename, 'w') as outfile:
        yaml.dump(values, outfile, default_flow_style=False)
