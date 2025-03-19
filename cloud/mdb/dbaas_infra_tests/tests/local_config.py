"""
Override default configuration with ~/.dbaas-infrastructure-test.yaml
"""

import os

import yaml

CONF_OVERRIDE = {}
PATH = os.path.expanduser('~/.dbaas-infrastructure-test.yaml')

try:
    if os.path.exists(PATH):
        with open(PATH) as CONF:
            CONF_OVERRIDE.update(yaml.load(CONF))
except Exception as exc:
    print('Unable to load local config: {exc}'.format(exc=repr(exc)))
