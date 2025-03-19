"""
Variables that influence testing behavior are defined here.
"""
# pylint: disable=too-many-lines

import copy
import getpass
import os
import random
import uuid
from itertools import chain
from typing import Iterable, NamedTuple

from netaddr import IPNetwork

from tests.default_versions import DEFAULT_VERSIONS
from tests.helpers import crypto
from tests.helpers.docker import generate_ipv4, generate_ipv6
from tests.helpers.utils import merge
from tests.local_config import CONF_OVERRIDE
from tests.resources import RESOURCES


class MajorVersionImage(NamedTuple):
    """
    Major Version Image
    """
    # Used for:
    # - copy staging config from given container type
    # - worker config
    kind: str
    bootstrap_cmd: str
    # copy expose and ports from given container type
    expose_same_as: str
    # how image called used in:
    # - staging path
    # - docker images
    name: str
    # s3 image path
    s3_path: str
    # version in terms of images
    version: str
    # version in terms of API
    major_version: str


def _get_major_versions_images() -> Iterable[MajorVersionImage]:
    """
    Get versions of `per-major-version` images
    """
    return chain(
        [
            MajorVersionImage(
                kind='postgresql',
                bootstrap_cmd='/usr/local/yandex/porto/mdb_dbaas_pg_{}_bionic.sh'.format(ver),
                expose_same_as='postgresql-bionic',
                name='postgresql-{}'.format(ver),
                s3_path='postgresql-{}-bionic/'.format(ver),
                version=ver,
                major_version=major_version,
            ) for (major_version, ver) in {
                '10': '10',
                '10-1c': '10-1c',
                '11': '11',
                '11-1c': '11-1c',
                '12': '12',
                '12-1c': '12-1c',
                '13': '13',
                '13-1c': '13-1c',
                '14': '14',
                '14-1c': '14-1c',
            }.items()
        ],
        [
            MajorVersionImage(
                kind='elasticsearch',
                bootstrap_cmd='/usr/local/yandex/porto/mdb_dbaas_elasticsearch_{}_bionic.sh'.format(ver),
                expose_same_as='elasticsearch-bionic',
                name='elasticsearch-{}'.format(ver),
                s3_path='elasticsearch-{}-bionic/'.format(ver),
                version=ver,
                major_version=major_version,
            ) for (major_version, ver) in {
                '710': '710',
                '711': '711',
                '712': '712',
                '713': '713',
                '714': '714',
                '715': '715',
                '716': '716',
                '717': '717',
            }.items()
        ],
        [
            MajorVersionImage(
                kind='opensearch',
                bootstrap_cmd='/usr/local/yandex/porto/mdb_dbaas_opensearch_{}_bionic.sh'.format(ver),
                expose_same_as='opensearch-bionic',
                name='opensearch-{}'.format(ver),
                s3_path='opensearch-{}-bionic/'.format(ver),
                version=ver,
                major_version=major_version,
            ) for (major_version, ver) in {
                '21': '21',
            }.items()
        ],
        [
            MajorVersionImage(
                kind='redis',
                bootstrap_cmd='/usr/local/yandex/porto/mdb_dbaas_redis_{}_bionic.sh'.format(ver),
                expose_same_as='redis-bionic',
                name='redis-{}'.format(ver),
                s3_path='redis-{}-bionic/'.format(ver),
                version=ver,
                major_version=major_version,
            ) for (major_version, ver) in {
                '5.0': '50',
                '6.0': '60',
                '6.2': '62',
                '7.0': '62',
            }.items()
        ],
    )


def make_network_name(prefix):
    """
    Generate new network name
    """
    return '{prefix}{suffix}'.format(prefix=prefix, suffix=random.randint(0, 4096))


def ya_make_cmd(target):
    """
    Return ya make command for target

    target should be a path relative to cloud/mdb
    """
    return f"../../../ya make -j2 --output ../../../ --no-src-links ../{target}"


def get():
    """
    Get configuration (non-idempotent function)
    """
    # Docker network name. Also used as a project and domain name.
    net_name = CONF_OVERRIDE.get('network_name', make_network_name(
        CONF_OVERRIDE.get('network_name_prefix', 'test.net')))

    # https://bb.y-t.ru/projects/JUGGLER/repos/monrun/browse/gethostname.py#24
    if len(net_name.split('.')) < 2:
        raise RuntimeError('Network name should have at least 2 ' 'domains for monrun to work')
    dynamic_config = generate_dynamic_config(net_name)

    config = {
        # noqa
        # Common conf options.
        'jwt': {
            'key_id': 'bfbe0t16tej0urdn1i0l',
            'private_key':
                '-----BEGIN PRIVATE KEY-----\\nMIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQDmAhvKMFgz0Zdo\\ncj7'
                '+l+y52ggM3kgrIg1CDDDFjQqs9Lny35zSPeXWE4aBOHQhGEsNQ7h4ey+JvhMB\\n'
                '+10Z3HfHBN6zOIoOrzc6Is3AO4WTosG4wYQDOPCLIFxjZ5jcFsrzUu4FA3oOK67e\\nSfyu'
                '/8Nsy4tyFEX4ctLJV9F2ZAdNvTcmejqiEChkL/bTvvAWVITFCkgQn+6AJOwN'
                '\\nJlcupsrERkxhQJfE0EOIHnmBBmTh7o135sXGrMJIMzHgYB7+Mm9hmOMO3f0VEgtw\\ntgYOPfPmbSxr4E6Wug'
                '/IlhiYj7EbGw9ZcvzqqPdMlfx5vGvEx574eCU+BACctsF+\\no39sEjTjAgMBAAECggEBAJdfTeC7'
                '/mBkEKw5fAHcBHyv5Fprs71HVMhh83sf/qWj\\nDfmsdq3a9Lb19LB4cd0R+trI9c+86qVRRXspbbVGZkHBj7sNe7Z8U'
                '/Fb1GMuCRQH\\nqkyodvQT9iLp8kOte2llSr3mlyUPi7VlxFkhAj49ruSb9LLoasA++UBvHjg3TqM2\\no35dUvGq1ePgbZobQw'
                '/ZzoggM6CDp1q3bKIG3RX+uuiA/Za0hmNeRsZHPKg7FmQJ\\nrHoscFfuwK7YH2CgPTXLWwpS3TXst'
                '+libeLuXvS30V1COTGxLYW5E3AlUEd0tleh\\nrmtm12YooGW1mRt3lnKVK6f1pwnEXZNNFFVjtz08p+kCgYEA8t6C/TDsO'
                '+NRuF5K\\ni6JqWrXfBdSFvAcuO92zb0S/reQ8y2PeisSg2P+/ihu7pubBfInpzNfGvds8PmO6'
                '\\nVtvzKMp8Cev2khLpG7Mz497UPK3F2oMpztaCi/c49JAkqbKW+dichdGpRbYGrgyx'
                '\\nnbsL6Uqa7JuLYs0AKdBmNhFPIdcCgYEA8nGXhJPt1kz9pTcfMEqxr8dGZZmADjFv\\n9tyKR2Q2/HLZmUFp+VfiAXd'
                '/FOkc57xqJ/1Wbwcbw8F/c8g+QeR638dx/QjxY/Th\\nYKvNLnWHfLo4Uz64fHIGtQViGai+zW3emsU8XxO6Xux5jd5'
                '+TWqwu9PUw2bmqawb\\n2kp4iro1u9UCgYBr+/k7wAvZGNpV4j681Qr6qBCwU+zeTEcHQSyt1WBwXaGWQSJK'
                '\\niuCFezjbnDcUH1d6GwvEE1B/S8H+b3MDeaokwdriwnKQQi45LbVtpL6y+ASXgmgN\\nWh0TRGmje4+BkDFGh0QYz762ixdvPZ'
                '+fZPIH2S8G8qXH2SQwc0Nu2MVZYwKBgDsG\\n1PD'
                '+YyWN0SNsbDeBuAkn50fNO5Q5DR15TGFdUNXd0ISznG2MrAXXZiVdLCBvixj5\\nYRXfES9z6OfzlNTOH'
                '+xjzqjgiIThlg3HRklNbBM984CxAJGr4V4pVV0R7IJvgYcF\\nBlHPp8x8nouf4'
                '/hNRYI1bNO2NeqRcKaRAxAjjfmRAoGBAMVQREaufM+NXbP9rS7t'
                '\\nda7jJ0RgjrJg9Ejz7hSL0vEJ5A9AUlNPy31trMmV86u1u20h8pbl43ER/LqwMjBN'
                '\\nWxdsCW1RcRvrcJSAnLbicz8CtzUdbXPWsxTsFwzr4z1EVxWeJZ107EQIO79HbvXW\\nfl5I9UwEuQQQLKRhgYCWIZK8\\n'
                '-----END PRIVATE KEY-----\\n',
        },
        # See below for dynamic stuff (keys, certs, etc)
        'dynamic':
            dynamic_config,
        # Controls whether to perform cleanup after tests execution or not.
        'cleanup':
            True,
        # Controls deploy options
        'deploy': {
            # Name of deploy group for deploy V2
            'group': 'mdb-infra-test-deploy-group',
            'token': 'testtoken',
        },
        # Mlock options
        'mlock': {
            # This token is actualy a garbage generated with fake_iam (mocked access service accepts anything)
            'token': 'CgwVAgAAABoFZHVtbXkSgASVCmFiiQorhciSfpn4SEQF5PTbg2Mx-M0ju52zg7fRxTqDDAkYSu3wytUQv5-Py5zZ9UrPh'
                     'U5ny8ENY-Qof7fBT2WeEM9we7nfaI50Ejr_KU5kQ7TEPkZ28wGfb5M57OeB8WFFVN9iVbBDqVwa5Xni50CLtYqIjmfKH5'
                     'OuN6G8xPdxUSZdBb7C48tsgan-7Hs_FjYTvvdX92nwh_zNCN_Dl7yhZGv_noB01yfCiHWeClGARw28k9PX2Geg2vl3V-R'
                     'qogLL2Vd7CCiunNWFuIF-CTLDr5QzH6ijy-SxmDhz9ADSXktVH7mKxGvYGl9t735GzeWgafwkVbzn7yOikbRTQWOIQoFf'
                     'qNII3zaxpknB8-Ys0l980Hu26DTUpc8XlqPGR0-HvgPISqHIYh15i-gS_wZl8QJ0UVvxwwwSFAyISk3P_AHhT4E2Glmkn'
                     '99U273qla7OX99xaTPypMZIEq7Z0T471Cahz8LbkPAHaXEjWkvgDjz4EnxFxQ2vVO3qUivueB9fXsWNTchAnFDaq-JBqQ'
                     'J-DxZZJCSweN2LmUWqxhFXFV8s8O1Iqo3Mm1wqrGrASLftuedynELmuVluujPJer2UPaRorCJWT-OWKSP7TAcZaSKeveT'
                     'F9WRNPqu_Q-klvEQffUA2UmTIJQ79NbXuHy8GQO-i3_Oh1wu2l04GzhpWCiAxMGNkZTVlNTZhNTE0M2JmYmYxY2YxOGNh'
                     'MGJmNDIxZBCI0o_cBRiI2L_yBiIXCgVkdW1teRIFZHVtbXkqBWR1bW15MAIwBTgBWgkKBWR1bW15EAE',
        },
        'worker': {
            'postgresql': {
                'dbm_bootstrap_cmd': '/usr/local/yandex/porto/mdb_dbaas_pg_bionic.sh',
                'dbm_bootstrap_cmd_template': {
                    'template': '/usr/local/yandex/porto/mdb_dbaas_pg_{major_version}_bionic.sh',
                    'task_arg': 'major_version',
                    'whitelist': {},
                },
            },
            'elasticsearch': {
                'dbm_bootstrap_cmd': '/usr/local/yandex/porto/mdb_dbaas_elasticsearch_bionic.sh',
                'dbm_bootstrap_cmd_template': {
                    'template': '/usr/local/yandex/porto/mdb_dbaas_elasticsearch_{major_version}_bionic.sh',
                    'task_arg': 'major_version',
                    'whitelist': {},
                },
            },
            'opensearch': {
                'dbm_bootstrap_cmd': '/usr/local/yandex/porto/mdb_dbaas_opensearch_bionic.sh',
                'dbm_bootstrap_cmd_template': {
                    'template': '/usr/local/yandex/porto/mdb_dbaas_opensearch_{major_version}_bionic.sh',
                    'task_arg': 'major_version',
                    'whitelist': {},
                },
            },
            'redis': {
                'dbm_bootstrap_cmd': '/usr/local/yandex/porto/mdb_dbaas_redis_bionic.sh',
                'dbm_bootstrap_cmd_template': {
                    'template': '/usr/local/yandex/porto/mdb_dbaas_redis_{major_version}_bionic.sh',
                    'task_arg': 'major_version',
                    'whitelist': {},
                },
            },
        },
        # At slow matches override it local_configuration
        'retries_multiplier':
            1,
        # Code checkout
        # Where does all the fun happens.
        # Assumption is that it can be safely rm-rf`ed later.
        'staging_dir':
            'staging',
        # Default repo bases to pull code from.
        'repo_bases': {
            'gerrit':
                os.environ.get('DBAAS_INFRA_REPO_BASE',
                               'ssh://{user}@review.db.yandex-team.ru:9440').format(user=getpass.getuser()),
            'github':
                os.environ.get('DBAAS_GITHUB_REPO_BASE', 'https://github.yandex-team.ru'),
            'bitbucket':
                os.environ.get('DBAAS_BB_REPO_BASE', 'ssh://git@bb.yandex-team.ru'),
        },
        # Controls whether overwrite existing locally checked out
        # code or not (default)
        'git_clone_overwrite':
            False,
        # If present, git.checkout_code will attempt to checkout this topic
        # in case of gerrit checkout
        'gerrit_topic':
            os.environ.get('GERRIT_TOPIC'),
        # These docker images are brewed on `docker.prep_images` stage.
        # Options below are passed as-is to
        # <docker_api_instance>.container.create()
        'base_images': {
            'dbaas-infra-tests-base': {
                'tag': 'dbaas-infra-tests-base',
                'path': 'staging/images/base',
            },
            'base-deploy': {
                'tag': 'base-deploy',
                'path': 'staging/images/base-deploy',
            },
            'postgresql': {
                'tag': 'dbaas-infra-tests-postgresql-bionic:{tag}'.format(tag=net_name),
                'path': 'staging/images/postgresql',
            },
            'clickhouse': {
                'tag': 'dbaas-infra-tests-clickhouse-bionic:{tag}'.format(tag=net_name),
                'path': 'staging/images/clickhouse',
            },
            'zookeeper': {
                'tag': 'dbaas-infra-tests-zookeeper-bionic:{tag}'.format(tag=net_name),
                'path': 'staging/images/zookeeper-template',
            },
            'mongodb': {
                'tag': 'dbaas-infra-tests-mongodb-bionic:{tag}'.format(tag=net_name),
                'path': 'staging/images/mongodb',
            },
            'redis': {
                'tag': 'dbaas-infra-tests-redis-bionic:{tag}'.format(tag=net_name),
                'path': 'staging/images/redis',
            },
            'mysql': {
                'tag': 'dbaas-infra-tests-mysql-bionic:{tag}'.format(tag=net_name),
                'path': 'staging/images/mysql',
            },
            'elasticsearch': {
                'tag': 'dbaas-infra-tests-elasticsearch-bionic:{tag}'.format(tag=net_name),
                'path': 'staging/images/elasticsearch',
            },
            'opensearch': {
                'tag': 'dbaas-infra-tests-opensearch-bionic:{tag}'.format(tag=net_name),
                'path': 'staging/images/opensearch',
            },
            'greenplum': {
                'tag': 'dbaas-infra-tests-greenplum-bionic:{tag}'.format(tag=net_name),
                'path': 'staging/images/greenplum',
            },
        },  # noqa: E122
        'versioned_images': {
            # Per major-version images setup.
            # Filled later.
        },
        'repos': {
            'go-mdb': {
                'base': 'gerrit',
                'repo': 'mdb/go-mdb',
            },
        },
        # Docker network name. Also doubles as a project and domain name.
        'network_name':
            net_name,
        's3-templates': {
            'host': 's3.mds.yandex.net',
            # Using test image in our tests
            'bucket': 'dbaas-images-vm-built-test',
            'images': {
                'ch-bionic-template': {
                    'path': 'clickhouse-bionic/',
                },
                'mongodb-bionic-template': {
                    'path': 'mongodb-bionic/',
                },
                'pg-bionic-template': {
                    'path': 'postgresql-bionic/',
                },
                'zk-bionic-template': {
                    'path': 'zookeeper-bionic/',
                },
                'redis-bionic-template': {
                    'path': 'redis-bionic/',
                },
                'mysql-bionic-template': {
                    'path': 'mysql-bionic/',
                },
                'elasticsearch-bionic-template': {
                    'path': 'elasticsearch-bionic/',
                },
                'opensearch-bionic-template': {
                    'path': 'opensearch-bionic/',
                },
                'greenplum-bionic-template': {
                    'path': 'greenplum-bionic/',
                },
            },
        },
        'disk_type_config': [{
            'disk_type_id': 1,
            'quota_type': 'ssd',
            'disk_type_ext_id': 'local-ssd',
        }],
        'regions': [{
            'region_id': 1,
            'name': 'ru-central-1',
            'cloud_provider': 'yandex',
        }],
        'geos': [{
            'geo_id': 1,
            'name': 'man',
            'region_id': 1,
        }, {
            'geo_id': 2,
            'name': 'sas',
            'region_id': 1,
        }, {
            'geo_id': 3,
            'name': 'vla',
            'region_id': 1,
        }, {
            'geo_id': 4,
            'name': 'iva',
            'region_id': 1,
        }, {
            'geo_id': 5,
            'name': 'myt',
            'region_id': 1,
        }],
        'feature_flags': ['MDB_GREENPLUM_CLUSTER'],
        'flavors': [{
            'id': '6aaf915e-ceb2-4b46-8f18-b71c2d4694d1',
            'cpu_guarantee': 0.5,
            'cpu_limit': 1.0,
            'memory_guarantee': 1073741824,
            'memory_limit': 2147483648,
            'network_guarantee': 1048576,
            'network_limit': 16777216,
            'io_limit': 5242880,
            'name': 'db1.nano',
            'visible': True,
            'vtype': 'porto',
            'platform_id': '1',
        },
                    {
                        'id': '6a9f8640-cb56-4afe-985d-0a407936ca50',
                        'cpu_guarantee': 0.5,
                        'cpu_limit': 1.0,
                        'memory_guarantee': 107374,
                        'memory_limit': 214748,
                        'network_guarantee': 1048576,
                        'network_limit': 16777216,
                        'io_limit': 5242880,
                        'name': 'db1.dnische',
                        'visible': True,
                        'vtype': 'porto',
                        'platform_id': '1',
                    },
                    {
                        'id': '7e589410-c8c9-4a66-bac4-09d9dfb5de5e',
                        'cpu_guarantee': 0.5,
                        'cpu_limit': 1.0,
                        'memory_guarantee': 160000000,
                        'memory_limit': 320000000,
                        'network_guarantee': 1048576,
                        'network_limit': 16777216,
                        'io_limit': 5242880,
                        'name': 'db1.dnische2',
                        'visible': True,
                        'vtype': 'porto',
                        'platform_id': '1',
                    },
                    {
                        'id': '33f89442-0994-41ff-984a-6763e45bba82',
                        'cpu_guarantee': 1.0,
                        'cpu_limit': 1.0,
                        'memory_guarantee': 4294967296,
                        'memory_limit': 8589934592,
                        'network_guarantee': 2097152,
                        'network_limit': 16777216,
                        'io_limit': 20971520,
                        'name': 'db1.micro',
                        'visible': True,
                        'vtype': 'porto',
                        'platform_id': '1',
                    },
                    {
                        'id': '325648ce-2daa-4046-b958-d5a9fe27ccfe',
                        'cpu_guarantee': 2.0,
                        'cpu_limit': 2.0,
                        'memory_guarantee': 8589934592,
                        'memory_limit': 17179869184,
                        'network_guarantee': 4194304,
                        'network_limit': 33554432,
                        'io_limit': 41943040,
                        'name': 'db1.small',
                        'visible': True,
                        'vtype': 'porto',
                        'platform_id': '1',
                    },
                    {
                        'id': '21f29f4a-17c5-4403-923b-2fae3089027a',
                        'cpu_guarantee': 4.0,
                        'cpu_limit': 4.0,
                        'memory_guarantee': 17179869184,
                        'memory_limit': 34359738368,
                        'network_guarantee': 8388608,
                        'network_limit': 33554432,
                        'io_limit': 83886080,
                        'name': 'db1.medium',
                        'visible': True,
                        'vtype': 'porto',
                        'platform_id': '1',
                    },
                    {
                        'id': 'c4b807f9-72ff-4322-aaba-e8a995b8bad5',
                        'cpu_guarantee': 8.0,
                        'cpu_limit': 8.0,
                        'memory_guarantee': 34359738368,
                        'memory_limit': 68719476736,
                        'network_guarantee': 16777216,
                        'network_limit': 67108864,
                        'io_limit': 167772160,
                        'name': 'db1.large',
                        'visible': True,
                        'vtype': 'porto',
                        'platform_id': '1',
                    },
                    {
                        'id': 'ad8e7269-c87b-41a7-bc2e-3db64ee23e1d',
                        'cpu_guarantee': 16.0,
                        'cpu_limit': 16.0,
                        'memory_guarantee': 68719476736,
                        'memory_limit': 137438953472,
                        'network_guarantee': 33554432,
                        'network_limit': 134217728,
                        'io_limit': 335544320,
                        'name': 'db1.xlarge',
                        'visible': True,
                        'vtype': 'porto',
                        'platform_id': '1',
                    }],
        'resources_config':
            RESOURCES,
        'default_versions_config':
            DEFAULT_VERSIONS,
        'test_cluster_configs': {
            'postgresql': {
                'standard': {
                    'description':
                        'Standart PostgreSQL cluster',
                    'labels': {
                        'acid': 'surely',
                    },
                    'environment':
                        'PRESTABLE',
                    'configSpec': {
                        'version': '10',
                        'postgresqlConfig_10': {
                            'logMinDurationStatement': 1000,
                            'sharedBuffers': 134217728,
                        },
                        'poolerConfig': {
                            'poolingMode': 'TRANSACTION',
                        },
                        'resources': {
                            'resourcePresetId': 'db1.nano',
                            'diskTypeId': 'local-ssd',
                            'diskSize': 10737418240,
                        },
                    },
                    'databaseSpecs': [{
                        'name': 'testdb',
                        'owner': 'test_user',
                        'extensions': [{
                            'name': 'btree_gist',
                        }],
                        'lcCollate': 'fi_FI.UTF-8',
                        'lcCtype': 'fi_FI.UTF-8',
                    }],
                    'userSpecs': [{
                        'name': 'test_user',
                        'connLimit': 10,
                        'password': 'mysupercooltestpassword11111',
                    }, {
                        'name': 'another_test_user',
                        'connLimit': 10,
                        'password': 'mysupercooltestpassword11111',
                    }],
                    'hostSpecs': [{
                        'zoneId': 'man',
                    }, {
                        'zoneId': 'sas',
                    }, {
                        'zoneId': 'vla',
                    }],
                },
                'standard 10-1c': {
                    'description':
                        'Standart PostgreSQL 10-1c cluster',
                    'labels': {
                        'acid': 'surely',
                    },
                    'environment':
                        'PRESTABLE',
                    'configSpec': {
                        'version': '10-1c',
                        'postgresqlConfig_10_1c': {
                            'logMinDurationStatement': 1000,
                            'sharedBuffers': 134217728,
                        },
                        'poolerConfig': {
                            'poolingMode': 'TRANSACTION',
                        },
                        'resources': {
                            'resourcePresetId': 'db1.nano',
                            'diskTypeId': 'local-ssd',
                            'diskSize': 10737418240,
                        },
                    },
                    'databaseSpecs': [{
                        'name': 'testdb',
                        'owner': 'test_user',
                    }],
                    'userSpecs': [{
                        'name': 'test_user',
                        'connLimit': 10,
                        'password': 'mysupercooltestpassword11111',
                    }],
                    'hostSpecs': [{
                        'zoneId': 'man',
                    }, {
                        'zoneId': 'sas',
                    }, {
                        'zoneId': 'vla',
                    }],
                },
                'standard 11': {
                    'description':
                        'Standart PostgreSQL 11 cluster',
                    'labels': {
                        'acid': 'surely',
                    },
                    'environment':
                        'PRESTABLE',
                    'configSpec': {
                        'version': '11',
                        'postgresqlConfig_11': {
                            'logMinDurationStatement': 1000,
                            'sharedBuffers': 134217728,
                        },
                        'poolerConfig': {
                            'poolingMode': 'TRANSACTION',
                        },
                        'resources': {
                            'resourcePresetId': 'db1.nano',
                            'diskTypeId': 'local-ssd',
                            'diskSize': 10737418240,
                        },
                    },
                    'databaseSpecs': [{
                        'name': 'testdb',
                        'owner': 'test_user',
                        'extensions': [{
                            'name': 'btree_gist',
                        }],
                        'lcCollate': 'fi_FI.UTF-8',
                        'lcCtype': 'fi_FI.UTF-8',
                    }],
                    'userSpecs': [{
                        'name': 'test_user',
                        'connLimit': 10,
                        'password': 'mysupercooltestpassword11111',
                    }, {
                        'name': 'another_test_user',
                        'connLimit': 10,
                        'password': 'mysupercooltestpassword11111',
                    }],
                    'hostSpecs': [{
                        'zoneId': 'man',
                    }, {
                        'zoneId': 'sas',
                    }, {
                        'zoneId': 'vla',
                    }],
                },
                'standard 11-1c': {
                    'description':
                        'Standart PostgreSQL 11-1c cluster',
                    'labels': {
                        'acid': 'surely',
                    },
                    'environment':
                        'PRESTABLE',
                    'configSpec': {
                        'version': '11-1c',
                        'postgresqlConfig_11_1c': {
                            'logMinDurationStatement': 1000,
                            'sharedBuffers': 134217728,
                        },
                        'poolerConfig': {
                            'poolingMode': 'TRANSACTION',
                        },
                        'resources': {
                            'resourcePresetId': 'db1.nano',
                            'diskTypeId': 'local-ssd',
                            'diskSize': 10737418240,
                        },
                    },
                    'databaseSpecs': [{
                        'name': 'testdb',
                        'owner': 'test_user',
                        'extensions': [{
                            'name': 'btree_gist',
                        }],
                        'lcCollate': 'fi_FI.UTF-8',
                        'lcCtype': 'fi_FI.UTF-8',
                    }],
                    'userSpecs': [{
                        'name': 'test_user',
                        'connLimit': 10,
                        'password': 'mysupercooltestpassword11111',
                    }, {
                        'name': 'another_test_user',
                        'connLimit': 10,
                        'password': 'mysupercooltestpassword11111',
                    }],
                    'hostSpecs': [{
                        'zoneId': 'man',
                    }, {
                        'zoneId': 'sas',
                    }, {
                        'zoneId': 'vla',
                    }],
                },
                'standard 12': {
                    'description':
                        'Standart PostgreSQL 12 cluster',
                    'labels': {
                        'acid': 'surely',
                    },
                    'environment':
                        'PRESTABLE',
                    'configSpec': {
                        'version': '12',
                        'postgresqlConfig_12': {
                            'logMinDurationStatement': 1000,
                            'sharedBuffers': 134217728,
                        },
                        'poolerConfig': {
                            'poolingMode': 'TRANSACTION',
                        },
                        'resources': {
                            'resourcePresetId': 'db1.nano',
                            'diskTypeId': 'local-ssd',
                            'diskSize': 10737418240,
                        },
                    },
                    'databaseSpecs': [{
                        'name': 'testdb',
                        'owner': 'test_user',
                        'extensions': [{
                            'name': 'btree_gist',
                        }],
                        'lcCollate': 'fi_FI.UTF-8',
                        'lcCtype': 'fi_FI.UTF-8',
                    }],
                    'userSpecs': [{
                        'name': 'test_user',
                        'connLimit': 10,
                        'password': 'mysupercooltestpassword11111',
                    }, {
                        'name': 'another_test_user',
                        'connLimit': 10,
                        'password': 'mysupercooltestpassword11111',
                    }],
                    'hostSpecs': [{
                        'zoneId': 'man',
                    }, {
                        'zoneId': 'sas',
                    }, {
                        'zoneId': 'vla',
                    }],
                },
                'standard 12-1c': {
                    'description':
                        'Standart PostgreSQL 12-1c cluster',
                    'labels': {
                        'acid': 'surely',
                    },
                    'environment':
                        'PRESTABLE',
                    'configSpec': {
                        'version': '12-1c',
                        'postgresqlConfig_12_1c': {
                            'logMinDurationStatement': 1000,
                            'sharedBuffers': 134217728,
                        },
                        'poolerConfig': {
                            'poolingMode': 'TRANSACTION',
                        },
                        'resources': {
                            'resourcePresetId': 'db1.nano',
                            'diskTypeId': 'local-ssd',
                            'diskSize': 10737418240,
                        },
                    },
                    'databaseSpecs': [{
                        'name': 'testdb',
                        'owner': 'test_user',
                        'extensions': [{
                            'name': 'btree_gist',
                        }],
                        'lcCollate': 'fi_FI.UTF-8',
                        'lcCtype': 'fi_FI.UTF-8',
                    }],
                    'userSpecs': [{
                        'name': 'test_user',
                        'connLimit': 10,
                        'password': 'mysupercooltestpassword11111',
                    }, {
                        'name': 'another_test_user',
                        'connLimit': 10,
                        'password': 'mysupercooltestpassword11111',
                    }],
                    'hostSpecs': [{
                        'zoneId': 'man',
                    }, {
                        'zoneId': 'sas',
                    }, {
                        'zoneId': 'vla',
                    }],
                },
                'standard 13': {
                    'description':
                        'Standart PostgreSQL 13 cluster',
                    'labels': {
                        'acid': 'surely',
                    },
                    'environment':
                        'PRESTABLE',
                    'configSpec': {
                        'version': '13',
                        'postgresqlConfig_13': {
                            'logMinDurationStatement': 1000,
                            'sharedBuffers': 134217728,
                            'archiveTimeout': 60000,
                        },
                        'poolerConfig': {
                            'poolingMode': 'TRANSACTION',
                        },
                        'resources': {
                            'resourcePresetId': 'db1.nano',
                            'diskTypeId': 'local-ssd',
                            'diskSize': 10737418240,
                        },
                    },
                    'databaseSpecs': [{
                        'name': 'testdb',
                        'owner': 'test_user',
                        'extensions': [{
                            'name': 'btree_gist',
                        }],
                        'lcCollate': 'fi_FI.UTF-8',
                        'lcCtype': 'fi_FI.UTF-8',
                    }],
                    'userSpecs': [{
                        'name': 'test_user',
                        'connLimit': 10,
                        'password': 'mysupercooltestpassword11111',
                    }, {
                        'name': 'another_test_user',
                        'connLimit': 10,
                        'password': 'mysupercooltestpassword11111',
                    }],
                    'hostSpecs': [{
                        'zoneId': 'man',
                    }, {
                        'zoneId': 'sas',
                    }, {
                        'zoneId': 'vla',
                    }],
                },
                'standard 13-1c': {
                    'description':
                        'Standart PostgreSQL 13-1c cluster',
                    'labels': {
                        'acid': 'surely',
                    },
                    'environment':
                        'PRESTABLE',
                    'configSpec': {
                        'version': '13-1c',
                        'postgresqlConfig_13_1c': {
                            'logMinDurationStatement': 1000,
                            'sharedBuffers': 134217728,
                            'archiveTimeout': 60000,
                        },
                        'poolerConfig': {
                            'poolingMode': 'TRANSACTION',
                        },
                        'resources': {
                            'resourcePresetId': 'db1.nano',
                            'diskTypeId': 'local-ssd',
                            'diskSize': 10737418240,
                        },
                    },
                    'databaseSpecs': [{
                        'name': 'testdb',
                        'owner': 'test_user',
                        'extensions': [{
                            'name': 'btree_gist',
                        }],
                        'lcCollate': 'fi_FI.UTF-8',
                        'lcCtype': 'fi_FI.UTF-8',
                    }],
                    'userSpecs': [{
                        'name': 'test_user',
                        'connLimit': 10,
                        'password': 'mysupercooltestpassword11111',
                    }, {
                        'name': 'another_test_user',
                        'connLimit': 10,
                        'password': 'mysupercooltestpassword11111',
                    }],
                    'hostSpecs': [{
                        'zoneId': 'man',
                    }, {
                        'zoneId': 'sas',
                    }, {
                        'zoneId': 'vla',
                    }],
                },
                'standard 14': {
                    'description':
                        'Standart PostgreSQL 14 cluster',
                    'labels': {
                        'acid': 'surely',
                    },
                    'environment':
                        'PRESTABLE',
                    'configSpec': {
                        'version': '14',
                        'postgresqlConfig_14': {
                            'logMinDurationStatement': 1000,
                            'sharedBuffers': 134217728,
                            'archiveTimeout': 60000,
                        },
                        'poolerConfig': {
                            'poolingMode': 'TRANSACTION',
                        },
                        'resources': {
                            'resourcePresetId': 'db1.nano',
                            'diskTypeId': 'local-ssd',
                            'diskSize': 10737418240,
                        },
                    },
                    'databaseSpecs': [{
                        'name': 'testdb',
                        'owner': 'test_user',
                        'extensions': [{
                            'name': 'btree_gist',
                        }],
                        'lcCollate': 'fi_FI.UTF-8',
                        'lcCtype': 'fi_FI.UTF-8',
                    }],
                    'userSpecs': [{
                        'name': 'test_user',
                        'connLimit': 10,
                        'password': 'mysupercooltestpassword11111',
                    }, {
                        'name': 'another_test_user',
                        'connLimit': 10,
                        'password': 'mysupercooltestpassword11111',
                    }],
                    'hostSpecs': [{
                        'zoneId': 'man',
                    }, {
                        'zoneId': 'sas',
                    }, {
                        'zoneId': 'vla',
                    }],
                },
                'standard 14-1c': {
                    'description':
                        'Standart PostgreSQL 14-1c cluster',
                    'labels': {
                        'acid': 'surely',
                    },
                    'environment':
                        'PRESTABLE',
                    'configSpec': {
                        'version': '14-1c',
                        'postgresqlConfig_14_1c': {
                            'logMinDurationStatement': 1000,
                            'sharedBuffers': 134217728,
                            'archiveTimeout': 60000,
                        },
                        'poolerConfig': {
                            'poolingMode': 'TRANSACTION',
                        },
                        'resources': {
                            'resourcePresetId': 'db1.nano',
                            'diskTypeId': 'local-ssd',
                            'diskSize': 10737418240,
                        },
                    },
                    'databaseSpecs': [{
                        'name': 'testdb',
                        'owner': 'test_user',
                        'extensions': [{
                            'name': 'btree_gist',
                        }],
                        'lcCollate': 'fi_FI.UTF-8',
                        'lcCtype': 'fi_FI.UTF-8',
                    }],
                    'userSpecs': [{
                        'name': 'test_user',
                        'connLimit': 10,
                        'password': 'mysupercooltestpassword11111',
                    }, {
                        'name': 'another_test_user',
                        'connLimit': 10,
                        'password': 'mysupercooltestpassword11111',
                    }],
                    'hostSpecs': [{
                        'zoneId': 'man',
                    }, {
                        'zoneId': 'sas',
                    }, {
                        'zoneId': 'vla',
                    }],
                },
                'single': {
                    'environment':
                        'PRESTABLE',
                    'configSpec': {
                        'version': '14',
                        'postgresqlConfig_14': {
                            'logMinDurationStatement': 1000,
                            'sharedBuffers': 134217728,
                        },
                        'poolerConfig': {
                            'poolingMode': 'TRANSACTION',
                        },
                        'resources': {
                            'resourcePresetId': 'db1.nano',
                            'diskTypeId': 'local-ssd',
                            'diskSize': 10737418240,
                        },
                    },
                    'databaseSpecs': [{
                        'name': 'testdb',
                        'owner': 'test_user',
                        'extensions': [{
                            'name': 'btree_gist',
                        }],
                        'lcCollate': 'zh_HK.UTF-8',
                        'lcCtype': 'zh_HK.UTF-8',
                    }],
                    'userSpecs': [{
                        'name': 'test_user',
                        'connLimit': 10,
                        'password': 'mysupercooltestpassword11111',
                    }, {
                        'name': 'another_test_user',
                        'connLimit': 10,
                        'password': 'mysupercooltestpassword11111',
                    }],
                    'hostSpecs': [{
                        'zoneId': 'man',
                    }],
                },
            },
            'mongodb': {
                'standard_replicaset': {
                    'environment':
                        'PRESTABLE',
                    'configSpec': {
                        'mongodbSpec_3_6': {
                            'mongod': {
                                'resources': {
                                    'resourcePresetId': 'db1.nano',
                                    'diskTypeId': 'local-ssd',
                                    'diskSize': 10737418240,
                                },
                                'config': {
                                    'net': {
                                        'maxIncomingConnections': 128,
                                    },
                                },
                            },
                        },
                    },
                    'databaseSpecs': [
                        {
                            'name': 'testdb1',
                        },
                        {
                            'name': 'testdb2',
                        },
                        {
                            'name': 'testdb3',
                        },
                    ],
                    'userSpecs': [{
                        'name': 'test_user',
                        'password': 'mysupercooltestpassword11111',
                        'permissions': [{
                            'databaseName': 'testdb1',
                            'roles': ['read'],
                        }],
                    },
                                  {
                                      'name':
                                          'another_test_user',
                                      'password':
                                          'mysupercooltestpassword11111',
                                      'permissions': [{
                                          'databaseName': 'testdb1',
                                          'roles': ['readWrite'],
                                      }, {
                                          'databaseName': 'testdb2',
                                          'roles': ['readWrite'],
                                      }],
                                  }, {
                                      'name': 'and_yet_another_test_user',
                                      'password': 'mysupercooltestpassword11111',
                                  }],
                    'hostSpecs': [{
                        'zoneId': 'man',
                    }, {
                        'zoneId': 'sas',
                    }, {
                        'zoneId': 'vla',
                    }],
                },
                'standard_replicaset_4_0': {
                    'environment':
                        'PRESTABLE',
                    'configSpec': {
                        'mongodbSpec_4_0': {
                            'mongod': {
                                'resources': {
                                    'resourcePresetId': 'db1.nano',
                                    'diskTypeId': 'local-ssd',
                                    'diskSize': 10737418240,
                                },
                                'config': {
                                    'net': {
                                        'maxIncomingConnections': 128,
                                    },
                                },
                            },
                        },
                    },
                    'databaseSpecs': [
                        {
                            'name': 'testdb1',
                        },
                        {
                            'name': 'testdb2',
                        },
                        {
                            'name': 'testdb3',
                        },
                        {
                            'name': 'testsh_db',
                        },
                    ],
                    'userSpecs': [
                        {
                            'name': 'test_user',
                            'password': 'mysupercooltestpassword11111',
                            'permissions': [{
                                'databaseName': 'testdb1',
                                'roles': ['read'],
                            }],
                        },
                        {
                            'name':
                                'another_test_user',
                            'password':
                                'mysupercooltestpassword11111',
                            'permissions': [{
                                'databaseName': 'testdb1',
                                'roles': ['readWrite'],
                            }, {
                                'databaseName': 'testdb2',
                                'roles': ['read'],
                            }],
                        },
                        {
                            'name': 'user_testdb2',
                            'password': 'mysupercooltestpassword11111',
                            'permissions': [{
                                'databaseName': 'testdb2',
                                'roles': ['readWrite'],
                            }],
                        },
                        {
                            'name': 'and_yet_another_test_user',
                            'password': 'mysupercooltestpassword11111',
                        },
                        {
                            'name': 'user_testsh_db',
                            'password': 'mysupercooltestpassword11111',
                            'permissions': [{
                                'databaseName': 'testsh_db',
                                'roles': ['readWrite'],
                            }],
                        },
                        {
                            'name':
                                'sh_admin',
                            'password':
                                'mysupercooltestpassword11111',
                            'permissions': [{
                                'databaseName': 'admin',
                                'roles': ['mdbShardingManager', 'mdbMonitor'],
                            }],
                        },
                    ],
                    'hostSpecs': [{
                        'zoneId': 'man',
                    }, {
                        'zoneId': 'sas',
                    }, {
                        'zoneId': 'vla',
                    }],
                },
                'standard_replicaset_4_2': {
                    'environment':
                        'PRESTABLE',
                    'configSpec': {
                        'mongodbSpec_4_2': {
                            'mongod': {
                                'resources': {
                                    'resourcePresetId': 'db1.nano',
                                    'diskTypeId': 'local-ssd',
                                    'diskSize': 10737418240,
                                },
                                'config': {
                                    'net': {
                                        'maxIncomingConnections': 128,
                                    },
                                },
                            },
                        },
                    },
                    'databaseSpecs': [
                        {
                            'name': 'testdb1',
                        },
                        {
                            'name': 'testdb2',
                        },
                        {
                            'name': 'testdb3',
                        },
                        {
                            'name': 'testsh_db',
                        },
                    ],
                    'userSpecs': [
                        {
                            'name': 'test_user',
                            'password': 'mysupercooltestpassword11111',
                            'permissions': [{
                                'databaseName': 'testdb1',
                                'roles': ['read'],
                            }],
                        },
                        {
                            'name':
                                'another_test_user',
                            'password':
                                'mysupercooltestpassword11111',
                            'permissions': [{
                                'databaseName': 'testdb1',
                                'roles': ['readWrite'],
                            }, {
                                'databaseName': 'testdb2',
                                'roles': ['read'],
                            }],
                        },
                        {
                            'name': 'user_testdb2',
                            'password': 'mysupercooltestpassword11111',
                            'permissions': [{
                                'databaseName': 'testdb2',
                                'roles': ['readWrite'],
                            }],
                        },
                        {
                            'name': 'and_yet_another_test_user',
                            'password': 'mysupercooltestpassword11111',
                        },
                        {
                            'name': 'user_testsh_db',
                            'password': 'mysupercooltestpassword11111',
                            'permissions': [{
                                'databaseName': 'testsh_db',
                                'roles': ['readWrite'],
                            }],
                        },
                        {
                            'name':
                                'sh_admin',
                            'password':
                                'mysupercooltestpassword11111',
                            'permissions': [{
                                'databaseName': 'admin',
                                'roles': ['mdbShardingManager', 'mdbMonitor'],
                            }],
                        },
                    ],
                    'hostSpecs': [{
                        'zoneId': 'man',
                    }, {
                        'zoneId': 'sas',
                    }, {
                        'zoneId': 'vla',
                    }],
                },
                'standard_replicaset_4_4': {
                    'environment':
                        'PRESTABLE',
                    'configSpec': {
                        'mongodbSpec_4_4': {
                            'mongod': {
                                'resources': {
                                    'resourcePresetId': 'db1.nano',
                                    'diskTypeId': 'local-ssd',
                                    'diskSize': 10737418240,
                                },
                                'config': {
                                    'net': {
                                        'maxIncomingConnections': 128,
                                    },
                                },
                            },
                        },
                    },
                    'databaseSpecs': [
                        {
                            'name': 'testdb1',
                        },
                        {
                            'name': 'testdb2',
                        },
                        {
                            'name': 'testdb3',
                        },
                        {
                            'name': 'testsh_db',
                        },
                    ],
                    'userSpecs': [
                        {
                            'name': 'test_user',
                            'password': 'mysupercooltestpassword11111',
                            'permissions': [{
                                'databaseName': 'testdb1',
                                'roles': ['read'],
                            }],
                        },
                        {
                            'name':
                                'another_test_user',
                            'password':
                                'mysupercooltestpassword11111',
                            'permissions': [{
                                'databaseName': 'testdb1',
                                'roles': ['readWrite'],
                            }, {
                                'databaseName': 'testdb2',
                                'roles': ['read'],
                            }],
                        },
                        {
                            'name': 'user_testdb2',
                            'password': 'mysupercooltestpassword11111',
                            'permissions': [{
                                'databaseName': 'testdb2',
                                'roles': ['readWrite'],
                            }],
                        },
                        {
                            'name': 'and_yet_another_test_user',
                            'password': 'mysupercooltestpassword11111',
                        },
                        {
                            'name': 'user_testsh_db',
                            'password': 'mysupercooltestpassword11111',
                            'permissions': [{
                                'databaseName': 'testsh_db',
                                'roles': ['readWrite'],
                            }],
                        },
                        {
                            'name':
                                'sh_admin',
                            'password':
                                'mysupercooltestpassword11111',
                            'permissions': [{
                                'databaseName': 'admin',
                                'roles': ['mdbShardingManager', 'mdbMonitor'],
                            }],
                        },
                    ],
                    'hostSpecs': [{
                        'zoneId': 'man',
                    }, {
                        'zoneId': 'sas',
                    }, {
                        'zoneId': 'vla',
                    }],
                },
                'standard_replicaset_5_0': {
                    'environment':
                        'PRESTABLE',
                    'configSpec': {
                        'mongodbSpec_5_0': {
                            'mongod': {
                                'resources': {
                                    'resourcePresetId': 'db1.nano',
                                    'diskTypeId': 'local-ssd',
                                    'diskSize': 10737418240,
                                },
                                'config': {
                                    'net': {
                                        'maxIncomingConnections': 128,
                                    },
                                },
                            },
                        },
                    },
                    'databaseSpecs': [
                        {
                            'name': 'testdb1',
                        },
                        {
                            'name': 'testdb2',
                        },
                        {
                            'name': 'testdb3',
                        },
                        {
                            'name': 'testsh_db',
                        },
                    ],
                    'userSpecs': [
                        {
                            'name': 'test_user',
                            'password': 'mysupercooltestpassword11111',
                            'permissions': [{
                                'databaseName': 'testdb1',
                                'roles': ['read'],
                            }],
                        },
                        {
                            'name':
                                'another_test_user',
                            'password':
                                'mysupercooltestpassword11111',
                            'permissions': [{
                                'databaseName': 'testdb1',
                                'roles': ['readWrite'],
                            }, {
                                'databaseName': 'testdb2',
                                'roles': ['read'],
                            }],
                        },
                        {
                            'name': 'user_testdb2',
                            'password': 'mysupercooltestpassword11111',
                            'permissions': [{
                                'databaseName': 'testdb2',
                                'roles': ['readWrite'],
                            }],
                        },
                        {
                            'name': 'and_yet_another_test_user',
                            'password': 'mysupercooltestpassword11111',
                        },
                        {
                            'name': 'user_testsh_db',
                            'password': 'mysupercooltestpassword11111',
                            'permissions': [{
                                'databaseName': 'testsh_db',
                                'roles': ['readWrite'],
                            }],
                        },
                        {
                            'name':
                                'sh_admin',
                            'password':
                                'mysupercooltestpassword11111',
                            'permissions': [{
                                'databaseName': 'admin',
                                'roles': ['mdbShardingManager', 'mdbMonitor'],
                            }],
                        },
                    ],
                    'hostSpecs': [{
                        'zoneId': 'man',
                    }, {
                        'zoneId': 'sas',
                    }, {
                        'zoneId': 'vla',
                    }],
                },
                'standard_replicaset_4_4_enterprise': {
                    'environment':
                        'PRESTABLE',
                    'configSpec': {
                        'mongodbSpec_4_4_enterprise': {
                            'mongod': {
                                'resources': {
                                    'resourcePresetId': 'db1.nano',
                                    'diskTypeId': 'local-ssd',
                                    'diskSize': 10737418240,
                                },
                                'config': {
                                    'net': {
                                        'maxIncomingConnections': 128,
                                    },
                                    'security': {
                                        'enableEncryption': True,
                                        'kmip': {
                                            'serverName':
                                                'pykmip01',
                                            'port':
                                                5696,
                                            'serverCa':
                                                '-----BEGIN CERTIFICATE-----\n' +
                                                'MIIDSzCCAjOgAwIBAgIURg6Ropn/Y/T1Czzd5gAFwIe90FMwDQYJKoZIhvcNAQEL\n' +
                                                'BQAwFjEUMBIGA1UEAwwLRWFzeS1SU0EgQ0EwHhcNMjIwNDA4MDcxMTAzWhcNMzIw\n' +
                                                'NDA1MDcxMTAzWjAWMRQwEgYDVQQDDAtFYXN5LVJTQSBDQTCCASIwDQYJKoZIhvcN\n' +
                                                'AQEBBQADggEPADCCAQoCggEBALLkItP9LIP55cMJ3oGBaK5Htg/xCZNuV8XV+YZ/\n' +
                                                'DrACn/yZLHGlhe+rRzrJ6k1jaap25LmXe1DE5ZfF/7TNbRo4W94i83iU75R9vLg7\n' +
                                                'Fkhu1gYiv4f75G/gvB8+Vab/eIQFH3dgw4vtIPA2dGjqTIsNeXWNJ7VTYvuNXIvk\n' +
                                                'iTz32K9enkVXOunBcjF1hpoCoSRHBIsh+wPCR0aMKnhtNxW0hsor2Yt/Yh6VMWaF\n' +
                                                'b05WWd//qYaxzsO467xQVoz49IvCFR7m2PyKEGc90knWIz+PjkLaTQSNn6muMW4J\n' +
                                                'Coi/3Cl2la1oDH2IN6jMQnH2reug6TK+1HiiQ8JNwp5RUysCAwEAAaOBkDCBjTAd\n' +
                                                'BgNVHQ4EFgQUT2IRMCgNJJRB8ke3h0vqxt7IqeAwUQYDVR0jBEowSIAUT2IRMCgN\n' +
                                                'JJRB8ke3h0vqxt7IqeChGqQYMBYxFDASBgNVBAMMC0Vhc3ktUlNBIENBghRGDpGi\n' +
                                                'mf9j9PULPN3mAAXAh73QUzAMBgNVHRMEBTADAQH/MAsGA1UdDwQEAwIBBjANBgkq\n' +
                                                'hkiG9w0BAQsFAAOCAQEArsgjFLZ83lr1Z8lWbpdOgO/JHEISg0negQ+oUqHOPuW9\n' +
                                                'Us6PaKyCHbkJCwC1gPSNM0g4D2ld2IEtAoLzZy+0XDxlOTPqdct98j/co4qLwXWh\n' +
                                                'eWYvf2ajW9I8SfxQwGKVbizZ69tLFB2sDIQf+wDStkWt06CzrBav+eg6gLOG5n6N\n' +
                                                'KbaahjhFJEw+fp0d3LRFGKoZ9+szPZmqdsbo//e/zE2UJJHw/v+9MDQ/gdlBSRRw\n' +
                                                'Up6ZPtmPLzKVmOqV61HpoTKb/WWcF+X/5KtZ70ZKOmsjpVrW+VQmwL2mUHICQeiY\n' +
                                                'xfaKXwHbZLKaX7z++2y/IYx9ymIlVCkMPc8omUMMKA==\n' +
                                                '-----END CERTIFICATE-----',
                                            'clientCertificate':
                                                '-----BEGIN CERTIFICATE-----\n' +
                                                'MIIDVjCCAj6gAwIBAgIRAMN1GJc/scFyM0yzh7edF2YwDQYJKoZIhvcNAQELBQAw\n' +
                                                'FjEUMBIGA1UEAwwLRWFzeS1SU0EgQ0EwHhcNMjIwNDA4MDcxMjA2WhcNMjQwNzEx\n' +
                                                'MDcxMjA2WjASMRAwDgYDVQQDDAdtb25nb2RiMIIBIjANBgkqhkiG9w0BAQEFAAOC\n' +
                                                'AQ8AMIIBCgKCAQEAvDUYFgyWe/4BApMTT4zbv2x+nHe/X10sP+psqGrQworzBTLI\n' +
                                                'zCAelRdyEd8mfQej5d1t6UvymstetZn4ZWnUfeo5ZnAc1Re7mdfDFR/vPedwL/MR\n' +
                                                'evJ01aC8p1kjIpQb+7rZzDnrSDYLQ+ZJUlLuvKX/WPeTouLC6ZgIxEyriN/5VKQo\n' +
                                                '92zPoYMuMOae340UMIsnx4FGtxp9GaVCWpR/Fh9lihc5OjduyVFj5iFUNiAbgZfl\n' +
                                                'c+pmqKEXjwtAKnHWKVS8/FV7eeejXkD2WARjt3Gfw8idI5ss26e8N1E//z0XZ3fO\n' +
                                                'sTCFxJoimyz5zhCtwTMjUTujxPFjhvrK57bqJQIDAQABo4GiMIGfMAkGA1UdEwQC\n' +
                                                'MAAwHQYDVR0OBBYEFPFCBg7dUPg42yg8QzSfSn6LP6JbMFEGA1UdIwRKMEiAFE9i\n' +
                                                'ETAoDSSUQfJHt4dL6sbeyKngoRqkGDAWMRQwEgYDVQQDDAtFYXN5LVJTQSBDQYIU\n' +
                                                'Rg6Ropn/Y/T1Czzd5gAFwIe90FMwEwYDVR0lBAwwCgYIKwYBBQUHAwIwCwYDVR0P\n' +
                                                'BAQDAgeAMA0GCSqGSIb3DQEBCwUAA4IBAQA4Px7I6oEMQaoe69yPS6CAZtRqYtrZ\n' +
                                                'zyy/JzfrJNlLABYmeWPAhEHtbr6RpNsV0aXrQkw+iBaEH2OIxnsQq5NrOna3GCtv\n' +
                                                'rpCqNohMaTMMss9GdBvDeXuNHAVB/fKi2KYHDNaI6udod57npcsmfbyVa4oYQPtl\n' +
                                                'nh4mcvJM2I3dvEJA5xNVHpJdHwdkKI/YfAPpnbXOdw32Cf9upP7aPcZtkPaw1NYy\n' +
                                                'wvaRRNtOMkcnjFXaDHbXzrNqtwAXDsNFiQ2/9jWz7VUlOlUlJfYhEZF2fQ+2MefV\n' +
                                                'qVkXfKa6xgveN/+skMWksdQ6scFmcTuXMsH03N7dpx4WEf0Zr49N9Qr3\n' +
                                                '-----END CERTIFICATE-----\n' + '-----BEGIN PRIVATE KEY-----\n' +
                                                'MIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQC8NRgWDJZ7/gEC\n' +
                                                'kxNPjNu/bH6cd79fXSw/6myoatDCivMFMsjMIB6VF3IR3yZ9B6Pl3W3pS/Kay161\n' +
                                                'mfhladR96jlmcBzVF7uZ18MVH+8953Av8xF68nTVoLynWSMilBv7utnMOetINgtD\n' +
                                                '5klSUu68pf9Y95Oi4sLpmAjETKuI3/lUpCj3bM+hgy4w5p7fjRQwiyfHgUa3Gn0Z\n' +
                                                'pUJalH8WH2WKFzk6N27JUWPmIVQ2IBuBl+Vz6maooRePC0AqcdYpVLz8VXt556Ne\n' +
                                                'QPZYBGO3cZ/DyJ0jmyzbp7w3UT//PRdnd86xMIXEmiKbLPnOEK3BMyNRO6PE8WOG\n' +
                                                '+srntuolAgMBAAECggEBALvc34r6y6c+trFkL41jU+HyoTr7yLmfHlE6ZXWDEZhu\n' +
                                                '6/9PXuFqWjyF56XxMdDxtGb5LQIHfkWHJKVu6GQKTEHXb65R0GXgR7Fbjm3ir4MC\n' +
                                                'JpimLtejdn3a8RS2Q/z7DCesrkRNuA4fbAU9tAiJoaYKqCSdE/AuG1LiIDYZNcvr\n' +
                                                '1T/xkv70IPPnr6W89/lw37n+y4/LZAt3dPGORxm3+zGEND3o2xvjyEwKNCA8LIFs\n' +
                                                'dvAmwLY6rX82KAZFMXYZ/1R/nUJ8DoZ/xtJrc/K368EE1yLdQljIk85nO6gzaRBr\n' +
                                                'ROdhAYg5PMLhVCUfdd47AAo3/1Yuk5FMUNX5OILb9sECgYEA8I9d1Z1/6Q36L6D9\n' +
                                                'HRC0CSo0SWfbEaWLcOhyinSc4wAHeJTWDXkIsoNyaxR0kIhTLB0wkebA4Hb8i/J+\n' +
                                                'kTXZN1GNBhqsDXIWAmbRQbH8Ef0ww5xJXiaMwYlVwdtEw5jcsmNpz4IjpUrfozdZ\n' +
                                                'sbFS7gUncBzeZ/kZa2CeTb7Iv3UCgYEAyEmF+f/c6Ty9AjAsYy8z/CGL/oL/r9LD\n' +
                                                'WxjdL3uPEJO+79V6TUsG65QJzpCYNmKF+QtGNEqmN24kA6Dpfl23Bot9TokO7TNg\n' +
                                                'kxfKeHjjdx+ya8mPLy+rBYcFb7grPW5Gu8O7L8do7kgHThFyYhQe35ylEPDB5NaM\n' +
                                                'D7sIiqcfWfECgYBzwJinRD0bOGWNa4q/5Jys2EkGlVm9WQoKz17mLoybUhVGOV/y\n' +
                                                'Za4Ar+1rhxE7xs02qekIG5/tonONJ6ctWlrmGnCgYk8tvRrIFw9T7D/drBY92cMX\n' +
                                                '8bbDHcFNIaQp9jEkCWANwJJEZA3ObMDVFv1PmN1MPifqodtQZtJlmIriTQKBgQCS\n' +
                                                'ddFXA1dT52p/kyKiVP46vX1V3A2FUSYyE3iLJFt1z9SsJPuOUfL5igOx3eKwwlMd\n' +
                                                'zrTDwGLT3eLQFHcqRPV1/8LeDzOvGQbiCV+xwRT1I2ShlX7zDnSNUjMTgyV7goyO\n' +
                                                '+Y6EXdnJhTpySCfQuM3qzu2V+biP0qQRTL/uRE+UwQKBgFJqV1o4yMDsdmsdpm7j\n' +
                                                'SChYKjvqtyn2XxdrkkVlFM4tCJkH12t9u61rY1LL/4mKkZMfYJSMGuVZ6faAxcfx\n' +
                                                '45R1rUnS6/Ow3n9gkrfx7FdnQXyG/tP3iRtmBJjnqdPKKk1xhuRub1e0EF/E5B0H\n' +
                                                'fJZSkl9t84//V9iVNJhs8Koh\n' + '-----END PRIVATE KEY-----',
                                        },
                                    },
                                },
                            },
                        },
                    },
                    'databaseSpecs': [
                        {
                            'name': 'testdb1',
                        },
                        {
                            'name': 'testdb2',
                        },
                        {
                            'name': 'testdb3',
                        },
                        {
                            'name': 'testsh_db',
                        },
                    ],
                    'userSpecs': [
                        {
                            'name': 'test_user',
                            'password': 'mysupercooltestpassword11111',
                            'permissions': [{
                                'databaseName': 'testdb1',
                                'roles': ['read'],
                            }],
                        },
                        {
                            'name':
                                'another_test_user',
                            'password':
                                'mysupercooltestpassword11111',
                            'permissions': [{
                                'databaseName': 'testdb1',
                                'roles': ['readWrite'],
                            }, {
                                'databaseName': 'testdb2',
                                'roles': ['read'],
                            }],
                        },
                        {
                            'name': 'user_testdb2',
                            'password': 'mysupercooltestpassword11111',
                            'permissions': [{
                                'databaseName': 'testdb2',
                                'roles': ['readWrite'],
                            }],
                        },
                        {
                            'name': 'and_yet_another_test_user',
                            'password': 'mysupercooltestpassword11111',
                        },
                        {
                            'name': 'user_testsh_db',
                            'password': 'mysupercooltestpassword11111',
                            'permissions': [{
                                'databaseName': 'testsh_db',
                                'roles': ['readWrite'],
                            }],
                        },
                        {
                            'name':
                                'sh_admin',
                            'password':
                                'mysupercooltestpassword11111',
                            'permissions': [{
                                'databaseName': 'admin',
                                'roles': ['mdbShardingManager', 'mdbMonitor'],
                            }],
                        },
                    ],
                    'hostSpecs': [{
                        'zoneId': 'man',
                    }, {
                        'zoneId': 'sas',
                    }, {
                        'zoneId': 'vla',
                    }],
                },
                'standard_replicaset_5_0_enterprise': {
                    'environment':
                        'PRESTABLE',
                    'configSpec': {
                        'mongodbSpec_5_0_enterprise': {
                            'mongod': {
                                'resources': {
                                    'resourcePresetId': 'db1.nano',
                                    'diskTypeId': 'local-ssd',
                                    'diskSize': 10737418240,
                                },
                                'config': {
                                    'net': {
                                        'maxIncomingConnections': 128,
                                    },
                                    'security': {
                                        'enableEncryption': True,
                                        'kmip': {
                                            'serverName':
                                                'pykmip01',
                                            'port':
                                                5696,
                                            'serverCa':
                                                '-----BEGIN CERTIFICATE-----\n' +
                                                'MIIDSzCCAjOgAwIBAgIURg6Ropn/Y/T1Czzd5gAFwIe90FMwDQYJKoZIhvcNAQEL\n' +
                                                'BQAwFjEUMBIGA1UEAwwLRWFzeS1SU0EgQ0EwHhcNMjIwNDA4MDcxMTAzWhcNMzIw\n' +
                                                'NDA1MDcxMTAzWjAWMRQwEgYDVQQDDAtFYXN5LVJTQSBDQTCCASIwDQYJKoZIhvcN\n' +
                                                'AQEBBQADggEPADCCAQoCggEBALLkItP9LIP55cMJ3oGBaK5Htg/xCZNuV8XV+YZ/\n' +
                                                'DrACn/yZLHGlhe+rRzrJ6k1jaap25LmXe1DE5ZfF/7TNbRo4W94i83iU75R9vLg7\n' +
                                                'Fkhu1gYiv4f75G/gvB8+Vab/eIQFH3dgw4vtIPA2dGjqTIsNeXWNJ7VTYvuNXIvk\n' +
                                                'iTz32K9enkVXOunBcjF1hpoCoSRHBIsh+wPCR0aMKnhtNxW0hsor2Yt/Yh6VMWaF\n' +
                                                'b05WWd//qYaxzsO467xQVoz49IvCFR7m2PyKEGc90knWIz+PjkLaTQSNn6muMW4J\n' +
                                                'Coi/3Cl2la1oDH2IN6jMQnH2reug6TK+1HiiQ8JNwp5RUysCAwEAAaOBkDCBjTAd\n' +
                                                'BgNVHQ4EFgQUT2IRMCgNJJRB8ke3h0vqxt7IqeAwUQYDVR0jBEowSIAUT2IRMCgN\n' +
                                                'JJRB8ke3h0vqxt7IqeChGqQYMBYxFDASBgNVBAMMC0Vhc3ktUlNBIENBghRGDpGi\n' +
                                                'mf9j9PULPN3mAAXAh73QUzAMBgNVHRMEBTADAQH/MAsGA1UdDwQEAwIBBjANBgkq\n' +
                                                'hkiG9w0BAQsFAAOCAQEArsgjFLZ83lr1Z8lWbpdOgO/JHEISg0negQ+oUqHOPuW9\n' +
                                                'Us6PaKyCHbkJCwC1gPSNM0g4D2ld2IEtAoLzZy+0XDxlOTPqdct98j/co4qLwXWh\n' +
                                                'eWYvf2ajW9I8SfxQwGKVbizZ69tLFB2sDIQf+wDStkWt06CzrBav+eg6gLOG5n6N\n' +
                                                'KbaahjhFJEw+fp0d3LRFGKoZ9+szPZmqdsbo//e/zE2UJJHw/v+9MDQ/gdlBSRRw\n' +
                                                'Up6ZPtmPLzKVmOqV61HpoTKb/WWcF+X/5KtZ70ZKOmsjpVrW+VQmwL2mUHICQeiY\n' +
                                                'xfaKXwHbZLKaX7z++2y/IYx9ymIlVCkMPc8omUMMKA==\n' +
                                                '-----END CERTIFICATE-----',
                                            'clientCertificate':
                                                '-----BEGIN CERTIFICATE-----\n' +
                                                'MIIDVjCCAj6gAwIBAgIRAMN1GJc/scFyM0yzh7edF2YwDQYJKoZIhvcNAQELBQAw\n' +
                                                'FjEUMBIGA1UEAwwLRWFzeS1SU0EgQ0EwHhcNMjIwNDA4MDcxMjA2WhcNMjQwNzEx\n' +
                                                'MDcxMjA2WjASMRAwDgYDVQQDDAdtb25nb2RiMIIBIjANBgkqhkiG9w0BAQEFAAOC\n' +
                                                'AQ8AMIIBCgKCAQEAvDUYFgyWe/4BApMTT4zbv2x+nHe/X10sP+psqGrQworzBTLI\n' +
                                                'zCAelRdyEd8mfQej5d1t6UvymstetZn4ZWnUfeo5ZnAc1Re7mdfDFR/vPedwL/MR\n' +
                                                'evJ01aC8p1kjIpQb+7rZzDnrSDYLQ+ZJUlLuvKX/WPeTouLC6ZgIxEyriN/5VKQo\n' +
                                                '92zPoYMuMOae340UMIsnx4FGtxp9GaVCWpR/Fh9lihc5OjduyVFj5iFUNiAbgZfl\n' +
                                                'c+pmqKEXjwtAKnHWKVS8/FV7eeejXkD2WARjt3Gfw8idI5ss26e8N1E//z0XZ3fO\n' +
                                                'sTCFxJoimyz5zhCtwTMjUTujxPFjhvrK57bqJQIDAQABo4GiMIGfMAkGA1UdEwQC\n' +
                                                'MAAwHQYDVR0OBBYEFPFCBg7dUPg42yg8QzSfSn6LP6JbMFEGA1UdIwRKMEiAFE9i\n' +
                                                'ETAoDSSUQfJHt4dL6sbeyKngoRqkGDAWMRQwEgYDVQQDDAtFYXN5LVJTQSBDQYIU\n' +
                                                'Rg6Ropn/Y/T1Czzd5gAFwIe90FMwEwYDVR0lBAwwCgYIKwYBBQUHAwIwCwYDVR0P\n' +
                                                'BAQDAgeAMA0GCSqGSIb3DQEBCwUAA4IBAQA4Px7I6oEMQaoe69yPS6CAZtRqYtrZ\n' +
                                                'zyy/JzfrJNlLABYmeWPAhEHtbr6RpNsV0aXrQkw+iBaEH2OIxnsQq5NrOna3GCtv\n' +
                                                'rpCqNohMaTMMss9GdBvDeXuNHAVB/fKi2KYHDNaI6udod57npcsmfbyVa4oYQPtl\n' +
                                                'nh4mcvJM2I3dvEJA5xNVHpJdHwdkKI/YfAPpnbXOdw32Cf9upP7aPcZtkPaw1NYy\n' +
                                                'wvaRRNtOMkcnjFXaDHbXzrNqtwAXDsNFiQ2/9jWz7VUlOlUlJfYhEZF2fQ+2MefV\n' +
                                                'qVkXfKa6xgveN/+skMWksdQ6scFmcTuXMsH03N7dpx4WEf0Zr49N9Qr3\n' +
                                                '-----END CERTIFICATE-----\n' + '-----BEGIN PRIVATE KEY-----\n' +
                                                'MIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQC8NRgWDJZ7/gEC\n' +
                                                'kxNPjNu/bH6cd79fXSw/6myoatDCivMFMsjMIB6VF3IR3yZ9B6Pl3W3pS/Kay161\n' +
                                                'mfhladR96jlmcBzVF7uZ18MVH+8953Av8xF68nTVoLynWSMilBv7utnMOetINgtD\n' +
                                                '5klSUu68pf9Y95Oi4sLpmAjETKuI3/lUpCj3bM+hgy4w5p7fjRQwiyfHgUa3Gn0Z\n' +
                                                'pUJalH8WH2WKFzk6N27JUWPmIVQ2IBuBl+Vz6maooRePC0AqcdYpVLz8VXt556Ne\n' +
                                                'QPZYBGO3cZ/DyJ0jmyzbp7w3UT//PRdnd86xMIXEmiKbLPnOEK3BMyNRO6PE8WOG\n' +
                                                '+srntuolAgMBAAECggEBALvc34r6y6c+trFkL41jU+HyoTr7yLmfHlE6ZXWDEZhu\n' +
                                                '6/9PXuFqWjyF56XxMdDxtGb5LQIHfkWHJKVu6GQKTEHXb65R0GXgR7Fbjm3ir4MC\n' +
                                                'JpimLtejdn3a8RS2Q/z7DCesrkRNuA4fbAU9tAiJoaYKqCSdE/AuG1LiIDYZNcvr\n' +
                                                '1T/xkv70IPPnr6W89/lw37n+y4/LZAt3dPGORxm3+zGEND3o2xvjyEwKNCA8LIFs\n' +
                                                'dvAmwLY6rX82KAZFMXYZ/1R/nUJ8DoZ/xtJrc/K368EE1yLdQljIk85nO6gzaRBr\n' +
                                                'ROdhAYg5PMLhVCUfdd47AAo3/1Yuk5FMUNX5OILb9sECgYEA8I9d1Z1/6Q36L6D9\n' +
                                                'HRC0CSo0SWfbEaWLcOhyinSc4wAHeJTWDXkIsoNyaxR0kIhTLB0wkebA4Hb8i/J+\n' +
                                                'kTXZN1GNBhqsDXIWAmbRQbH8Ef0ww5xJXiaMwYlVwdtEw5jcsmNpz4IjpUrfozdZ\n' +
                                                'sbFS7gUncBzeZ/kZa2CeTb7Iv3UCgYEAyEmF+f/c6Ty9AjAsYy8z/CGL/oL/r9LD\n' +
                                                'WxjdL3uPEJO+79V6TUsG65QJzpCYNmKF+QtGNEqmN24kA6Dpfl23Bot9TokO7TNg\n' +
                                                'kxfKeHjjdx+ya8mPLy+rBYcFb7grPW5Gu8O7L8do7kgHThFyYhQe35ylEPDB5NaM\n' +
                                                'D7sIiqcfWfECgYBzwJinRD0bOGWNa4q/5Jys2EkGlVm9WQoKz17mLoybUhVGOV/y\n' +
                                                'Za4Ar+1rhxE7xs02qekIG5/tonONJ6ctWlrmGnCgYk8tvRrIFw9T7D/drBY92cMX\n' +
                                                '8bbDHcFNIaQp9jEkCWANwJJEZA3ObMDVFv1PmN1MPifqodtQZtJlmIriTQKBgQCS\n' +
                                                'ddFXA1dT52p/kyKiVP46vX1V3A2FUSYyE3iLJFt1z9SsJPuOUfL5igOx3eKwwlMd\n' +
                                                'zrTDwGLT3eLQFHcqRPV1/8LeDzOvGQbiCV+xwRT1I2ShlX7zDnSNUjMTgyV7goyO\n' +
                                                '+Y6EXdnJhTpySCfQuM3qzu2V+biP0qQRTL/uRE+UwQKBgFJqV1o4yMDsdmsdpm7j\n' +
                                                'SChYKjvqtyn2XxdrkkVlFM4tCJkH12t9u61rY1LL/4mKkZMfYJSMGuVZ6faAxcfx\n' +
                                                '45R1rUnS6/Ow3n9gkrfx7FdnQXyG/tP3iRtmBJjnqdPKKk1xhuRub1e0EF/E5B0H\n' +
                                                'fJZSkl9t84//V9iVNJhs8Koh\n' + '-----END PRIVATE KEY-----',
                                        },
                                    },
                                },
                            },
                        },
                    },
                    'databaseSpecs': [
                        {
                            'name': 'testdb1',
                        },
                        {
                            'name': 'testdb2',
                        },
                        {
                            'name': 'testdb3',
                        },
                        {
                            'name': 'testsh_db',
                        },
                    ],
                    'userSpecs': [
                        {
                            'name': 'test_user',
                            'password': 'mysupercooltestpassword11111',
                            'permissions': [{
                                'databaseName': 'testdb1',
                                'roles': ['read'],
                            }],
                        },
                        {
                            'name':
                                'another_test_user',
                            'password':
                                'mysupercooltestpassword11111',
                            'permissions': [{
                                'databaseName': 'testdb1',
                                'roles': ['readWrite'],
                            }, {
                                'databaseName': 'testdb2',
                                'roles': ['read'],
                            }],
                        },
                        {
                            'name': 'user_testdb2',
                            'password': 'mysupercooltestpassword11111',
                            'permissions': [{
                                'databaseName': 'testdb2',
                                'roles': ['readWrite'],
                            }],
                        },
                        {
                            'name': 'and_yet_another_test_user',
                            'password': 'mysupercooltestpassword11111',
                        },
                        {
                            'name': 'user_testsh_db',
                            'password': 'mysupercooltestpassword11111',
                            'permissions': [{
                                'databaseName': 'testsh_db',
                                'roles': ['readWrite'],
                            }],
                        },
                        {
                            'name':
                                'sh_admin',
                            'password':
                                'mysupercooltestpassword11111',
                            'permissions': [{
                                'databaseName': 'admin',
                                'roles': ['mdbShardingManager', 'mdbMonitor'],
                            }],
                        },
                    ],
                    'hostSpecs': [{
                        'zoneId': 'man',
                    }, {
                        'zoneId': 'sas',
                    }, {
                        'zoneId': 'vla',
                    }],
                },
                'one_node_replicaset': {
                    'environment':
                        'PRESTABLE',
                    'configSpec': {
                        'mongodbSpec_3_6': {
                            'mongod': {
                                'resources': {
                                    'resourcePresetId': 'db1.nano',
                                    'diskTypeId': 'local-ssd',
                                    'diskSize': 10737418240,
                                },
                                'config': {
                                    'net': {
                                        'maxIncomingConnections': 128,
                                    },
                                },
                            },
                        },
                    },
                    'databaseSpecs': [
                        {
                            'name': 'testdb1',
                        },
                        {
                            'name': 'testdb2',
                        },
                        {
                            'name': 'testdb3',
                        },
                    ],
                    'userSpecs': [{
                        'name': 'test_user',
                        'password': 'mysupercooltestpassword11111',
                        'permissions': [{
                            'databaseName': 'testdb1',
                            'roles': ['read'],
                        }],
                    },
                                  {
                                      'name':
                                          'another_test_user',
                                      'password':
                                          'mysupercooltestpassword11111',
                                      'permissions': [{
                                          'databaseName': 'testdb1',
                                          'roles': ['readWrite'],
                                      }, {
                                          'databaseName': 'testdb2',
                                          'roles': ['read'],
                                      }],
                                  }, {
                                      'name': 'and_yet_another_test_user',
                                      'password': 'mysupercooltestpassword11111',
                                  }],
                    'hostSpecs': [{
                        'zoneId': 'man',
                    }],
                },
                'one_node_replicaset_4_0': {
                    'environment':
                        'PRESTABLE',
                    'configSpec': {
                        'mongodbSpec_4_0': {
                            'mongod': {
                                'resources': {
                                    'resourcePresetId': 'db1.nano',
                                    'diskTypeId': 'local-ssd',
                                    'diskSize': 10737418240,
                                },
                                'config': {
                                    'net': {
                                        'maxIncomingConnections': 128,
                                    },
                                },
                            },
                        },
                    },
                    'databaseSpecs': [
                        {
                            'name': 'testdb1',
                        },
                        {
                            'name': 'testdb2',
                        },
                        {
                            'name': 'testdb3',
                        },
                    ],
                    'userSpecs': [{
                        'name': 'test_user',
                        'password': 'mysupercooltestpassword11111',
                        'permissions': [{
                            'databaseName': 'testdb1',
                            'roles': ['read'],
                        }],
                    },
                                  {
                                      'name':
                                          'another_test_user',
                                      'password':
                                          'mysupercooltestpassword11111',
                                      'permissions': [{
                                          'databaseName': 'testdb1',
                                          'roles': ['readWrite'],
                                      }, {
                                          'databaseName': 'testdb2',
                                          'roles': ['read'],
                                      }],
                                  }, {
                                      'name': 'and_yet_another_test_user',
                                      'password': 'mysupercooltestpassword11111',
                                  }],
                    'hostSpecs': [{
                        'zoneId': 'man',
                    }],
                },
                'one_node_replicaset_4_2': {
                    'environment':
                        'PRESTABLE',
                    'configSpec': {
                        'mongodbSpec_4_2': {
                            'mongod': {
                                'resources': {
                                    'resourcePresetId': 'db1.nano',
                                    'diskTypeId': 'local-ssd',
                                    'diskSize': 10737418240,
                                },
                                'config': {
                                    'net': {
                                        'maxIncomingConnections': 128,
                                    },
                                },
                            },
                        },
                    },
                    'databaseSpecs': [
                        {
                            'name': 'testdb1',
                        },
                        {
                            'name': 'testdb2',
                        },
                        {
                            'name': 'testdb3',
                        },
                    ],
                    'userSpecs': [{
                        'name': 'test_user',
                        'password': 'mysupercooltestpassword11111',
                        'permissions': [{
                            'databaseName': 'testdb1',
                            'roles': ['read'],
                        }],
                    },
                                  {
                                      'name':
                                          'another_test_user',
                                      'password':
                                          'mysupercooltestpassword11111',
                                      'permissions': [{
                                          'databaseName': 'testdb1',
                                          'roles': ['readWrite'],
                                      }, {
                                          'databaseName': 'testdb2',
                                          'roles': ['read'],
                                      }],
                                  }, {
                                      'name': 'and_yet_another_test_user',
                                      'password': 'mysupercooltestpassword11111',
                                  }],
                    'hostSpecs': [{
                        'zoneId': 'man',
                    }],
                },
                'one_node_replicaset_4_4': {
                    'environment':
                        'PRESTABLE',
                    'configSpec': {
                        'mongodbSpec_4_4': {
                            'mongod': {
                                'resources': {
                                    'resourcePresetId': 'db1.nano',
                                    'diskTypeId': 'local-ssd',
                                    'diskSize': 10737418240,
                                },
                                'config': {
                                    'net': {
                                        'maxIncomingConnections': 128,
                                    },
                                },
                            },
                        },
                    },
                    'databaseSpecs': [
                        {
                            'name': 'testdb1',
                        },
                        {
                            'name': 'testdb2',
                        },
                        {
                            'name': 'testdb3',
                        },
                    ],
                    'userSpecs': [{
                        'name': 'test_user',
                        'password': 'mysupercooltestpassword11111',
                        'permissions': [{
                            'databaseName': 'testdb1',
                            'roles': ['read'],
                        }],
                    },
                                  {
                                      'name':
                                          'another_test_user',
                                      'password':
                                          'mysupercooltestpassword11111',
                                      'permissions': [{
                                          'databaseName': 'testdb1',
                                          'roles': ['readWrite'],
                                      }, {
                                          'databaseName': 'testdb2',
                                          'roles': ['read'],
                                      }],
                                  }, {
                                      'name': 'and_yet_another_test_user',
                                      'password': 'mysupercooltestpassword11111',
                                  }],
                    'hostSpecs': [{
                        'zoneId': 'man',
                    }],
                },
                'one_node_replicaset_5_0': {
                    'environment':
                        'PRESTABLE',
                    'configSpec': {
                        'mongodbSpec_5_0': {
                            'mongod': {
                                'resources': {
                                    'resourcePresetId': 'db1.nano',
                                    'diskTypeId': 'local-ssd',
                                    'diskSize': 10737418240,
                                },
                                'config': {
                                    'net': {
                                        'maxIncomingConnections': 128,
                                    },
                                },
                            },
                        },
                    },
                    'databaseSpecs': [
                        {
                            'name': 'testdb1',
                        },
                        {
                            'name': 'testdb2',
                        },
                        {
                            'name': 'testdb3',
                        },
                    ],
                    'userSpecs': [{
                        'name': 'test_user',
                        'password': 'mysupercooltestpassword11111',
                        'permissions': [{
                            'databaseName': 'testdb1',
                            'roles': ['read'],
                        }],
                    },
                                  {
                                      'name':
                                          'another_test_user',
                                      'password':
                                          'mysupercooltestpassword11111',
                                      'permissions': [{
                                          'databaseName': 'testdb1',
                                          'roles': ['readWrite'],
                                      }, {
                                          'databaseName': 'testdb2',
                                          'roles': ['read'],
                                      }],
                                  }, {
                                      'name': 'and_yet_another_test_user',
                                      'password': 'mysupercooltestpassword11111',
                                  }],
                    'hostSpecs': [{
                        'zoneId': 'man',
                    }],
                },
            },
            'mysql': {
                'standard': {
                    'environment':
                        'PRESTABLE',
                    'configSpec': {
                        'mysqlConfig_5_7': {},
                        'resources': {
                            'resourcePresetId': 'db1.nano',
                            'diskTypeId': 'local-ssd',
                            'diskSize': 10737418240,
                        },
                    },
                    'databaseSpecs': [
                        {
                            'name': 'testdb1',
                        },
                        {
                            'name': 'testdb2',
                        },
                        {
                            'name': 'testdb3',
                        },
                    ],
                    'userSpecs': [{
                        'name': 'test_user',
                        'password': 'mysupercooltestpassword11111',
                        'permissions': [{
                            'databaseName': 'testdb1',
                            'roles': ['SELECT'],
                        }],
                    },
                                  {
                                      'name':
                                          'another_test_user',
                                      'password':
                                          'mysupercooltestpassword11111',
                                      'permissions': [{
                                          'databaseName': 'testdb1',
                                          'roles': ['ALL_PRIVILEGES'],
                                      }, {
                                          'databaseName': 'testdb2',
                                          'roles': ['SELECT'],
                                      }],
                                  }, {
                                      'name': 'and_yet_another_test_user',
                                      'password': 'mysupercooltestpassword11111',
                                  }],
                    'hostSpecs': [{
                        'zoneId': 'man',
                    }, {
                        'zoneId': 'sas',
                    }, {
                        'zoneId': 'vla',
                    }],
                },
                'standard 8': {
                    'environment':
                        'PRESTABLE',
                    'configSpec': {
                        'mysqlConfig_8_0': {},
                        'resources': {
                            'resourcePresetId': 'db1.nano',
                            'diskTypeId': 'local-ssd',
                            'diskSize': 10737418240,
                        },
                    },
                    'databaseSpecs': [
                        {
                            'name': 'testdb1',
                        },
                        {
                            'name': 'testdb2',
                        },
                        {
                            'name': 'testdb3',
                        },
                    ],
                    'userSpecs': [{
                        'name': 'test_user',
                        'password': 'mysupercooltestpassword11111',
                        'permissions': [{
                            'databaseName': 'testdb1',
                            'roles': ['SELECT'],
                        }],
                    },
                                  {
                                      'name':
                                          'another_test_user',
                                      'password':
                                          'mysupercooltestpassword11111',
                                      'permissions': [{
                                          'databaseName': 'testdb1',
                                          'roles': ['ALL_PRIVILEGES'],
                                      }, {
                                          'databaseName': 'testdb2',
                                          'roles': ['SELECT'],
                                      }],
                                  }, {
                                      'name': 'and_yet_another_test_user',
                                      'password': 'mysupercooltestpassword11111',
                                  }],
                    'hostSpecs': [{
                        'zoneId': 'man',
                    }, {
                        'zoneId': 'sas',
                    }, {
                        'zoneId': 'vla',
                    }],
                },
                'single_instance': {
                    'environment':
                        'PRESTABLE',
                    'configSpec': {
                        'mysqlConfig_5_7': {},
                        'resources': {
                            'resourcePresetId': 'db1.nano',
                            'diskTypeId': 'local-ssd',
                            'diskSize': 10737418240,
                        },
                    },
                    'databaseSpecs': [
                        {
                            'name': 'testdb1',
                        },
                        {
                            'name': 'testdb2',
                        },
                        {
                            'name': 'testdb3',
                        },
                    ],
                    'userSpecs': [{
                        'name': 'test_user',
                        'password': 'mysupercooltestpassword11111',
                        'permissions': [{
                            'databaseName': 'testdb1',
                            'roles': ['SELECT'],
                        }],
                    },
                                  {
                                      'name':
                                          'another_test_user',
                                      'password':
                                          'mysupercooltestpassword11111',
                                      'permissions': [{
                                          'databaseName': 'testdb1',
                                          'roles': ['ALL_PRIVILEGES'],
                                      }, {
                                          'databaseName': 'testdb2',
                                          'roles': ['SELECT'],
                                      }],
                                  }, {
                                      'name': 'and_yet_another_test_user',
                                      'password': 'mysupercooltestpassword11111',
                                  }],
                    'hostSpecs': [{
                        'zoneId': 'man',
                    }],
                },
            },
            'redis': {
                'standard': {
                    'environment': 'PRESTABLE',
                    'configSpec': {
                        'version': '5.0',
                        'redisConfig_5_0': {
                            'maxmemoryPolicy': 'VOLATILE_LRU',
                            'timeout': 100,
                            'password': 'passw0rd',
                        },
                        'resources': {
                            'resourcePresetId': 'db1.nano',
                            'diskSize': 17179869184,
                        },
                    },
                    'hostSpecs': [
                        {
                            'zoneId': 'man',
                        },
                        {
                            'zoneId': 'sas',
                        },
                        {
                            'zoneId': 'vla',
                        },
                    ],
                },
                'single_instance': {
                    'environment': 'PRESTABLE',
                    'configSpec': {
                        'version': '5.0',
                        'redisConfig_5_0': {
                            'maxmemoryPolicy': 'VOLATILE_LRU',
                            'timeout': 100,
                            'password': 'passw0rd',
                        },
                        'resources': {
                            'resourcePresetId': 'db1.nano',
                            'diskSize': 17179869184,
                        },
                    },
                    'hostSpecs': [{
                        'zoneId': 'man',
                    }],
                },
                'sharded': {
                    'environment':
                        'PRESTABLE',
                    'sharded':
                        True,
                    'configSpec': {
                        'version': '5.0',
                        'redisConfig_5_0': {
                            'maxmemoryPolicy': 'VOLATILE_LRU',
                            'timeout': 100,
                            'password': 'passw0rd',
                        },
                        'resources': {
                            'resourcePresetId': 'db1.nano',
                            'diskSize': 17179869184,
                        },
                    },
                    'hostSpecs': [{
                        'zoneId': 'myt',
                        'shardName': 'shard1',
                    }, {
                        'zoneId': 'iva',
                        'shardName': 'shard2',
                    }, {
                        'zoneId': 'sas',
                        'shardName': 'shard3',
                    }, {
                        'zoneId': 'iva',
                        'shardName': 'shard1',
                    }, {
                        'zoneId': 'sas',
                        'shardName': 'shard2',
                    }, {
                        'zoneId': 'myt',
                        'shardName': 'shard3',
                    }],
                },
                'standard_6': {
                    'environment': 'PRESTABLE',
                    'configSpec': {
                        'version': '6.0',
                        'redisConfig_6_0': {
                            'maxmemoryPolicy': 'VOLATILE_LRU',
                            'timeout': 100,
                            'password': 'passw0rd',
                        },
                        'resources': {
                            'resourcePresetId': 'db1.nano',
                            'diskSize': 17179869184,
                        },
                    },
                    'hostSpecs': [
                        {
                            'zoneId': 'man',
                        },
                        {
                            'zoneId': 'sas',
                        },
                        {
                            'zoneId': 'vla',
                        },
                    ],
                },
                'single_instance_6': {
                    'environment': 'PRESTABLE',
                    'configSpec': {
                        'version': '6.0',
                        'redisConfig_6_0': {
                            'maxmemoryPolicy': 'VOLATILE_LRU',
                            'timeout': 100,
                            'password': 'passw0rd',
                        },
                        'resources': {
                            'resourcePresetId': 'db1.nano',
                            'diskSize': 17179869184,
                        },
                    },
                    'hostSpecs': [{
                        'zoneId': 'man',
                    }],
                },
                'sharded_6': {
                    'environment':
                        'PRESTABLE',
                    'sharded':
                        True,
                    'configSpec': {
                        'version': '6.0',
                        'redisConfig_6_0': {
                            'maxmemoryPolicy': 'VOLATILE_LRU',
                            'timeout': 100,
                            'password': 'passw0rd',
                        },
                        'resources': {
                            'resourcePresetId': 'db1.nano',
                            'diskSize': 17179869184,
                        },
                    },
                    'hostSpecs': [{
                        'zoneId': 'myt',
                        'shardName': 'shard1',
                    }, {
                        'zoneId': 'iva',
                        'shardName': 'shard2',
                    }, {
                        'zoneId': 'sas',
                        'shardName': 'shard3',
                    }, {
                        'zoneId': 'iva',
                        'shardName': 'shard1',
                    }, {
                        'zoneId': 'sas',
                        'shardName': 'shard2',
                    }, {
                        'zoneId': 'myt',
                        'shardName': 'shard3',
                    }],
                },
                'standard_6_tls': {
                    'environment': 'PRESTABLE',
                    'tlsEnabled': True,
                    'configSpec': {
                        'version': '6.0',
                        'redisConfig_6_0': {
                            'maxmemoryPolicy': 'VOLATILE_LRU',
                            'timeout': 100,
                            'password': 'passw0rd',
                        },
                        'resources': {
                            'resourcePresetId': 'db1.nano',
                            'diskSize': 17179869184,
                        },
                    },
                    'hostSpecs': [
                        {
                            'zoneId': 'man',
                        },
                        {
                            'zoneId': 'sas',
                        },
                        {
                            'zoneId': 'vla',
                        },
                    ],
                },
                'sharded_6_tls': {
                    'environment':
                        'PRESTABLE',
                    'sharded':
                        True,
                    'tlsEnabled':
                        True,
                    'configSpec': {
                        'version': '6.0',
                        'redisConfig_6_0': {
                            'maxmemoryPolicy': 'VOLATILE_LRU',
                            'timeout': 100,
                            'password': 'passw0rd',
                        },
                        'resources': {
                            'resourcePresetId': 'db1.nano',
                            'diskSize': 17179869184,
                        },
                    },
                    'hostSpecs': [{
                        'zoneId': 'myt',
                        'shardName': 'shard1',
                    }, {
                        'zoneId': 'iva',
                        'shardName': 'shard2',
                    }, {
                        'zoneId': 'sas',
                        'shardName': 'shard3',
                    }, {
                        'zoneId': 'iva',
                        'shardName': 'shard1',
                    }, {
                        'zoneId': 'sas',
                        'shardName': 'shard2',
                    }, {
                        'zoneId': 'myt',
                        'shardName': 'shard3',
                    }],
                },
                'standard_62_tls': {
                    'environment': 'PRESTABLE',
                    'tlsEnabled': True,
                    'configSpec': {
                        'version': '6.2',
                        'redisConfig_6_2': {
                            'maxmemoryPolicy': 'VOLATILE_LRU',
                            'timeout': 100,
                            'password': 'passw0rd',
                        },
                        'resources': {
                            'resourcePresetId': 'db1.nano',
                            'diskSize': 17179869184,
                        },
                    },
                    'hostSpecs': [
                        {
                            'zoneId': 'man',
                        },
                        {
                            'zoneId': 'sas',
                        },
                        {
                            'zoneId': 'vla',
                        },
                    ],
                },
                'sharded_62_tls': {
                    'environment':
                        'PRESTABLE',
                    'sharded':
                        True,
                    'tlsEnabled':
                        True,
                    'configSpec': {
                        'version': '6.2',
                        'redisConfig_6_2': {
                            'maxmemoryPolicy': 'VOLATILE_LRU',
                            'timeout': 100,
                            'password': 'passw0rd',
                        },
                        'resources': {
                            'resourcePresetId': 'db1.nano',
                            'diskSize': 17179869184,
                        },
                    },
                    'hostSpecs': [{
                        'zoneId': 'myt',
                        'shardName': 'shard1',
                    }, {
                        'zoneId': 'iva',
                        'shardName': 'shard2',
                    }, {
                        'zoneId': 'sas',
                        'shardName': 'shard3',
                    }, {
                        'zoneId': 'iva',
                        'shardName': 'shard1',
                    }, {
                        'zoneId': 'sas',
                        'shardName': 'shard2',
                    }, {
                        'zoneId': 'myt',
                        'shardName': 'shard3',
                    }],
                },
                'standard_7': {
                    'environment': 'PRESTABLE',
                    'configSpec': {
                        'version': '7.0',
                        'redisConfig_7_0': {
                            'maxmemoryPolicy': 'VOLATILE_LRU',
                            'timeout': 100,
                            'password': 'passw0rd',
                        },
                        'resources': {
                            'resourcePresetId': 'db1.nano',
                            'diskSize': 17179869184,
                        },
                    },
                    'hostSpecs': [
                        {
                            'zoneId': 'man',
                        },
                        {
                            'zoneId': 'sas',
                        },
                        {
                            'zoneId': 'vla',
                        },
                    ],
                },
                'single_instance_7': {
                    'environment': 'PRESTABLE',
                    'configSpec': {
                        'version': '7.0',
                        'redisConfig_7_0': {
                            'maxmemoryPolicy': 'VOLATILE_LRU',
                            'timeout': 100,
                            'password': 'passw0rd',
                        },
                        'resources': {
                            'resourcePresetId': 'db1.nano',
                            'diskSize': 17179869184,
                        },
                    },
                    'hostSpecs': [{
                        'zoneId': 'man',
                    }],
                },
                'sharded_7': {
                    'environment':
                        'PRESTABLE',
                    'sharded':
                        True,
                    'configSpec': {
                        'version': '7.0',
                        'redisConfig_7_0': {
                            'maxmemoryPolicy': 'VOLATILE_LRU',
                            'timeout': 100,
                            'password': 'passw0rd',
                        },
                        'resources': {
                            'resourcePresetId': 'db1.nano',
                            'diskSize': 17179869184,
                        },
                    },
                    'hostSpecs': [{
                        'zoneId': 'myt',
                        'shardName': 'shard1',
                    }, {
                        'zoneId': 'iva',
                        'shardName': 'shard2',
                    }, {
                        'zoneId': 'sas',
                        'shardName': 'shard3',
                    }, {
                        'zoneId': 'iva',
                        'shardName': 'shard1',
                    }, {
                        'zoneId': 'sas',
                        'shardName': 'shard2',
                    }, {
                        'zoneId': 'myt',
                        'shardName': 'shard3',
                    }],
                },
                'standard_7_tls': {
                    'environment': 'PRESTABLE',
                    'tlsEnabled': True,
                    'configSpec': {
                        'version': '7.0',
                        'redisConfig_7_0': {
                            'maxmemoryPolicy': 'VOLATILE_LRU',
                            'timeout': 100,
                            'password': 'passw0rd',
                        },
                        'resources': {
                            'resourcePresetId': 'db1.nano',
                            'diskSize': 17179869184,
                        },
                    },
                    'hostSpecs': [
                        {
                            'zoneId': 'man',
                        },
                        {
                            'zoneId': 'sas',
                        },
                        {
                            'zoneId': 'vla',
                        },
                    ],
                },
                'sharded_7_tls': {
                    'environment':
                        'PRESTABLE',
                    'sharded':
                        True,
                    'tlsEnabled':
                        True,
                    'configSpec': {
                        'version': '7.0',
                        'redisConfig_7_0': {
                            'maxmemoryPolicy': 'VOLATILE_LRU',
                            'timeout': 100,
                            'password': 'passw0rd',
                        },
                        'resources': {
                            'resourcePresetId': 'db1.nano',
                            'diskSize': 17179869184,
                        },
                    },
                    'hostSpecs': [{
                        'zoneId': 'myt',
                        'shardName': 'shard1',
                    }, {
                        'zoneId': 'iva',
                        'shardName': 'shard2',
                    }, {
                        'zoneId': 'sas',
                        'shardName': 'shard3',
                    }, {
                        'zoneId': 'iva',
                        'shardName': 'shard1',
                    }, {
                        'zoneId': 'sas',
                        'shardName': 'shard2',
                    }, {
                        'zoneId': 'myt',
                        'shardName': 'shard3',
                    }],
                },
            },
            'clickhouse': {
                'standard': {
                    'environment':
                        'PRESTABLE',
                    'configSpec': {
                        'clickhouse': {
                            'config': {
                                'logLevel': 'TRACE',
                            },
                            'resources': {
                                'resourcePresetId': 'db1.micro',
                                'diskTypeId': 'local-ssd',
                                'diskSize': 10737418240,
                            },
                        },
                    },
                    'hostSpecs': [
                        {
                            'zoneId': 'man',
                            'type': 'CLICKHOUSE',
                        },
                        {
                            'zoneId': 'vla',
                            'type': 'CLICKHOUSE',
                        },
                    ],
                    'databaseSpecs': [
                        {
                            'name': 'testdb',
                        },
                        {
                            'name': 'testdb_2',
                        },
                    ],
                    'userSpecs': [{
                        'name': 'test_user',
                        'password': 'mysupercooltestpassword11111',
                    }, {
                        'name': 'another_test_user',
                        'password': 'mysupercooltestpassword11111',
                        'permissions': [],
                    }],
                },
                'single': {
                    'environment':
                        'PRESTABLE',
                    'configSpec': {
                        'clickhouse': {
                            'config': {
                                'logLevel': 'TRACE',
                            },
                            'resources': {
                                'resourcePresetId': 'db1.micro',
                                'diskTypeId': 'local-ssd',
                                'diskSize': 10737418240,
                            },
                        },
                    },
                    'hostSpecs': [{
                        'zoneId': 'man',
                        'type': 'CLICKHOUSE',
                    }],
                    'databaseSpecs': [
                        {
                            'name': 'testdb',
                        },
                        {
                            'name': 'testdb_2',
                        },
                        {
                            'name': 'testdb_big',
                        },
                    ],
                    'userSpecs': [{
                        'name': 'test_user',
                        'password': 'mysupercooltestpassword11111',
                    }, {
                        'name': 'another_test_user',
                        'password': 'mysupercooltestpassword11111',
                        'permissions': [],
                    }],
                },
            },
            'greenplum': {
                'standard': {
                    'environment': 'PRESTABLE',
                    'name': 'test',
                    'description': 'test cluster',
                    'config': {
                        'version': '6.19',
                        'zone_id': 'myt',
                        'subnet_id': '',
                        'assign_public_ip': False,
                    },
                    'master_config': {
                        'resources': {
                            'resourcePresetId': 'db1.nano',
                            'diskTypeId': 'local-ssd',
                            'diskSize': 10737418240,
                        },
                    },
                    'segment_config': {
                        'resources': {
                            'resourcePresetId': 'db1.medium',
                            'diskTypeId': 'local-ssd',
                            'diskSize': 10737418240,
                        },
                    },
                    'segment_in_host': 3,
                    'segment_host_count': 4,
                    'user_name': 'usr1',
                    'user_password': 'Pa$$w0rd',
                },
            },
            'elasticsearch': {
                'standard': {
                    'environment':
                        'PRESTABLE',
                    'config_spec': {
                        # 'version': '7.6',  # no version = just use default version for infra tests
                        'admin_password': 'password',
                        'elasticsearch_spec': {
                            'data_node': {
                                'elasticsearch_config_7': {
                                    'fielddata_cache_size': '512mb',
                                    'max_clause_count': 200,
                                },
                                'resources': {
                                    'resource_preset_id': 'db1.micro',
                                    'disk_type_id': 'local-ssd',
                                    'disk_size': 10737418240,
                                },
                            },
                            'master_node': {
                                'resources': {
                                    'resource_preset_id': 'db1.micro',
                                    'disk_type_id': 'local-ssd',
                                    'disk_size': 10737418240,
                                },
                            },
                        },
                    },
                    'hostSpecs': [
                        {
                            'zoneId': 'man',
                            'type': 'DATA_NODE',
                        },
                        {
                            'zoneId': 'vla',
                            'type': 'DATA_NODE',
                        },
                        {
                            'zoneId': 'vla',
                            'type': 'MASTER_NODE',
                        },
                        {
                            'zoneId': 'man',
                            'type': 'MASTER_NODE',
                        },
                        {
                            'zoneId': 'iva',
                            'type': 'MASTER_NODE',
                        },
                    ],
                },
                'single': {
                    'environment': 'PRESTABLE',
                    'config_spec': {
                        # 'version': '7.6',  # no version = just use default version for infra tests
                        'admin_password': 'password',
                        'elasticsearch_spec': {
                            'data_node': {
                                'elasticsearch_config_7': {
                                    'fielddata_cache_size': '512mb',
                                    'max_clause_count': 200,
                                },
                                'resources': {
                                    'resource_preset_id': 'db1.micro',
                                    'disk_type_id': 'local-ssd',
                                    'disk_size': 10737418240,
                                },
                            },
                        },
                    },
                    'hostSpecs': [{
                        'zoneId': 'man',
                        'type': 'DATA_NODE',
                    }],
                },
            },
            'opensearch': {
                'single': {
                    'environment': 'PRESTABLE',
                    'config_spec': {
                        'admin_password': 'password',
                        'opensearch_spec': {
                            'data_node': {
                                'resources': {
                                    'resource_preset_id': 'db1.micro',
                                    'disk_type_id': 'local-ssd',
                                    'disk_size': 10737418240,
                                },
                            },
                        },
                    },
                    'hostSpecs': [{
                        'zoneId': 'man',
                        'type': 'DATA_NODE',
                    }],
                },
                'standard': {
                    'environment':
                        'PRESTABLE',
                    'config_spec': {
                        'admin_password': 'password',
                        'opensearch_spec': {
                            'data_node': {
                                'resources': {
                                    'resource_preset_id': 'db1.micro',
                                    'disk_type_id': 'local-ssd',
                                    'disk_size': 10737418240,
                                },
                            },
                            'master_node': {
                                'resources': {
                                    'resource_preset_id': 'db1.micro',
                                    'disk_type_id': 'local-ssd',
                                    'disk_size': 10737418240,
                                },
                            },
                        },
                    },
                    'hostSpecs': [
                        {
                            'zoneId': 'man',
                            'type': 'MASTER_NODE',
                        },
                        {
                            'zoneId': 'vla',
                            'type': 'DATA_NODE',
                        },
                        {
                            'zoneId': 'iva',
                            'type': 'DATA_NODE',
                        },
                    ],
                },
            },
        },
        # A dict with all projects that are going to interact in this
        # testing environment.
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
            'salt-master': {
                'build': {
                    'path': 'images/salt-master',
                    'pull': False,
                },
                'prebuild_cmd': [
                    "mkdir -p staging/images/salt-master/bin",
                    ya_make_cmd("deploy/saltkeys/cmd/mdb-deploy-saltkeys"),
                    "cp ../deploy/saltkeys/cmd/mdb-deploy-saltkeys/mdb-deploy-saltkeys "
                    "staging/images/salt-master/bin/",
                    "rsync --delete -a ../salt/salt/ staging/code/salt-master/srv/salt/",
                    "rsync --delete -a ../pg/ staging/code/salt-master/srv/salt/components/pg-code",
                ],
                'docker_instances':
                    1,
                'cleanup_strategy':
                    'recreate',
            },
            'postgresql': {
                'docker_instances': 0,
            },
            'clickhouse': {
                'docker_instances': 0,
            },
            'mongodb': {
                'docker_instances': 0,
            },
            'redis': {
                'docker_instances': 0,
            },
            'mysql': {
                'docker_instances': 0,
            },
            'zookeeper-template': {
                'docker_instances': 0,
            },
            'elasticsearch': {
                'docker_instances': 0,
            },
            'opensearch': {
                'docker_instances': 0,
            },
            'greenplum': {
                'docker_instances': 0,
            },
            'base-deploy': {
                'docker_instances': 0,
            },
            'blackbox': {
                'build': {
                    'path': 'code/go-mdb/deploy/integration_test/images/blackbox',
                    'pull': False,
                },
                'expose': {
                    'https': 443,
                },
            },
            'dns-discovery': {
                'build': {
                    'path': 'images/dns-discovery',
                    'pull': False,
                },
                'docker_instances': 1,
                'networks': {
                    'test_net': {
                        'ipv4_address': str(IPNetwork(dynamic_config['docker_ip4_subnet'])[254]),
                        'ipv6_address': str(IPNetwork(dynamic_config['docker_ip6_subnet'])[254]),
                    },
                },
                'volumes': ['/var/run/docker.sock:/var/run/docker.sock:rw'],
                'no_tmpfs': True,
            },
            'zookeeper': {
                'build': {
                    'path': 'images/zookeeper',
                    'pull': False,
                },
                # How many instances o that container are created.
                # Each will have a name like "zookeeper%02d"
                'docker_instances': 1,
                'cleanup_strategy': 'recreate',
                'expose': {
                    'zk': 2181,
                    'zk_tls': 2281,
                },
            },
            'fake_juggler': {
                'build': {
                    'path': 'images/fake_juggler',
                    'pull': False,
                },
                'cleanup_strategy': 'reset_mock',
                'config': {
                    'oauth': {
                        'token': 'juggler_oauth_token',
                    },
                },
                'expose': {
                    'http': 80,
                },
            },
            'fake_dbm': {
                'build': {
                    'path': 'images/fake_dbm',
                    'pull': False,
                },
                'config': {
                    'oauth': {
                        'token': 'dbm_oauth_token',
                    },
                    'fake_queue_hosts': '__NONEXISTENT_HOST',
                },
                'expose': {
                    'http': 80,
                },
                'cleanup_strategy': 'reset_dbm_mock',
                'containers': {
                    'exposes': {
                        'clickhouse-bionic': {
                            'https': 8443,
                        },
                        'postgresql-bionic': {
                            'pgbouncer': 6432,
                            'postgres': 5432,
                        },
                        'mongodb-bionic': {
                            'mongos': 27017,
                            'mongod': 27018,
                            'mongocfg': 27019,
                        },
                        'zookeeper-bionic': {
                            'zookeeper': 2181,
                            'zookeeper_tls': 2281,
                        },
                        'redis-bionic': {
                            'server': 6379,
                            'sentinel': 26379,
                            'cluster': 16379,
                            'server_tls': 6380,
                            'sentinel_tls': 26380,
                            'cluster_tls': 16380,
                        },
                        'mysql-bionic': {
                            'mysql': 3306,
                        },
                        'elasticsearch-bionic': {
                            'elasticsearch': 9200,
                        },
                        'opensearch-bionic': {
                            'opensearch': 9200,
                        },
                        'greenplum-bionic': {
                            'greenplum': 5432,
                        },
                    },
                    'bootstraps': {
                        '/usr/local/yandex/porto/mdb_dbaas_pg_bionic.sh': 'postgresql-bionic',
                        '/usr/local/yandex/porto/mdb_dbaas_ch_bionic.sh': 'clickhouse-bionic',
                        '/usr/local/yandex/porto/mdb_dbaas_mongodb_bionic.sh': 'mongodb-bionic',
                        '/usr/local/yandex/porto/mdb_dbaas_zk_bionic.sh': 'zookeeper-bionic',
                        '/usr/local/yandex/porto/mdb_dbaas_redis_bionic.sh': 'redis-bionic',
                        '/usr/local/yandex/porto/mdb_dbaas_mysql.sh': 'mysql-bionic',
                        '/usr/local/yandex/porto/mdb_dbaas_elasticsearch_bionic.sh': 'elasticsearch-bionic',
                        '/usr/local/yandex/porto/mdb_dbaas_opensearch_bionic.sh': 'opensearch-bionic',
                        '/usr/local/yandex/porto/mdb_dbaas_gpdb_bionic.sh': 'greenplum-bionic',
                    },
                },
            },
            'fake_certificator': {
                'build': {
                    'path': 'images/fake_certificator',
                    'pull': False,
                },
                'config': {
                    'oauth': {
                        'token': 'certificator_oauth_token',
                    },
                },
            },
            'fake_iam': {
                'build': {
                    'path': 'images/fake_iam',
                    'pull': False,
                },
                'cleanup_strategy': 'reset_access_service_mock',
                'git': {
                    'identity': {
                        'base': 'bitbucket',
                        'repo': 'cloud/identity.git',
                        'commit': '19906b1008d1b1508558de5472f31db3e5bc1993',
                    },
                },
                'expose': {
                    'as': 4284,
                    'as-control': 2484,
                    'identity': 4336,
                },
            },
            'fake_conductor': {
                'build': {
                    'path': 'images/fake_conductor',
                    'pull': False,
                },
                'expose': {
                    'http': 80,
                },
                'cleanup_strategy': 'reset_mock',
                'config': {
                    'oauth': {
                        'token': 'conductor_oauth_token',
                    },
                },
            },
            'fake_solomon': {
                'build': {
                    'path': 'images/fake_solomon',
                    'pull': False,
                },
                'expose': {
                    'http': 80,
                },
                'cleanup_strategy': 'reset_mock',
                'config': {
                    'oauth': {
                        'token': 'solomon_oauth_token',
                    },
                    'robot_name': 'robot-pgaas-deploy',
                },
            },
            'fake_smtp': {
                'build': {
                    'path': 'images/fake_smtp',
                    'pull': False,
                },
                'expose': {
                    'smtp': 5000,
                    'http': 6000,
                },
                'cleanup_strategy': 'reset_mock',
            },
            'fake_health': {
                'build': {
                    'path': 'images/fake_health',
                    'pull': False,
                },
            },
            'idm-service': {
                'prebuild_cmd': [
                    ya_make_cmd('idm_service/uwsgi'),
                    'cp ../idm_service/uwsgi/mdb-idm-service.wsgi staging/images/idm-service/',
                    'cp ../idm_service/uwsgi/uwsgi.deb.ini staging/images/idm-service/',
                ],
                'build': {
                    'path': 'images/idm-service',
                    'pull': False,
                },
                'expose': {
                    'http': 80,
                },
            },
            'internal-api': {
                'prebuild_cmd': [
                    # pylint: disable=line-too-long
                    ya_make_cmd('dbaas-internal-api-image/uwsgi'),
                    'mkdir -p staging/images/internal-api/uwsgi/etc',
                    'cp ../dbaas-internal-api-image/uwsgi/internal-api.wsgi staging/images/internal-api/uwsgi/',
                    'cp ../dbaas-internal-api-image/uwsgi/etc/uwsgi.ini staging/images/internal-api/uwsgi/etc',
                ],
                'build': {
                    'path': 'images/internal-api',
                    'pull': False,
                },
                # Forms 'ports' docker-compose arg.
                # 'ports' cannot be used directly,
                # as it requires parsing when referenced by Behave.
                'expose': {
                    'https': 443,
                },
            },
            'go-internal-api': {
                'build': {
                    'path': 'images/go-internal-api',
                    'pull': False,
                },
                'prebuild_cmd': [
                    "mkdir -p staging/images/go-internal-api/bin",
                    ya_make_cmd('mdb-internal-api/cmd/infra-test-internal-api'),
                    "cp ../mdb-internal-api/cmd/infra-test-internal-api/infra-test-internal-api "
                    "staging/images/go-internal-api/bin/mdb-internal-api",
                ],
                'code_path':
                    'go-mdb/mdb-internal-api',
                'expose': {
                    'grpc': 50050,
                },
            },
            'fake_resourcemanager': {
                'build': {
                    'path': 'images/fake_resourcemanager',
                    'pull': False,
                },
                'prebuild_cmd': [
                    "mkdir -p staging/images/fake_resourcemanager/bin",
                    ya_make_cmd('internal/compute/resmanager/cmd/resourcemanager-mock'),
                    "cp ../internal/compute/resmanager/cmd/resourcemanager-mock/resourcemanager-mock  "
                    "staging/images/fake_resourcemanager/bin/resourcemanager-mock",
                ],
                'code_path':
                    'go-mdb/fake_resourcemanager',
                'expose': {
                    'grpc': 4040,
                    'control': 4041,
                },
            },
            'fake_tokenservice': {
                'build': {
                    'path': 'images/fake_tokenservice',
                    'pull': False,
                },
                'prebuild_cmd': [
                    "mkdir -p staging/images/fake_tokenservice/bin",
                    ya_make_cmd('internal/compute/iam/cmd/tokenservice-mock'),
                    "cp ../internal/compute/iam/cmd/tokenservice-mock/tokenservice-mock  "
                    "staging/images/fake_tokenservice/bin/tokenservice-mock",
                ],
                'code_path':
                    'go-mdb/fake_tokenservice',
                'expose': {
                    'grpc': 50051,
                    'rest': 50052,
                },
            },
            'mlock': {
                'build': {
                    'path': 'images/mlock',
                    'pull': False,
                },
                'prebuild_cmd': [
                    'mkdir -p staging/images/mlock/bin',
                    ya_make_cmd('mlock/cmd/mdb-mlock'),
                    'cp ../mlock/cmd/mdb-mlock/mdb-mlock staging/images/mlock/bin/mlock',
                ],
                'code_path':
                    'go-mdb/mlock',
                'expose': {
                    'grpc': 30030,
                },
            },
            'mlockdb': {
                'build': {
                    'path': 'images/mlockdb',
                    'pull': False,
                },
                'prebuild_cmd': ['cp -r ../mlockdb staging/images/mlockdb/data'],
                'code_path': 'go-mdb/mlockdb',
                # We need this options for backward compatibility with minipgaas (to allow logs dump)
                'db': {
                    'dbname': 'mlockdb',
                    'user': 'mlock',
                    'password': 'mlock',
                },
                'mlockdb01': {
                    'expose': {
                        'pgbouncer': 5432,
                    },
                },
            },
            'dbaas-worker': {
                'prebuild_cmd': [
                    # pylint: disable=line-too-long
                    ya_make_cmd('dbaas_worker/bin'),
                    'mkdir -p staging/images/dbaas-worker/bin',
                    'cp ../dbaas_worker/bin/dbaas-worker staging/images/dbaas-worker/bin/',
                    'cp ../dbaas_worker/dbaas-worker.conf staging/images/dbaas-worker/',
                ],
                'build': {
                    'path': 'images/dbaas-worker',
                },
            },
            'mdb-deploy-api': {
                'build': {
                    'path': 'images/mdb-deploy-api',
                    'pull': False,
                },
                'prebuild_cmd': [
                    "mkdir -p staging/images/mdb-deploy-api/bin",
                    ya_make_cmd('deploy/api/cmd/mdb-deploy-api'),
                    "cp ../deploy/api/cmd/mdb-deploy-api/mdb-deploy-api staging/images/mdb-deploy-api/bin/",
                ],
                'code_path':
                    'go-mdb/deploy/api',
                'expose': {
                    'https': 443,
                },
            },
            'deploy_db': {
                'build': {
                    'path': 'images/deploy_db',
                    'pull': False,
                },
                'code_path': 'go-mdb/deploydb',
                'prebuild_cmd': ['cp -r ../deploydb staging/images/deploy_db/data'],
                # We don't need replicas, so we use 1 node here
                'docker_instances': 1,
                'db': {
                    'dbname': 'deploydb',
                    'user': 'deploy_api',
                    'password': 'deploy_api',
                    'users': {
                        'deploy_cleaner': 'deploy_cleaner',
                        'mdb_ui': 'mdb_ui',
                    },
                },
                'cleanup_strategy': 'recreate',
                # Instance-specific config.
                # In this case, we need deploydb01 to become master.
                'deploy_db01': {
                    # Expose master port to localhost,
                    # so that local tests will be able to reach it.
                    'expose': {
                        'pgbouncer': 5432,
                    },
                    'environment': {
                        # Template placeholder rendered
                        # at compose file generation stage
                        'OPT_role_mode': 'master',
                    },
                },
            },
            'backup': {
                'build': {
                    'path': 'images/backup',
                    'pull': False,
                },
                'prebuild_cmd': [
                    'mkdir -p staging/images/backup/bin',
                    ya_make_cmd('backup/worker/cmd/worker'),
                    'cp ../backup/worker/cmd/worker/worker staging/images/backup/bin/',
                    ya_make_cmd('backup/worker/cmd/cli'),
                    'cp ../backup/worker/cmd/cli/cli staging/images/backup/bin/',
                ],
                'code_path':
                    'go-mdb/backup',
            },
            'metadb': {
                'build': {
                    'path': 'images/metadb',
                    'pull': False,
                },
                'prebuild_cmd': ['cp -r staging/code/metadb/ staging/images/metadb/data'],
                # We don't need replicas, so we use 1 node here
                'docker_instances': 1,
                'db': {
                    'dbname': 'dbaas_metadb',
                    'user': 'dbaas_api',
                    'password': 'dbaas_api',
                    'users': {
                        'dbaas_worker': 'dbaas_worker',
                        'idm_service': 'idm_service',
                        'dbaas_support': 'dbaas_support',
                        'mdb_health': 'mdb_health',
                        'mdb_dns': 'mdb_dns',
                        'mdb_report': 'mdb_report',
                        'mdb_dispenser_sync': 'mdb_dispenser_sync',
                        'mdb_search_producer': 'mdb_search_producer',
                        'mdb_event_producer': 'mdb_event_producer',
                        'katan_imp': 'katan_imp',
                        'mdb_maintenance': 'mdb_maintenance',
                        'backup_scheduler': 'backup_scheduler',
                        'backup_worker': 'backup_worker',
                        'backup_cli': 'backup_cli',
                        'mdb_ui': 'mdb_ui',
                        'dataproc_health': 'dataproc_health',
                        'mdb_downtimer': 'mdb_downtimer',
                        'cms': 'cms',
                    },
                },
                'cleanup_strategy': 'recreate',
                # Instance-specific config.
                # In this case, we need metadb01 to become master.
                'metadb01': {
                    # Expose master port to localhost,
                    # so that local tests will be able to reach it.
                    'expose': {
                        'pgbouncer': 5432,
                    },
                    'environment': {
                        # Template placeholder rendered
                        # at compose file generation stage
                        'OPT_role_mode': 'master',
                    },
                },
            },
            'minio': {
                'prebuild_cmd': [
                    'mkdir -p staging/images/minio/bin',
                    '/usr/bin/s3cmd -c /etc/s3cmd.cfg get --skip-existing '
                    's3://dbaas-infra-test-cache/minio.RELEASE.2021-01-16T02-19-44Z.gz '
                    'staging/images/minio/bin/minio.gz',
                    'gunzip -f staging/images/minio/bin/minio.gz',
                    '/usr/bin/s3cmd -c /etc/s3cmd.cfg get --skip-existing '
                    's3://dbaas-infra-test-cache/mc.RELEASE.2021-01-16T02-45-34Z.gz '
                    'staging/images/minio/bin/mc.gz',
                    'gunzip -f staging/images/minio/bin/mc.gz',
                ],
                'build': {
                    'path': 'images/minio',
                    'pull': False,
                },
                'expose': {
                    'http': 9000,
                    'https': 443,
                },
                'cleanup_strategy':
                    'recreate',
            },
            'pykmip': {
                'build': {
                    'path': 'images/pykmip',
                    'pull': False,
                },
                'expose': {
                    'kmip': 5696,
                },
                'cleanup_strategy': 'recreate',
            },
            'secretsdb': {
                'build': {
                    'path': 'images/secretsdb',
                    'pull': False,
                },
                'code_path': 'go-mdb/secretsdb',
                'prebuild_cmd': ['cp -r ../secretsdb staging/images/secretsdb'],
                # We need this options for backward compatibility with minipgaas (to allow logs dump)
                'db': {
                    'dbname': 'secretsdb',
                    'user': 'secrets',
                    'password': 'secrets',
                },
                'secretsdb01': {
                    'expose': {
                        'pgbouncer': 5432,
                    },
                },
            },
            'secrets': {
                'build': {
                    'path': 'images/secrets',
                    'pull': False,
                },
                'prebuild_cmd': [
                    'mkdir -p staging/images/secrets/bin',
                    ya_make_cmd('mdb-secrets/cmd/mdb-secrets'),
                    'cp ../mdb-secrets/cmd/mdb-secrets/mdb-secrets staging/images/secrets/bin/mdb-secrets',
                ],
                'code_path':
                    'go-mdb/mdb-secrets',
                'expose': {
                    'http': 8080,
                },
            },
        },
    }

    _fill_major_versions_in_config(config, net_name)

    return merge(config, CONF_OVERRIDE)


def _fill_major_versions_in_config(config: dict, net_name: str) -> None:
    """
    fill versioned_images and all related stuff
    """
    for img in _get_major_versions_images():
        # From s3-image we create docker '-template' image.
        # On top of docker-template-image we create 'base_image'.
        # From `base_image` we start containers.
        container_type = img.name + '-bionic'
        docker_template_image_name = img.name + '-template'

        staging_path = os.path.join('staging/images', img.name)

        # Fill versioned_images section for versions_image.create_staging stage
        config['versioned_images'][staging_path] = {
            # Copy Dockerfile and all configs from
            'from_dir': os.path.join('images', img.kind),
            'rewrite_docker_from': 'FROM {}:latest'.format(docker_template_image_name),
        }

        config['base_images'][img.name] = {
            'tag': 'dbaas-infra-tests-{type}:{tag}'.format(type=container_type, tag=net_name),
            'path': staging_path,
        }
        config['s3-templates']['images'][docker_template_image_name] = {
            'path': img.s3_path,
        }
        config['projects'][img.name] = {
            'docker_instances': 0,
            'templates-inherits-from': img.kind,
        }

        # adding our mj-image to DBM
        dbm_containers_config: dict = config['projects']['fake_dbm']['containers']
        from_exposes = copy.deepcopy(dbm_containers_config['exposes'][img.expose_same_as])

        dbm_containers_config['exposes'][container_type] = from_exposes
        dbm_containers_config['bootstraps'][img.bootstrap_cmd] = container_type

        # add version to worker whitelist
        config['worker'][img.kind]['dbm_bootstrap_cmd_template']['whitelist'][img.major_version] = img.version


def generate_dynamic_config(net_name):
    """
    Generates dynamic stuff like keys, uuids and other.
    """
    keys = {
        'internal_api': crypto.gen_keypair(),
        'client': crypto.gen_keypair(),
        'secret_api': crypto.gen_keypair(),
    }
    # https://pynacl.readthedocs.io/en/latest/public/#nacl-public-box
    # CryptoBox is a subclass of Box, but returning a string instead.
    api_to_client_box = crypto.CryptoBox(
        keys['internal_api']['secret_obj'],
        keys['client']['public_obj'],
    )
    clouds = {
        'test': {
            'cloud_ext_id': 'rmi99999999999999999',
        },
    }
    folders = {
        'test': {
            'cloud_ext_id': 'rmi99999999999999999',
            'folder_id': 1,
            'folder_ext_id': 'rmi00000000000000001',
        },
    }
    s3_credentials = {
        'access_secret_key': crypto.gen_plain_random_string(40),
        'access_key_id': crypto.gen_plain_random_string(20),
    }
    config = {
        'environment_name': 'porto',
        'salt': {
            'pki': keys['client'],
            'access_id': str(uuid.uuid4()),
            'access_secret': crypto.gen_random_string(),
            'file_api_secret': str(uuid.uuid4()),
        },
        'docker_ip4_subnet': generate_ipv4(),
        'docker_ip6_subnet': generate_ipv6(),
        'clouds': clouds,
        'folders': folders,
        's3': {
            'use_https': False,
            'host': 'minio01.{domain}'.format(domain=net_name),
            'fake_host': 'minio',
            'port': 9000,
            'endpoint': 'http+path://minio01.{domain}:9000'.format(domain=net_name),
            'access_secret_key': s3_credentials['access_secret_key'],
            'access_key_id': s3_credentials['access_key_id'],
            'enc_access_secret_key': api_to_client_box.encrypt_utf(s3_credentials['access_secret_key']),
            'enc_access_key_id': api_to_client_box.encrypt_utf(s3_credentials['access_key_id']),
            'bucket_prefix': 'yandexcloud-dbaas-',
        },
        'internal_api': {
            'pki': keys['internal_api'],
            'box': api_to_client_box,
        },
        'mdb_api_for_worker': {
            'access_id': str(uuid.uuid4()),
            'access_secret': crypto.gen_random_string(),
        },
        'cluster_type_pillars': {
            'postgresql_cluster': {
                'data': {
                    'salt_py_version': 3,
                    'odyssey': {
                        'user': 'postgres',
                    },
                    'config': {
                        'shared_preload_libraries': 'pg_stat_statements,pg_stat_kcache,repl_mon',
                        'archive_timeout': '30',
                    },
                    'use_barman': False,
                    'use_pgsync': True,
                    'use_postgis': True,
                    'use_walg': True,
                },
            },
            'clickhouse_cluster': {
                'data': {
                    'salt_py_version': 3,
                    'cloud_storage': {
                        's3': {
                            'scheme': 'http',
                            'endpoint': 'minio01.{domain}:9000'.format(domain=net_name),
                            'access_key_id': {
                                'data': api_to_client_box.encrypt_utf(s3_credentials['access_key_id']),
                                'encryption_version': 1,
                            },
                            'access_secret_key': {
                                'data': api_to_client_box.encrypt_utf(s3_credentials['access_secret_key']),
                                'encryption_version': 1,
                            },
                        },
                    },
                    'testing_repos': True,
                },
            },
            'mongodb_cluster': {
                'data': {
                    'salt_py_version': 3,
                    'mongodb': {
                        'wait_after_start_secs': 5,
                    },
                    'walg': {
                        'install': True,
                        'enabled': True,
                        'oplog_push': True,
                        'oplog_replay': True,
                        'oplog_archive_timeout': '10s',
                    },
                },
            },
            'mysql_cluster': {
                'data': {
                    'salt_py_version': 3,
                    'mysql': {
                        'critical_disk_usage': 142,
                    },
                },
            },
            'redis_cluster': {
                'data': {
                    'salt_py_version': 3,
                    'walg': {
                        'install': True,
                        'enabled': True,
                    },
                    'redis': {
                        'config': {
                            'bind': '0.0.0.0 ::',
                        },
                        'secrets': {
                            'sentinel_renames': {
                                'RESET': 'secret_reset',
                                'FAILOVER': 'secret_failover',
                            },
                        },
                        'tools': {
                            'redisctl_enabled': True,
                        },
                    },
                    'sentinel': {
                        'config': {
                            'bind': '0.0.0.0 ::',
                        },
                    },
                },
            },
            'greenplum_cluster': {
                'data': {
                    'salt_py_version': 3,
                    'odyssey': {
                        'user': 'gpadmin',
                    },
                    'gp_admin': 'gpadmin',
                    'gp_data_folders': ['/var/lib/greenplum/data1'],
                    'gp_master_directory': '/var/lib/greenplum/data1',
                    'fscreate': False,
                    'use_telegraf': True,
                    'greenplum': {
                        'config': {
                            'gp_resource_manager': 'queue',
                            'gp_interconnect_type': 'udpifc',
                        },
                    },
                    'solomon': {
                        'project': 'dbaas_infra_tests',
                        'push_url': 'https://solomon.yandex-team.ru/api/v2/push',
                        'oauth_token': 'xxx',
                    },
                },
            },
            'elasticsearch_cluster': {
                'data': {
                    'salt_py_version': 3,
                },
            },
            'opensearch_cluster': {
                'data': {
                    'salt_py_version': 3,
                },
            },
        },
        'secrets_api': {
            'pki': keys['secret_api'],
        },
    }
    return config
