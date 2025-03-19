"""
Variables that influence testing behavior are defined here.
"""

import os
import random

from tests.func.helpers.utils import merge

try:
    from local_configuration import CONF_OVERRIDE
except ImportError:
    CONF_OVERRIDE = {}


def get():
    """
    Get configuration (non-idempotent function)
    """
    # This "factor" is later used in the network name and port forwarding.
    port_factor = random.randint(0, 4096)  # nosec
    # Docker network name. Also used as a project and domain name.
    net_name = 'testnet{num}'.format(num=port_factor)

    config = {
        # Common conf options.
        # See below for dynamic stuff (keys, certs, etc)
        # Controls whether to perform cleanup after tests execution or not.
        'cleanup': True,
        # Code checkout
        # Where does all the fun happens.
        # Assumption is that it can be safely rm-rf`ed later.
        'staging_dir': 'staging',
        # Default repository to pull code from.
        'git_repo_base': os.environ.get('DBAAS_INFRA_REPO_BASE', 'ssh://{user}@review.db.yandex.net:9501'),
        # Controls whether overwrite existing locally checked out
        # code or not (default)
        'git_clone_overwrite': False,
        # If present, git.checkout_code will attempt to checkout this topic.
        'gerrit_topic': os.environ.get('GERRIT_TOPIC'),
        # Docker-related
        'docker_ip4_subnet': '10.%s.0/24',
        'docker_ip6_subnet': 'fd00:dead:beef:%s::/96',
        # See above.
        'port_factor': port_factor,
        # These docker images are brewed on `docker.prep_images` stage.
        # Options below are passed as-is to
        # <docker_api_instance>.container.create()
        'network_name': net_name,
        # Docker network name. Also doubles as a project and domain name.
        'projects': {
            # Basically this mimics docker-compose 'service'.
            # Matching keys will be used in docker-compose,
            # while others will be ignored in compose file, but may be
            # referenced in any other place.
            'base': {
                # The base needs to be present so templates,
                # if any, will be rendered.
                # It is brewed by docker directly,
                # and not used in compose environment.
                'docker_instances': 0,
            },
            'mongodb': {
                'build': '..',
                # Config can have arbitrary keys.
                # This one is used in template matching of config file options.
                # See Dockerfile itself for examples.
                # 'docker_instances': 3,
                'users': {
                    'admin': {
                        'username': 'admin',
                        'password': 'password',
                        'dbname': 'admin',
                        'roles': ['root'],
                    },
                },
                'expose': {
                    'mongod': 27018,
                    'ssh': 22,
                },
                'docker_instances': 3,
                'args': {
                    'MONGODB_VERSION': '$MONGODB_VERSION',
                },
            },
        },
        # A dict with all projects that are going to interact in this
        # testing environment.
        'base_images': {
            'mdb-mongo-tools-base': {
                'tag': 'mdb-mongo-tools-base',
                'path': 'images/base',
            },
        },
    }
    return merge(config, CONF_OVERRIDE)
