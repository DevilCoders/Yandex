#!/usr/bin/env python3
"""
Make override pillar from templates and configs
"""
import os
from getpass import getuser

import yaml

from .utils import ALL_CA_LOCAL_PATH, combine_dict

ALL_CA = '/opt/yandex/allCAs.pem'
DATAPROC_SERVER_NAME = 'dataproc-manager.private-api.cloud-preprod.yandex.net'
DATAPROC_PRIVATE_PORT = 4040
DATAPROC_ADAPTER_PLAINTEXT_PORT = 6666
DATAPROC_PUBLIC_PORT = 11003
# temporarily start UI Proxy on port 2182 in order to free up port 2181 for mdb-internal-api
UI_PROXY_PORT = 2182
MDB_INT_API_PORT = 2181

ACCESS_SERVICE = 'as.private-api.cloud-preprod.yandex.net'
ACCESS_SERVICE_PORT = 4281
COMPUTE_SERVICE = 'iaas.private-api.cloud-preprod.yandex.net'
COMPUTE_GRPC_SERVICE_HOST = 'compute-api.cloud-preprod.yandex.net'
COMPUTE_GRPC_SERVICE_PORT = 9051
VPC_GRPC_SERVICE_HOST = 'network-api-internal.private-api.cloud-preprod.yandex.net'
VPC_GRPC_SERVICE_PORT = 9823
IDENTITY_SERVICE = 'identity.private-api.cloud-preprod.yandex.net'
IDENTITY_SERVICE_PORT = 14336
INSTANCE_GROUP_SERVICE_HOST = 'instance-group.private-api.ycp.cloud-preprod.yandex.net'
INSTANCE_GROUP_SERVICE_PORT = 443
TOKEN_SERVICE = 'ts.private-api.cloud-preprod.yandex.net'
TOKEN_SERVICE_PORT = 4282
LICENSE_SERVICE = 'billing.private-api.cloud-preprod.yandex.net'
LICENSE_SERVICE_PORT = 16465


def get_pillar(conf, fqdn, ipv4_address, runlist, managed):
    CA = open(ALL_CA_LOCAL_PATH).read()
    common = conf['common']
    cert = common['cert']['cert']
    private_key = common['cert']['key']

    saltapi_password_hash = conf['projects']['saltapi']['password_hash']
    nginx = conf['projects']['nginx']
    certificator_mock = conf['projects']['certificator_mock']

    # We should rewrite some fields manually,
    # because some of them has merged incorrectly.
    # For example, lists are replaced instead of appended.
    override = {
        'cert.ca': CA,
        'cert.crt': cert,
        'cert.key': private_key,
        'data': {
            'migrates_in_k8s': False,
            'disable_worker': False,
            'connection_pooler': 'pgbouncer',
            'salt_version': '3002.7+ds-1+yandex0',
            'salt_py_version': 3,
            'is_infratest_env': True,
            'common': {
                'nginx': {
                    'server_names_hash_bucket_size': 128,
                },
                'dh': nginx['dhparam'],
            },
            'config': {
                'archive_command': '/bin/true',
                'pgusers': pgusers_pillar(conf),
                'salt': saltmaster_pillar(conf),
            },
            'infratest_cert': cert,
            'infratest_cert_key': private_key,
            'salt_masterless': True,
            'use_pgsync': False,
            'use_mdbmetrics': False,
            'mdb_metrics': {
                'enabled': False,
            },
            'monrun': False,
            'monrun2': False,
            'nginx_no_mdbmetrics': True,
            'use_walg': False,
            'walg': {
                'enabled': False,
            },
            'use_yasmagent': False,
            'use_pushclient': False,
            'web_api_base': {
                'use_nginx': True,
            },
            'l3host': False,
            'ipv6selfdns': False,
            'runlist': runlist,
            'dbaas': {
                'shard_hosts': [fqdn],
                'cluster_name': 'mdb-dataproc-manager-redis-infratest',
            },
            'internal_api': internal_api_pillar(conf, fqdn, ipv4_address),
            'mdb-internal-api': mdb_internal_api_pillar(conf),
            'dbaas_worker': dbaas_worker_pillar(conf, fqdn),
            # metadb is using cert that is valid for dataproc-manager.private-api.cloud-preprod.yandex.net and
            # *.infratest.db.yandex.net, but connections are only allowed on local socket. So we setup mdb-maintenance
            # to connect to metadb through dataproc-manager.private-api.cloud-preprod.yandex.net
            # which is an alias for 127.0.0.1
            'metadb': {'hosts': ['dataproc-manager.private-api.cloud-preprod.yandex.net']},
            'dbaas_metadb': dbaas_metadb_pillar(conf, fqdn),
            'mdb-dataproc-manager': mdb_dataproc_manager_pillar(conf, fqdn),
            'dataproc-ui-proxy': dataproc_ui_proxy_pillar(conf, fqdn),
            'gateway': gateway_pillar(conf, fqdn),
            'redis': redis_pillar(conf),
            'docker_private_config': docker_pillar(conf),
            'salt_master': {
                'root': '/srv-master',
                'use_s3_images': False,
            },
            'mdb-deploy-saltkeys': saltkeys_pillar(conf, fqdn),
            'mdb-deploy-api': mdb_deploy_api_pillar(conf),
            'mdb-deploy-salt-api': {
                'salt_api_user': 'mdb-deploy-salt-api',
                'salt_api_password': saltapi_password_hash,
            },
            'mdb-deploy-cleaner': {
                'zk_hosts': [],
                'config': {
                    'deploydb': {
                        'postgresql': {
                            'hosts': [],
                        },
                    },
                },
            },
            'tvmtool': tvmtool_pillar(conf),
            'dbaas_pillar': dbaas_pillar(conf, fqdn),
            'mdb_secrets': mdb_secrets_pillar(conf),
            'blackbox-mock': blackbox_pillar(conf),
            'certificator_mock': {
                'private': certificator_mock['private'],
                'cert': certificator_mock['cert'],
                'crl': certificator_mock['crl'],
            },
            'mdb-maintenance': mdb_maintenance_pillar(conf),
            'prefixes': {
                'tasks': {
                    'kafka': 'mdb',
                },
            },
            # override WAL-G configs:
            's3': {
                'endpoint': conf['s3']['backup']['endpoint'],
                'access_key': conf['s3']['backup']['access_key_id'],
                'secret_key': conf['s3']['backup']['access_secret_key'],
            },
            'solomon': {
                'url': '',
            },
        },
    }
    if managed:
        del override['data']['dbaas_metadb']['cluster_type_pillars']
    user_pillar = _get_user_pillar_override()
    if user_pillar:
        override = combine_dict(override, user_pillar)
    return override


def pgusers_pillar(conf):
    common = conf['common']
    deploydb = conf['projects']['deploydb']
    metadb = conf['projects']['metadb']
    metadb_name = metadb['dbname']
    metadb_password = metadb['password']
    return {
        'dbaas_api': {
            'password': metadb_password,
            'allow_port': '6432',
            'allow_db': metadb_name,
            'superuser': True,
            'replication': False,
            'create': True,
            'bouncer': True,
            'connect_dbs': [metadb_name],
        },
        'dbaas_worker': {
            'password': metadb_password,
            'allow_port': '6432',
            'allow_db': metadb_name,
            'superuser': False,
            'replication': False,
            'create': True,
            'bouncer': True,
            'connect_dbs': [metadb_name],
        },
        'postgres': {
            'password': common['pguser_postgres'],
            'allow_port': '*',
            'allow_db': '*',
            'superuser': True,
            'replication': True,
            'create': True,
            'bouncer': True,
            'connect_dbs': ['*'],
            'settings': {
                'default_transaction_isolation': 'read committed',
                'lock_timeout': 0,
                'log_statement': 'mod',
                'synchronous_commit': 'local',
                'temp_file_limit': -1,
            },
        },
        deploydb['user']: {
            'password': deploydb['password'],
            'allow_port': '6432',
            'allow_db': deploydb['dbname'],
            'superuser': True,
            'replication': False,
            'create': True,
            'bouncer': True,
            'connect_dbs': [deploydb['dbname']],
        },
        'mdb_maintenance': {
            'password': metadb_password,
            'allow_port': '6432',
            'allow_db': metadb_name,
            'superuser': False,
            'replication': False,
            'create': True,
            'bouncer': True,
            'connect_dbs': [metadb_name],
        },
    }


def saltmaster_pillar(conf):
    salt_master = conf['projects']['salt_master']
    return {
        'master': {
            'public': salt_master['tls']['public'],
            'private': salt_master['tls']['private'],
        },
        'keys': {
            'pub2': salt_master['keys']['public'],
            'priv': salt_master['keys']['private'],
        },
    }


def internal_api_pillar(conf, fqdn, ipv4_address):
    common = conf['common']
    metadb = conf['projects']['metadb']
    cert = common['cert']['cert']
    private_key = common['cert']['key']
    internal_api = conf['projects']['internal_api']
    return {
        'config': {
            'sentry': {
                'dsn': '',
            },
            'hadoop_pillars': {
                'stable-1-0': {
                    'data': {
                        'unmanaged': {
                            'dataproc-manager': {
                                'cert': open(ALL_CA_LOCAL_PATH).read(),
                            },
                            'agent': {
                                'manager_url': f'{ipv4_address}:{DATAPROC_PUBLIC_PORT}',
                                'ui_proxy_url': f'https://{fqdn}:{UI_PROXY_PORT}/',
                                'cert_server_name': DATAPROC_SERVER_NAME,
                                # TODO replace with /usr/local/share/ca-certificates/yandex-cloud-ca.crt
                                # when new images will be published and will become stable
                                'cert_file': '/srv/CA.pem',
                                'hosts': {
                                    fqdn: ipv4_address,
                                },
                            },
                        },
                    },
                },
            },
            'hadoop_ui_links': {
                'base_url': f'https://cluster-${{CLUSTER_ID}}.{fqdn}:{UI_PROXY_PORT}/gateway/default-topology/',
                'knoxless_base_url': f'https://ui-${{CLUSTER_ID}}-${{HOST}}-${{PORT}}.{fqdn}:{UI_PROXY_PORT}/',
            },
            'metadb': {
                'hosts': [fqdn],
                'password': metadb['password'],
            },
            's3': {
                'endpoint_url': conf['s3']['backup']['endpoint'],
                'aws_access_key_id': conf['s3']['backup']['access_key_id'],
                'aws_secret_access_key': conf['s3']['backup']['access_secret_key'],
            },
            'network': {
                'ca_path': ALL_CA,
            },
            'logsdb': {
                'dsn': {},
                'hosts': [],
            },
            'identity': {
                'ca_path': ALL_CA,
            },
            'yc_identity': {
                'ca_path': ALL_CA,
            },
            'dataproc_manager_config': {
                'url': f'0.0.0.0:{DATAPROC_PRIVATE_PORT}',
                'private_url': f'{DATAPROC_SERVER_NAME}:{DATAPROC_PRIVATE_PORT}',
                'cert_server_name': DATAPROC_SERVER_NAME,
                'cert_file': ALL_CA,
                'insecure': True,
            },
            'crypto': {
                'client_public_key': conf['pillar_key']['public'],
                'api_secret_key': conf['pillar_key']['private'],
            },
        },
        'tls_key': private_key,
        'tls_crt': cert,
        'tls_port': internal_api['port'],
    }


def mdb_internal_api_pillar(conf):
    metadb = conf['projects']['metadb']
    mdb_internal_api = conf['projects']['mdb_internal_api']
    return {
        'config': {
            'app': {
                'logging': {
                    'level': 'debug',
                },
            },
            'api': {
                'expose_error_details': True,
            },
            'grpc': {
                'addr': ':' + str(mdb_internal_api['port']),
            },
            'metadb': {
                'addrs': ['localhost:6432'],
                'db': 'dbaas_metadb',
                'user': 'dbaas_api',
                'password': metadb['password'],
                'sslrootcert': '',
                'sslmode': 'require',
            },
            'logsdb': {
                'addrs': ['localhost'],
                'ca_file': ALL_CA,
            },
            'perfdiagdb_mongodb': {
                'disabled': True,
            },
            'perfdiagdb': {
                'addrs': ['localhost'],
                'ca_file': ALL_CA,
            },
            'health': {
                'host': 'localhost:1234',
                'tls': {
                    'ca_file': ALL_CA,
                },
            },
            'iam': {
                'uri': f'https://{IDENTITY_SERVICE}:{IDENTITY_SERVICE_PORT}',
                'http': {
                    'transport': {
                        'tls': {
                            'ca_file': ALL_CA,
                        },
                    },
                },
            },
            'access_service': {
                'addr': f'{ACCESS_SERVICE}:4286',
                'capath': ALL_CA,
            },
            'token_service': {
                'addr': f'{TOKEN_SERVICE}:{TOKEN_SERVICE_PORT}',
                'capath': ALL_CA,
            },
            'license_service': {
                'addr': f'https://{LICENSE_SERVICE}:{LICENSE_SERVICE_PORT}',
                'capath': ALL_CA,
                'log_http_body': True,
            },
            'logic': {
                'vtypes': {
                    'compute': 'mdb.cloud-preprod.yandex.net',
                },
                's3_bucket_prefix': 'dataproc-infra-tests-',
                'environment_vtype': 'compute',
                'saltenvs': {
                    'production': 'qa',
                    'prestable': 'dev',
                },
                'kafka': {
                    'zk_zones': ['ru-central1-a', 'ru-central1-b', 'ru-central1-c'],
                    'sync_topics': True,
                },
                'metastore': conf['test_kubernetes'],
                'sqlserver': {
                    'product_ids': {'standard': 'dqnversiomdbstdmssql', 'enterprise': 'dqnversiomdbentmssql'}
                },
                'cloud_default_quota': {
                    'clusters': 8,
                    'cpu': 64,
                    'memory': 274877906944,
                    'io': 137438953472,
                    'network': 137438953472,
                    'ssd_space': 2199023255552,
                    'hdd_space': 1099511627776,
                    'gpu': 0,
                },
            },
            'vpc': {
                'uri': f'{VPC_GRPC_SERVICE_HOST}:{VPC_GRPC_SERVICE_PORT}',
                'token': mdb_internal_api['vpc']['token'],
            },
            'compute': {
                'uri': f'{COMPUTE_GRPC_SERVICE_HOST}:{COMPUTE_GRPC_SERVICE_PORT}',
            },
            'crypto': {
                'public_key': conf['pillar_key']['public'],
                'private_key': conf['pillar_key']['private'],
            },
            'resource_manager': {
                'addr': 'rm.private-api.cloud-preprod.yandex.net:4284',
                'capath': '/opt/yandex/allCAs.pem',
            },
            's3': {
                'host': conf['s3']['backup']['host'],
                'access_key': conf['s3']['backup']['access_key_id'],
                'secret_key': conf['s3']['backup']['access_secret_key'],
            },
        },
        'console_default_resources': {
            'kafka_cluster': {
                'kafka_cluster': {
                    'generation': 1,
                    'resource_preset_id': 's2.micro',
                    'disk_type_id': 'network-hdd',
                    'disk_size': 10737418240,
                },
                'zk': {
                    'generation': 1,
                    'resource_preset_id': 'b2.medium',
                    'disk_type_id': 'network-ssd',
                    'disk_size': 10737418240,
                },
            },
            'sqlserver_cluster': {
                'sqlserver_cluster': {
                    'generation': 2,
                    'resource_preset_id': 's2.micro',
                    'disk_type_id': 'network-ssd',
                    'disk_size': '10737418240',
                },
                'windows_witness': {
                    'generation': 2,
                    'resource_preset_id': 's2.small',
                    'disk_type_id': 'network-ssd',
                    'disk_size': 10737418240,
                },
            },
        },
        'service_account': conf['compute_driver']['service_account'],
    }


def dbaas_worker_pillar(conf, fqdn):
    metadb = conf['projects']['metadb']
    internal_api = conf['projects']['internal_api']
    worker = conf['projects']['worker']
    internal_api['url'] = f'https://{fqdn}:{internal_api["port"]}/'
    deploy_api = conf['deploy_api']
    deploy_api['url'] = f'https://{fqdn}:{deploy_api["port"]}'
    return {
        'supervisor_stopwaitsecs': 5,
        'ssh_private_key': worker['ssh']['private'],
        'config': {
            'ssh': {
                'public_key': worker['ssh']['public'],
            },
            'main': {
                'metadb_hosts': [fqdn],
                'api_sec_key': conf['pillar_key']['private'],
                'client_pub_key': conf['pillar_key']['public'],
                'ca_path': ALL_CA,
                'sentry_dsn': '',
                'sentry_environment': '',
            },
            'internal_api': {
                'access_id': metadb['access_ids']['worker'],
                'access_secret': metadb['access_secret'],
                'url': internal_api['url'],
            },
            'compute': {
                'ca_path': ALL_CA,
                'managed_network_id': 'c64vs98keiqc7f24pvkd',
                'folder_id': 'aoeb0d5hocqev4i6rmmf',
            },
            'mlock': {
                'enabled': False,
            },
            'slayer_dns': {
                'ca_path': ALL_CA,
            },
            'solomon': {
                'ca_path': ALL_CA,
                'token': 'solomon_oauth_token',
                'url': 'http://localhost:8082',
            },
            'resource_manager': {},
            'dataproc_manager': {
                'url': f'0.0.0.0:{DATAPROC_PRIVATE_PORT}',
                'server_name': f'{DATAPROC_SERVER_NAME}',
                'cert_file': ALL_CA,
                'sleep_time': 1,
                'insecure': True,
            },
            'deploy': {
                'group': deploy_api['group'],
                'version': 2,
                'url_v2': deploy_api['url'],
                'token_v2': deploy_api['token'],
            },
            'conductor': {
                'enabled': False,
            },
            'cert_api': {
                'api': 'CERTIFICATOR',
                'ca_name': 'InternalCA',
                'cert_type': 'mdb',
                'url': 'http://localhost:8083',
                'token': 'certificator_oauth_token',
            },
            'juggler': {
                "token": "juggler_oauth_token",
                "url": "http://localhost:8081",
            },
            'kafka': {
                'labels': {
                    'owner': getuser(),
                    'purpose': 'dataproc_infratest',
                    'dbtype': 'kafka',
                },
                'group_dns': [
                    {
                        'id': 'cid',
                        'pattern': 'c-{id}.mdb.cloud-preprod.yandex.net',
                    }
                ],
            },
            'greenplum_master': {
                'labels': {
                    'owner': getuser(),
                    'purpose': 'dataproc_infratest',
                    'dbtype': 'greenplum',
                },
            },
            'greenplum_segment': {
                'labels': {
                    'owner': getuser(),
                    'purpose': 'dataproc_infratest',
                    'dbtype': 'greenplum',
                },
            },
            'zookeeper': {
                'labels': {
                    'owner': getuser(),
                    'purpose': 'dataproc_infratest',
                    'dbtype': 'zookeeper',
                },
            },
            'sqlserver': {
                'compute_image_type_template': {
                    'template': 'sqlserver-{major_version}',
                    'task_arg': 'major_version',
                    'whitelist': {
                        '2016sp2std': '2016sp2std',
                        '2016sp2ent': '2016sp2ent',
                    },
                },
                'labels': {
                    'owner': getuser(),
                    'purpose': 'dataproc_infratest',
                    'dbtype': 'sqlserver',
                },
            },
            'per_cluster_service_accounts': {
                'folder_id': 'aoeb0d5hocqev4i6rmmf',
            },
            's3': {
                'backup': {
                    'endpoint_url': conf['s3']['backup']['endpoint'],
                    'access_key_id': conf['s3']['backup']['access_key_id'],
                    'secret_access_key': conf['s3']['backup']['access_secret_key'],
                }
            },
        },
    }


def dbaas_metadb_pillar(conf, fqdn):
    default_feature_flags = [
        'MDB_DATAPROC_MANAGER',
        'MDB_KAFKA_CLUSTER',
        'MDB_SQLSERVER_CLUSTER',
        'MDB_DATAPROC_UI_PROXY',
        'MDB_DATAPROC_IMAGE_1_3',
        'MDB_NETWORK_DISK_TRUNCATE',
        'MDB_LOCAL_DISK_RESIZE',
        'MDB_ALLOW_NETWORK_SSD_NONREPLICATED_RESIZE',
        'MDB_KAFKA_ALLOW_NON_HA_ON_HG',
        'MDB_KAFKA_ALLOW_UPGRADE',
        'MDB_GREENPLUM_CLUSTER',
        'MDB_GREENPLUM_ALLOW_LOW_MEM',
        'MDB_GREENPLUM_E2E_AND_DEVELOPER_FLAVORS',
        'MDB_GREENPLUM_DEPRECATED_FLAVORS',
        'MDB_FLAVORS_FOR_E2E_AND_DEVELOPERS',
        'MDB_SQLSERVER_TWO_NODE_CLUSTER',
        'MDB_METASTORE_ALPHA',
        'MDB_REDIS_62',
    ]
    default_feature_flags = [dict(flag_name=flag) for flag in default_feature_flags]
    return {
        'default_feature_flags': default_feature_flags,
        'target': 'latest',
        'cluster_type_pillars': [
            {
                'type': 'hadoop_cluster',
                'value': {
                    'data': {
                        'unmanaged': {},
                    },
                },
            },
            {
                'type': 'kafka_cluster',
                'value': {
                    'data': {
                        'salt_version': '3002.7+ds-1+yandex0',
                        'kafka': {
                            'sync_topics': True,
                        },
                    },
                },
            },
        ],
        'config_host_access_ids': config_host_access_ids(conf),
        'default_pillar': [default_pillar(conf, fqdn)],
        'enable_cleaner': False,
    }


def default_pillar(conf, fqdn):
    return {
        'id': 1,
        'value': {
            'cert.ca': conf['projects']['certificator_mock']['cert'],
            'data': {
                's3': conf['s3']['backup'],
                'external_project_ids': ['0x453e', '0xf807'],
                'linux_kernel': {
                    'version': '4.19.114-29',
                },
                'monrun': {
                    'juggler_url': 'no-juggler',
                },
                'monrun2': True,
                'database_slice': {
                    'enable': True,
                },
                'use_yasmagent': False,
                'ship_logs': True,
                'mdb_dns': {
                    'host': 'mdb-dns.private-api.cloud-preprod.yandex.net',
                },
                'mdb_int_api': {
                    'address': f'{fqdn}:{MDB_INT_API_PORT}',
                    'insecure': True,
                },
                'sysctl': {
                    'vm.nr_hugepages': 0,
                },
                'billing': {
                    'topic': 'billing-mdb-instance',
                    'ident': 'yc-mdb-pre',
                    'oauth_token': 'fake_token',
                },
                'solomon_cloud': conf['solomon_cloud'],
                'token_service': {
                    'address': conf['token_service_address'],
                },
                'dist': {
                    'bionic': {
                        'secure': True,
                    },
                    'mdb-bionic': {
                        'secure': True,
                    },
                    'pgdg': {
                        'absent': True,
                    },
                },
            },
            'mine_functions': {
                'grains.item': ['id', 'role', 'virtual'],
            },
        },
    }


def config_host_access_ids(conf):
    metadb = conf['projects']['metadb']
    return [
        {
            'access_id': metadb['access_ids']['ext_pillar'],
            'access_secret': metadb['access_hash'],
            'active': True,
            'type': 'default',
        },
        {
            'access_id': metadb['access_ids']['worker'],
            'access_secret': metadb['access_hash'],
            'active': True,
            'type': 'dbaas-worker',
        },
        {
            'access_id': metadb['access_ids']['manager'],
            'access_secret': metadb['access_hash'],
            'active': True,
            'type': 'dataproc-api',
        },
        {
            'access_id': metadb['access_ids']['dataproc-ui-proxy'],
            'access_secret': metadb['access_hash'],
            'active': True,
            'type': 'dataproc-ui-proxy',
        },
    ]


def mdb_dataproc_manager_pillar(conf, fqdn):
    metadb = conf['projects']['metadb']
    internal_api = conf['projects']['internal_api']
    internal_api['url'] = f'https://{fqdn}:{internal_api["port"]}/'
    redis_password = conf['projects']['redis']['password']
    return {
        'server': {
            'listen': f'0.0.0.0:{DATAPROC_PRIVATE_PORT}',
        },
        'config': {
            'internal-api': {
                'url': internal_api['url'],
                'capath': ALL_CA,
                'access_id': metadb['access_ids']['manager'],
                'access_secret': metadb['access_secret'],
            },
            'sentry': {
                'dsn': '',
            },
            'logging': {
                'url': conf['projects']['control_plane_logging']['url'],
                'log_group_id': conf['projects']['control_plane_logging']['log_group_id'],
            },
            'logging_service_account_key': conf['projects']['control_plane_logging']['service_account_key'],
        },
        'redis': {
            'hosts': [f'{fqdn}:26379'],
            'mastername': 'mdb-dataproc-manager-redis-infratest',
            'password': redis_password,
        },
    }


def dataproc_ui_proxy_pillar(conf, fqdn):
    common = conf['common']
    cert = common['cert']['cert']
    private_key = common['cert']['key']
    metadb = conf['projects']['metadb']
    internal_api = conf['projects']['internal_api']
    return {
        'config': {
            'user_auth': {
                'host': f'https://{fqdn}:{UI_PROXY_PORT}',
            },
            'internal_api': {
                'url': f"https://{fqdn}:{internal_api['port']}",
                'access_id': metadb['access_ids']['dataproc-ui-proxy'],
                'access_secret': metadb['access_secret'],
            },
            'http_server': {
                'listen_addr': f':{UI_PROXY_PORT}',
                'tls': {
                    'cert': cert,
                    'key': private_key,
                },
            },
        },
    }


def gateway_pillar(conf, fqdn):
    internal_api = conf['projects']['internal_api']
    mdb_internal_api = conf['projects']['mdb_internal_api']
    return {
        'cert_server_name': DATAPROC_SERVER_NAME,
        'public_port': DATAPROC_PUBLIC_PORT,
        'adapter_plaintext_port': DATAPROC_ADAPTER_PLAINTEXT_PORT,
        'go_api_plaintext_port': mdb_internal_api['port'],
        'intapi_port': internal_api['port'],
        'private_port': DATAPROC_PRIVATE_PORT,
        'envoy_public_hostname': f'{fqdn}',
        'compute_api_hostname': COMPUTE_SERVICE,
        'compute_grpc_hostname': COMPUTE_GRPC_SERVICE_HOST,
        'compute_grpc_port': COMPUTE_GRPC_SERVICE_PORT,
        'iam_grpc_api_hostname': ACCESS_SERVICE,
        'iam_grpc_api_port': ACCESS_SERVICE_PORT,
        'iam_http_api_hostname': IDENTITY_SERVICE,
        'iam_http_api_port': IDENTITY_SERVICE_PORT,
        'instance_group_api_hostname': INSTANCE_GROUP_SERVICE_HOST,
        'instance_group_api_port': INSTANCE_GROUP_SERVICE_PORT,
        'is_cache': conf['common'].get('is_cache', False),
    }


def redis_pillar(conf):
    redis_password = conf['projects']['redis']['password']
    return {
        'repair_sentinel': False,
        'config': {
            'requirepass': redis_password,
            'masterauth': redis_password,
        },
        'secrets': {
            'renames': {
                'CONFIG': 'CONFIG',
                'SLAVEOF': 'SLAVEOF',
                'DEBUG': 'DEBUG',
                'MONITOR': 'MONITOR',
                'SHUTDOWN': 'SHUTDOWN',
            },
            'sentinel_renames': {
                'FAILOVER': 'FAILOVER',
                'RESET': 'RESET',
            },
        },
    }


def docker_pillar(conf):
    common = conf['common']
    return {
        'auths': {
            'registry.yandex.net': {
                'auth': common['docker_registry']['token'],
            },
            'cr.yandex': {
                'auth': common['cr']['token'],
            },
            'cr.cloud.yandex.net': {
                'auth': common['cr']['token'],
            },
        },
    }


def saltkeys_pillar(conf, fqdn):
    deploy_api = conf['deploy_api']
    deploy_api['url'] = f'https://{fqdn}:{deploy_api["port"]}'
    saltapi_password = conf['projects']['saltapi']['password']
    return {
        'config': {
            'deploy_api_url': deploy_api['url'],
            'deploy_api_token': deploy_api['token'],
            'saltapi': {
                'password': saltapi_password,
            },
        },
    }


def mdb_deploy_api_pillar(conf):
    common = conf['common']
    cert = common['cert']['cert']
    private_key = common['cert']['key']
    deploy_api = conf['deploy_api']
    deploydb = conf['projects']['deploydb']
    saltapi_password = conf['projects']['saltapi']['password']
    saltmaster_tls = conf['projects']['salt_master']['tls']
    return {
        'port': deploy_api['port'],
        'tls_key': private_key,
        'tls_crt': cert,
        'config': {
            'app': {
                'instrumentation': {
                    'addr': '[::1]:6061',
                },
            },
            'deployapi': {
                'saltapi': {
                    'password': saltapi_password,
                },
                'master_default_public_key': saltmaster_tls['public'],
            },
            'deploydb': {
                'postgresql': {
                    'hosts': [f'{deploydb["host"]}:{deploydb["port"]}'],
                    'db': deploydb['dbname'],
                    'user': deploydb['user'],
                    'password': deploydb['password'],
                },
            },
        },
        'blackbox_uri': 'http://localhost:8080',
        'user_scopes': ['mdbdeployapi:read'],
    }


def tvmtool_pillar(conf):
    tvmtool = conf['projects']['tvmtool']
    return {
        'tvm_id': int(tvmtool['id']),
        'token': tvmtool['token'],
        'config': {
            'client': 'mdb_infrastructure_tests',
            'secret': tvmtool['secret'],
        },
    }


def dbaas_pillar(conf, fqdn):
    metadb = conf['projects']['metadb']
    internal_api = conf['projects']['internal_api']
    return {
        'urls': [f"https://{fqdn}:{internal_api['port']}/api/v1.0/config/"],
        'api_pub_key': conf['pillar_key']['public'],
        'salt_sec_key': conf['pillar_key']['private'],
        'access_id': metadb['access_ids']['ext_pillar'],
        'access_secret': metadb['access_secret'],
    }


def mdb_secrets_pillar(conf):
    mdb_secrets = conf['projects']['mdb_secrets']
    return {
        'oauth': mdb_secrets['oauth'],
        'public_key': mdb_secrets['public'],
        'salt_private_key': mdb_secrets['private'],
        'sa_id': '',
    }


def blackbox_pillar(conf):
    deploy_api = conf['deploy_api']
    return {
        'tokens': [
            {
                'token': deploy_api['token'],
                'login': 'robot-pgaas-deploy',
                'scope': 'mdbdeployapi:read',
            }
        ],
    }


def mdb_maintenance_pillar(conf):
    return {
        'tasks': {'configs_dir': 'configs'},
        'metadb': {'password': conf['projects']['metadb']['password']},
        'maintainer': {'newly_planned_limit': 1000, 'newly_planned_clouds': 20},
        'notifier': {'endpoint': '', 'transport': {'logging': {}}},
        'ca_dir': '/opt/yandex/',
        'holidays': {'enabled': False},
        'prometheus': {
            'url': 'any-non-empty-string',
        },
    }


def _get_user_pillar_override():
    user_pillar_path = os.path.expanduser('~/.dataproc-user-pillar.yaml')
    if os.getenv('USER_PILLAR_PATH'):
        user_pillar_path = os.getenv('USER_PILLAR_PATH')
    try:
        if os.path.exists(user_pillar_path):
            with open(user_pillar_path) as user_pillar:
                return yaml.safe_load(user_pillar)
    except Exception as exc:
        print('Exception while loading user pillar: {exc}'.format(exc=repr(exc)))
