"""
dbm mock configuration.
"""

# flake8: noqa
# pylint: skip-file

import json

OAUTH_TOKEN = '{{ conf.projects.fake_dbm.config.oauth.token }}'

DBM = {
    'network_name': '{{conf.network_name}}',
}

EXPOSES = json.loads("""{{ conf.projects.fake_dbm.containers.exposes | tojson }}""")
BOOTSTRAPS = json.loads("""{{ conf.projects.fake_dbm.containers.bootstraps | tojson }}""")
