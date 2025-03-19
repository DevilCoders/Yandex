"""
Mocks for worker tests
"""

import collections.abc
from copy import deepcopy
from dateutil import parser as dt_parser
from queue import Queue
from types import SimpleNamespace

import jsonschema
from dbaas_common.dict import combine_dict
from httmock import HTTMock, all_requests

from cloud.mdb.dbaas_worker.internal.config import CONFIG_SCHEMA, DEFAULT_CONFIG
from cloud.mdb.dbaas_worker.internal.runners import TaskRunner

from .certificator import certificator
from .clickhouse.backup_key import ch_backup_key
from .compute import compute
from .conductor import conductor
from .dataproc_manager import dataproc_manager
from .dbm import dbm
from .deploy_v2 import deploy_v2
from .dns import dns
from .dns_resolver import dns_resolver
from .eds import eds
from .gpg import gpg
from .loadbalancer import loadbalancer
from .lockbox import lockbox
from .iam import iam
from .iam_jwt import iam_jwt
from .instance_group import instance_group
from .internal_api import internal_api
from .juggler import juggler
from .kubernetes_master import kubernetes_master
from .managed_kubernetes import managed_kubernetes
from .managed_postgresql import managed_postgresql
from .mdb_secrets import mdb_secrets
from .metadb import metadb
from .mlock import mlock
from .mock_disk_placement_group import disk_placement_groups
from .ssh_key import ssh_key
from .mysync import mysync, mysync_react_maintenance, mysync_react_switchover
from .nacl import nacl
from .pgsync import pgsync_react_delete_timeline, pgsync_react_switchover
from .resource_manager import resource_manager
from .s3 import s3
from .solomon import solomon
from .sqlserver_repl_cert import sqlserver_repl_cert
from .ssh import ssh
from .vpc import vpc
from .zookeeper import zookeeper
from .mock_placement_group import placement_groups

INTERNAL_API_HOST = 'internal-api.test'

DEFAULT_STATE = {
    # We preserve actions order to simplify debug (so we store them twice in 'actions' and in 'set_actions')
    'actions': [],
    'set_actions': set(),
    'cancel_actions': set(),
    'fail_actions': set(),
    'certificator': {},
    'mdb_secrets': {},
    'compute': {
        'disk_placement_groups': {},
        'placement_groups': {},
        'instances': {},
        'operations': {},
        'disks': {},
        'images': {
            'postgresql-image': {
                'description': 'dbaas-postgresql-image-1',
                'id': 'postgresql-image',
                'status': 'READY',
                'folderId': 'compute-folder-id',
                'minDiskSize': 10737418240,
                'createdAt': '1970-01-01T00:00:00+00:00',
            },
            'clickhouse-image': {
                'description': 'dbaas-clickhouse-image-1',
                'id': 'clickhouse-image',
                'status': 'READY',
                'folderId': 'compute-folder-id',
                'minDiskSize': 10737418240,
                'createdAt': '1970-01-01T00:00:00+00:00',
            },
            'zookeeper-image': {
                'description': 'dbaas-zookeeper-image-1',
                'id': 'zookeeper-image',
                'status': 'READY',
                'folderId': 'compute-folder-id',
                'minDiskSize': 10737418240,
                'createdAt': '1970-01-01T00:00:00+00:00',
            },
            'kafka-image': {
                'description': 'dbaas-kafka-image-1',
                'id': 'kafka-image',
                'status': 'READY',
                'folderId': 'compute-folder-id',
                'minDiskSize': 10737418240,
                'createdAt': '1970-01-01T00:00:00+00:00',
            },
            'mongodb-image': {
                'description': 'dbaas-mongodb-image-1',
                'id': 'mongodb-image',
                'status': 'READY',
                'folderId': 'compute-folder-id',
                'minDiskSize': 10737418240,
                'createdAt': '1970-01-01T00:00:00+00:00',
            },
            'mysql-image': {
                'description': 'dbaas-mysql-image-1',
                'id': 'mysql-image',
                'status': 'READY',
                'folderId': 'compute-folder-id',
                'minDiskSize': 10737418240,
                'createdAt': '1970-01-01T00:00:00+00:00',
            },
            'redis-image': {
                'description': 'dbaas-redis-image-1',
                'id': 'redis-image',
                'status': 'READY',
                'folderId': 'compute-folder-id',
                'minDiskSize': 10737418240,
                'createdAt': '1970-01-01T00:00:00+00:00',
            },
            'sqlserver-image': {
                'description': 'dbaas-sqlserver-image-1',
                'id': 'sqlserver-image',
                'status': 'READY',
                'folderId': 'compute-folder-id',
                'minDiskSize': 10737418240,
                'createdAt': '1970-01-01T00:00:00+00:00',
            },
            'elasticsearch-image': {
                'description': 'dbaas-elasticsearch-image-1',
                'id': 'elasticsearch-image',
                'status': 'READY',
                'folderId': 'compute-folder-id',
                'minDiskSize': 10737418240,
                'createdAt': '1970-01-01T00:00:00+00:00',
            },
            'opensearch-image': {
                'description': 'dbaas-opensearch-image-1',
                'id': 'opensearch-image',
                'status': 'READY',
                'folderId': 'compute-folder-id',
                'minDiskSize': 10737418240,
                'createdAt': '1970-01-01T00:00:00+00:00',
            },
            'greenplum-image': {
                'description': 'dbaas-greenplum-image-1',
                'id': 'greenplum-image',
                'status': 'READY',
                'folderId': 'compute-folder-id',
                'minDiskSize': 21474836480,
                'createdAt': '1970-01-01T00:00:00+00:00',
            },
            'hadoop-image': {
                'description': 'dbaas-hadoop-image-1',
                'id': 'hadoop-image',
                'status': 'READY',
                'folderId': 'user-compute-folder-id',
                'minDiskSize': 10737418240,
                'createdAt': '1970-01-01T00:00:00+00:00',
            },
            'windows_witness-image': {
                'description': 'dbaas-windows-witness-image-1',
                'id': 'windows-witness-image',
                'status': 'READY',
                'folderId': 'compute-folder-id',
                'minDiskSize': 10737418240,
                'createdAt': '1970-01-01T00:00:00+00:00',
            },
        },
        'networks': {
            'test-net': {
                'id': 'test-net',
                'folderId': 'compute-folder-id',
                'defaultSecurityGroupId': 'default-sg-id',
            },
        },
        'subnets': {
            'test-subnet': {
                'id': 'test-subnet',
                'zoneId': 'geo1',
                'v4CidrBlock': ['192.168.0.0/24'],
                'folderId': 'compute-folder-id',
                'networkId': 'test-net',
            },
            'test-compute-subnet-id-1': {
                'id': 'test-compute-subnet-id-1',
                'zoneId': 'geo1',
                'v4CidrBlock': ['192.168.0.0/24'],
                'folderId': 'compute-folder-id',
                'networkId': 'test-net',
            },
            'test-compute-subnet-id-2': {
                'id': 'test-compute-subnet-id-2',
                'zoneId': 'geo1',
                'v4CidrBlock': ['192.168.0.0/24'],
                'folderId': 'compute-folder-id',
                'networkId': 'test-net',
            },
            'managed-geo1': {
                'id': 'managed-geo1',
                'zoneId': 'geo1',
                'v6CidrBlock': ['2001:db8:1::/96'],
                'folderId': 'compute-folder-id',
                'networkId': 'compute-managed-network-id',
            },
            'managed-geo2': {
                'id': 'managed-geo2',
                'zoneId': 'geo2',
                'v6CidrBlock': ['2001:db8:2::/96'],
                'folderId': 'compute-folder-id',
                'networkId': 'compute-managed-network-id',
            },
            'managed-geo3': {
                'id': 'managed-geo3',
                'zoneId': 'geo3',
                'v6CidrBlock': ['2001:db8:3::/96'],
                'folderId': 'compute-folder-id',
                'networkId': 'compute-managed-network-id',
            },
        },
    },
    'conductor': {
        'groups': {
            'mdb_test_clickhouse': {
                'id': 1,
                'name': 'mdb_test_clickhouse',
                'parent_ids': [],
                'project': {
                    'id': 1,
                },
            },
            'mdb_test_postgresql': {
                'id': 2,
                'name': 'mdb_test_postgresql',
                'parent_ids': [],
                'project': {
                    'id': 1,
                },
            },
            'mdb_test_zookeeper': {
                'id': 3,
                'name': 'mdb_test_zookeeper',
                'parent_ids': [],
                'project': {
                    'id': 1,
                },
            },
            'mdb_test_mongod': {
                'id': 4,
                'name': 'mdb_test_mongod',
                'parent_ids': [],
                'project': {
                    'id': 1,
                },
            },
            'mdb_test_mongos': {
                'id': 5,
                'name': 'mdb_test_mongos',
                'parent_ids': [],
                'project': {
                    'id': 1,
                },
            },
            'mdb_test_mongocfg': {
                'id': 6,
                'name': 'mdb_test_mongocfg',
                'parent_ids': [],
                'project': {
                    'id': 1,
                },
            },
            'mdb_test_redis': {
                'id': 7,
                'name': 'mdb_test_redis',
                'parent_ids': [],
                'project': {
                    'id': 1,
                },
            },
            'mdb_test_mysql': {
                'id': 8,
                'name': 'mdb_test_mysql',
                'parent_ids': [],
                'project': {
                    'id': 1,
                },
            },
            'mdb_test_sqlserver': {
                'id': 9,
                'name': 'mdb_test_sqlserver',
                'parent_ids': [],
                'project': {
                    'id': 1,
                },
            },
            'mdb_test_kafka': {
                'id': 10,
                'name': 'mdb_test_kafka',
                'parent_ids': [],
                'project': {
                    'id': 1,
                },
            },
            'mdb_test_elasticsearch_data': {
                'id': 11,
                'name': 'mdb_test_elasticsearch_data',
                'parent_ids': [],
                'project': {
                    'id': 1,
                },
            },
            'mdb_test_elasticsearch_master': {
                'id': 12,
                'name': 'mdb_test_elasticsearch_master',
                'parent_ids': [],
                'project': {
                    'id': 1,
                },
            },
            'mdb_test_mongoinfra': {
                'id': 13,
                'name': 'mdb_test_mongoinfra',
                'parent_ids': [],
                'project': {
                    'id': 1,
                },
            },
            'mdb_test_greenplum_master': {
                'id': 14,
                'name': 'mdb_test_greenplum_master',
                'parent_ids': [],
                'project': {
                    'id': 1,
                },
            },
            'mdb_test_greenplum_segment': {
                'id': 15,
                'name': 'mdb_test_greenplum_segment',
                'parent_ids': [],
                'project': {
                    'id': 1,
                },
            },
            'mdb_test_altgroup': {
                'id': 9000,
                'name': 'mdb_test_altgroup',
                'parent_ids': [],
                'project': {
                    'id': 1,
                },
            },
            'mdb_test_windows_witness': {
                'id': 16,
                'name': 'mdb_test_altgroup',
                'parent_ids': [],
                'project': {
                    'id': 1,
                },
            },
            'mdb_test_opensearch_data': {
                'id': 17,
                'name': 'mdb_test_opensearch_data',
                'parent_ids': [],
                'project': {
                    'id': 1,
                },
            },
            'mdb_test_opensearch_master': {
                'id': 18,
                'name': 'mdb_test_opensearch_master',
                'parent_ids': [],
                'project': {
                    'id': 1,
                },
            },
        },
        'group2hosts': {
            'mdb_api_admin_compute_preprod': {
                "type": "host[]",
                "size": 1,
                "page": 1,
                "pages": 1,
                "value": [
                    {
                        "type": "host",
                        "id": 6519135,
                        "self": "/hosts/6519135",
                        "web_path": "/hosts/6519135",
                        "value": {
                            "fqdn": INTERNAL_API_HOST,
                            "short_name": "api-admin-dbaas-preprod01k",
                            "description": None,
                            "created_at": "2018-08-22T17:07:37+03:00",
                            "updated_at": "2018-08-22T17:07:37+03:00",
                            "group": {
                                "type": "group",
                                "id": 45341,
                                "self": "/groups/45341",
                                "web_path": "/groups/mdb_api_admin_compute_preprod",
                            },
                            "datacenter": {
                                "type": "datacenter",
                                "id": 138,
                                "self": "/datacenters/138",
                                "web_path": "/datacenters/vla",
                            },
                            "tags": {"type": "tag[]", "self": "/hosts/6519135/tags"},
                        },
                    }
                ],
            },
        },
        'dcs': {
            'geo1': 1,
            'geo2': 2,
            'geo3': 3,
        },
        'hosts': {},
    },
    'eds': {'dns_cname_records': {}},
    'dbm': {
        'transfers': {},
        'containers': {},
    },
    'deploy-v1': {},
    'deploy-v2': {
        'record_all_shipments': False,
        'minions': {},
        'shipments': {},
    },
    'dns': {},
    'metadb': {
        'queries': [
            {
                'query': 'get_alerts_by_cid',
                'kwargs': {'cid': 'cid-user-sg-test'},
                'result': [
                    {
                        'alert_group_id': '1',
                        'folder_ext_id': 'compute-folder-id',
                        'name': 'name1',
                        'warning_threshold': '1',
                        'critical_threshold': '1',
                        'status': 'DELETING',
                        'notification_channels': [],
                        'description': 'd1',
                        'alert_ext_id': 'test-deletion-id',
                        'template_id': '2',
                        'monitoring_folder_id': 'solomon-project',
                        'template_version': 'test-ver-1',
                    },
                    {
                        'alert_group_id': '1',
                        'folder_ext_id': 'compute-folder-id',
                        'name': 'name2',
                        'warning_threshold': '1',
                        'critical_threshold': '1',
                        'status': 'CREATING',
                        'notification_channels': [],
                        'description': 'd1',
                        'alert_ext_id': '',
                        'template_id': '1',
                        'monitoring_folder_id': 'solomon-project',
                        'template_version': 'test-ver-1',
                    },
                ],
            },
            {
                'query': 'alert_delete_by_id',
                'kwargs': {'alert_group_id': '1', 'template_ids': ['2', '1']},
                'result': [],
            },
            {
                'query': 'alert_delete_by_id',
                'kwargs': {'alert_group_id': '1', 'template_ids': ['2']},
                'result': [],
            },
            {
                'query': 'set_alert_active',
                'kwargs': {'group_id': '1', 'template_id': '1', 'ext_id': '3'},
                'result': [],
            },
            {
                'query': 'set_alert_active',
                'kwargs': {'group_id': '1', 'template_id': '1', 'ext_id': '2'},
                'result': [],
            },
            {
                'query': 'set_alert_active',
                'kwargs': {'group_id': '1', 'template_id': '1', 'ext_id': '1'},
                'result': [],
            },
            {
                'query': 'get_alerts_by_cid',
                'kwargs': {'cid': 'cid-test'},
                'result': [
                    {
                        'alert_group_id': '1',
                        'name': 'name1',
                        'folder_ext_id': 'compute-folder-id',
                        'warning_threshold': '1',
                        'critical_threshold': '1',
                        'status': 'DELETING',
                        'notification_channels': [],
                        'description': 'd1',
                        'alert_ext_id': 'test-deleting-id',
                        'template_id': '2',
                        'monitoring_folder_id': 'solomon-project',
                        'template_version': 'test-ver-1',
                    },
                    {
                        'alert_group_id': '1',
                        'name': 'name2',
                        'folder_ext_id': 'compute-folder-id',
                        'warning_threshold': '1',
                        'critical_threshold': '1',
                        'status': 'CREATING',
                        'notification_channels': [],
                        'description': 'd1',
                        'alert_ext_id': '1',
                        'template_id': '1',
                        'monitoring_folder_id': 'solomon-project',
                        'template_version': 'test-ver-1',
                    },
                ],
            },
            {
                'query': 'get_reject_rev',
                'result': [{'rev': 1}],
            },
            {
                'query': 'get_version',
                'kwargs': {
                    'cid': 'cid-test',
                    'component': 'postgres',
                },
                'result': [
                    {
                        'package_version': '13.3-201-yandex.50027.438e1ff1be',
                        'cid': 'cid',
                        'subcid': None,
                        'shard_id': None,
                        'component': 'postgres',
                        'major_version': 'major_version',
                        'minor_version': 'minor_version',
                        'edition': 'default',
                        'pinned': False,
                    }
                ],
            },
            {
                'query': 'get_version',
                'kwargs': {
                    'cid': 'cid-test',
                    'component': 'redis',
                },
                'result': [
                    {
                        'package_version': '6.2.3.1-7e55a63d-yandex--ssl',
                        'cid': 'cid',
                        'subcid': None,
                        'shard_id': None,
                        'component': 'redis',
                        'major_version': 'major_version',
                        'minor_version': 'minor_version',
                        'edition': 'default',
                        'pinned': False,
                    }
                ],
            },
            {
                'query': 'get_version',
                'kwargs': {
                    'cid': 'cid-test',
                    'component': 'mysql',
                },
                'result': [
                    {
                        'package_version': '8.0.25-8-1.bionic+yandex1',
                        'cid': 'cid',
                        'subcid': None,
                        'shard_id': None,
                        'component': 'mysql',
                        'major_version': '8.0',
                        'minor_version': '8.0.17',
                        'edition': 'default',
                        'pinned': False,
                    }
                ],
            },
            {
                'query': 'get_major_version',
                'kwargs': {
                    'cid': 'cid-test',
                    'component': 'postgres',
                },
                'result': [{'major_version': '13'}],
            },
            {
                'query': 'get_major_version',
                'kwargs': {
                    'cid': 'cid-test',
                    'component': 'redis',
                },
                'result': [{'major_version': '6.2'}],
            },
            {
                'query': 'get_major_version',
                'kwargs': {
                    'cid': 'cid-user-sg-test',
                    'component': 'postgres',
                },
                'result': [{'major_version': '13'}],
            },
            {
                'query': 'get_pillar',
                'result': [],
            },
            {
                'query': 'upsert_pillar',
                'result': [],
            },
            {
                'query': 'update_host_vtype_id',
                'result': [],
            },
            {
                'query': 'update_host_subnet_id',
                'result': [],
            },
            {
                'query': 'lock_cluster',
                'result': [{'rev': 2}],
            },
            {
                'query': 'update_instance_group',
                'result': [],
            },
            {
                'query': 'update_kubernetes_node_group',
                'result': [],
            },
            {
                'query': 'get_kubernetes_node_group',
                'result': [],
            },
            {
                'query': 'get_instance_group',
                'result': [],
            },
            {
                'query': 'get_host_info',
                'kwargs': {
                    'fqdn': 'host1',
                },
                'result': [
                    {
                        'cid': 'cid1',
                        'cluster_type': 'cluster_type_1',
                        'vtype_id': 'instance_id_1',
                        'pg_id': 'placement_group_id_1',
                        'local_id': -1,
                        'placement_group_id': 'placementgroup--1',
                    }
                ],
            },
            {
                'query': 'get_host_info',
                'kwargs': {
                    'fqdn': 'host2',
                },
                'result': [
                    {
                        'cid': 'cid1',
                        'cluster_type': 'cluster_type_1',
                        'vtype_id': 'instance_id_1',
                        'pg_id': 'placement_group_id_1',
                        'local_id': -1,
                        'placement_group_id': 'placementgroup--1',
                    }
                ],
            },
            {
                'query': 'get_host_info',
                'kwargs': {
                    'fqdn': 'host3',
                },
                'result': [
                    {
                        'cid': 'cid1',
                        'cluster_type': 'cluster_type_1',
                        'vtype_id': 'instance_id_1',
                        'pg_id': 'placement_group_id_1',
                        'local_id': 0,
                        'placement_group_id': 'placementgroup-0',
                    }
                ],
            },
            {
                'query': 'get_host_info',
                'kwargs': {
                    'fqdn': 'host4',
                },
                'result': [
                    {
                        'cid': 'cid1',
                        'cluster_type': 'cluster_type_1',
                        'vtype_id': 'instance_id_1',
                        'pg_id': 'placement_group_id_1',
                        'local_id': 0,
                        'placement_group_id': 'placementgroup-0',
                    }
                ],
            },
            {
                'query': 'get_host_info',
                'kwargs': {
                    'fqdn': 'host5',
                },
                'result': [
                    {
                        'cid': 'cid1',
                        'cluster_type': 'cluster_type_1',
                        'vtype_id': 'instance_id_1',
                        'pg_id': 'placement_group_id_1',
                        'local_id': 1,
                        'placement_group_id': 'placementgroup-1',
                    }
                ],
            },
            {
                'query': 'get_host_info',
                'result': [
                    {
                        'cid': 'cid1',
                        'cluster_type': 'cluster_type_1',
                        'vtype_id': 'instance_id_1',
                        'pg_id': 'placement_group_id_1',
                        'local_id': None,
                        'placement_group_id': None,
                    }
                ],
            },
            {
                'query': 'get_sgroups_info',
                'kwargs': {'cid': 'cid-test'},
                'result': [{'network_id': 'test-net', 'sgroups': [], 'sg_type': None}],
            },
            {
                'query': 'add_sgroups',
                'kwargs': {
                    'cid': 'cid-test',
                    'sg_type': 'service',
                    'sg_ext_ids': 'sg_id_service_cid_cid-test',
                },
                'result': [],
            },
            {
                'query': 'get_sgroups_info',
                'kwargs': {'cid': 'cid-user-sg-test'},
                'result': [{'network_id': 'test-net', 'sgroups': [], 'sg_type': None}],
            },
            {
                'query': 'add_sgroups',
                'kwargs': {
                    'cid': 'cid-user-sg-test',
                    'sg_type': 'service',
                    'sg_ext_ids': 'sg_id_service_cid_cid-user-sg-test',
                },
                'result': [],
            },
            {
                'query': 'add_sgroups',
                'kwargs': {
                    'cid': 'cid-user-sg-test',
                    'sg_type': 'user',
                    'sg_ext_ids': 'user-sg1,user-sg2',
                },
                'result': [],
            },
            {
                'query': 'delete_sgroups',
                'kwargs': {'cid': 'cid-test', 'type': 'user'},
                'result': [],
            },
            {
                'query': 'complete_cluster_change',
                'kwargs': {'rev': 2},
                'result': [],
            },
            {
                'query': 'get_backup_info',
                'kwargs': {'backup_id': 1},
                'result': [{'status': 'DONE', 'metadata': {'name': 'backupName'}}],
            },
            {
                'query': 'get_backup_info',
                'kwargs': {'backup_id': '1'},
                'result': [{'status': 'DONE', 'metadata': {'name': 'backupName'}}],
            },
            {
                'query': 'get_backup_info',
                'kwargs': {'backup_id': 2},
                'result': [{'status': 'DELETED', 'metadata': {'name': 'backupName'}}],
            },
            {
                'query': 'mark_cluster_backups_deleted',
                'kwargs': {'cid': 'cid-test'},
                'result': [],
            },
            {
                'query': 'schedule_initial_backup',
                'kwargs': {'cid': 'cid-test', 'subcid': 'subcid-test', 'shard_id': None, 'backup_id': 'backup_id-test'},
                'result': [],
            },
            {'query': 'get_cluster_project_id', 'kwargs': {'cid': 'cid-test'}, 'result': []},
            {
                'query': 'get_disks_info',
                'kwargs': {'fqdn': 'host1'},
                'result': [{'local_id': 0, 'mount_point': 'mp1'}],
            },
            {
                'query': 'get_disks_info',
                'kwargs': {'fqdn': 'host2'},
                'result': [{'local_id': 0, 'mount_point': 'mp1'}],
            },
            {
                'query': 'get_disks_info',
                'kwargs': {'fqdn': 'host3'},
                'result': [{'local_id': 1, 'mount_point': 'mp1'}],
            },
            {
                'query': 'get_disks_info',
                'kwargs': {'fqdn': 'host4'},
                'result': [{'local_id': 1, 'mount_point': 'mp1'}],
            },
            {
                'query': 'get_disks_info',
                'kwargs': {'fqdn': 'host5'},
                'result': [{'local_id': 0, 'mount_point': 'mp1'}],
            },
            {
                'query': 'get_disks_info',
                'kwargs': {'fqdn': 'foo.mdb.yacloud.net'},
                'result': [{'local_id': 0, 'mount_point': 'mp1'}],
            },
            {
                'query': 'get_disks_info',
                'kwargs': {'fqdn': 'bar.mdb.yacloud.net'},
                'result': [{'local_id': 0, 'mount_point': 'mp1'}],
            },
            {
                'query': 'get_disks_info',
                'kwargs': {'fqdn': 'baz.mdb.yacloud.net'},
                'result': [{'local_id': 0, 'mount_point': 'mp1'}],
            },
            {
                'query': 'get_disks_info',
                'kwargs': {'fqdn': 'bee.mdb.yacloud.net'},
                'result': [{'local_id': 0, 'mount_point': 'mp1'}],
            },
            {
                'query': 'update_disk_placement_group',
                'kwargs': {
                    'cid': 'cid-test',
                    'disk_placement_group_id': 'diskplacementgroup-1',
                    'local_id': 0,
                    'status': 'COMPLETE',
                },
                'result': [],
            },
            {
                'query': 'update_disk_placement_group',
                'kwargs': {
                    'cid': 'cid-test',
                    'disk_placement_group_id': 'diskplacementgroup-2',
                    'local_id': 1,
                    'status': 'COMPLETE',
                },
                'result': [],
            },
            {
                'query': 'update_disk_placement_group',
                'kwargs': {
                    'cid': 'cid-test',
                    'disk_placement_group_id': 'diskplacementgroup-2',
                    'local_id': 0,
                    'status': 'COMPLETE',
                },
                'result': [],
            },
            {
                'query': 'update_disk_placement_group',
                'kwargs': {
                    'cid': 'cid-test',
                    'disk_placement_group_id': 'diskplacementgroup-3',
                    'status': 'COMPLETE',
                },
                'result': [],
            },
            {
                'query': 'update_disk_placement_group',
                'kwargs': {
                    'cid': 'cid-test',
                    'disk_placement_group_id': 'diskplacementgroup-4',
                    'status': 'COMPLETE',
                },
                'result': [],
            },
            {
                'query': 'update_disk_placement_group',
                'kwargs': {
                    'cid': 'cid-test',
                    'disk_placement_group_id': 'diskplacementgroup-5',
                    'status': 'COMPLETE',
                },
                'result': [],
            },
            {
                'query': 'update_placement_group',
                'kwargs': {
                    'cid': 'cid-test',
                    'placement_group_id': 'placementgroup-1',
                    'status': 'COMPLETE',
                },
                'result': [],
            },
            {
                'query': 'update_placement_group',
                'kwargs': {
                    'cid': 'cid-test',
                    'placement_group_id': 'placementgroup--1',
                    'status': 'DELETED',
                },
                'result': [],
            },
            {
                'query': 'update_placement_group',
                'kwargs': {
                    'cid': 'cid-test',
                    'placement_group_id': 'placementgroup-0',
                    'status': 'DELETED',
                },
                'result': [],
            },
            {
                'query': 'update_placement_group',
                'kwargs': {
                    'cid': 'cid-test',
                    'placement_group_id': 'placementgroup-1',
                    'status': 'DELETED',
                },
                'result': [],
            },
            {
                'query': 'update_placement_group',
                'kwargs': {
                    'cid': 'cid-test',
                    'placement_group_id': 'placementgroup-2',
                    'status': 'COMPLETE',
                },
                'result': [],
            },
            {
                'query': 'update_placement_group',
                'kwargs': {
                    'cid': 'cid-test',
                    'placement_group_id': 'placementgroup-3',
                    'status': 'COMPLETE',
                },
                'result': [],
            },
            {
                'query': 'update_placement_group',
                'kwargs': {
                    'cid': 'cid-test',
                    'placement_group_id': 'placementgroup-4',
                    'status': 'COMPLETE',
                },
                'result': [],
            },
            {
                'query': 'update_placement_group',
                'kwargs': {
                    'cid': 'cid-test',
                    'placement_group_id': 'placementgroup-5',
                    'status': 'COMPLETE',
                },
                'result': [],
            },
            {
                'query': 'update_disk',
                'kwargs': {
                    'cid': 'cid-test',
                    'fqdn': 'host1',
                    'local_id': 0,
                    'status': 'COMPLETE',
                    'disk_id': 'disk-2',
                    'host_disk_id': 'disk-2',
                    'mount_point': 'mp1',
                },
                'result': [],
            },
            {
                'query': 'update_disk',
                'kwargs': {
                    'cid': 'cid-test',
                    'fqdn': 'host2',
                    'local_id': 0,
                    'status': 'COMPLETE',
                    'disk_id': 'disk-4',
                    'host_disk_id': 'disk-4',
                    'mount_point': 'mp1',
                },
                'result': [],
            },
            {
                'query': 'update_disk',
                'kwargs': {
                    'cid': 'cid-test',
                    'fqdn': 'host3',
                    'local_id': 1,
                    'status': 'COMPLETE',
                    'disk_id': 'disk-6',
                    'host_disk_id': 'disk-6',
                    'mount_point': 'mp1',
                },
                'result': [],
            },
            {
                'query': 'update_disk',
                'kwargs': {
                    'cid': 'cid-test',
                    'fqdn': 'host4',
                    'local_id': 1,
                    'status': 'COMPLETE',
                    'disk_id': 'disk-8',
                    'host_disk_id': 'disk-8',
                    'mount_point': 'mp1',
                },
                'result': [],
            },
            {
                'query': 'update_disk',
                'kwargs': {
                    'cid': 'cid-test',
                    'fqdn': 'host5',
                    'local_id': 0,
                    'status': 'COMPLETE',
                    'disk_id': 'disk-10',
                    'host_disk_id': 'disk-10',
                    'mount_point': 'mp1',
                },
                'result': [],
            },
            {
                'query': 'update_disk',
                'kwargs': {
                    'cid': 'cid-test',
                    'fqdn': 'foo.mdb.yacloud.net',
                    'local_id': 0,
                    'status': 'COMPLETE',
                    'mount_point': 'mp1',
                },
                'result': [],
            },
            {
                'query': 'update_disk',
                'kwargs': {
                    'cid': 'cid-test',
                    'fqdn': 'bar.mdb.yacloud.net',
                    'local_id': 0,
                    'status': 'COMPLETE',
                    'mount_point': 'mp1',
                },
                'result': [],
            },
            {
                'query': 'update_disk',
                'kwargs': {
                    'cid': 'cid-test',
                    'fqdn': 'baz.mdb.yacloud.net',
                    'local_id': 0,
                    'status': 'COMPLETE',
                    'mount_point': 'mp1',
                },
                'result': [],
            },
            {
                'query': 'update_disk',
                'kwargs': {
                    'cid': 'cid-test',
                    'fqdn': 'bee.mdb.yacloud.net',
                    'local_id': 0,
                    'status': 'COMPLETE',
                    'mount_point': 'mp1',
                },
                'result': [],
            },
        ],
    },
    'juggler': {},
    'mlock': {},
    'internal-api-config': {'backups': {}},
    'resource-manager': {},
    'salt-file': {},
    'solomon': {
        'calls_to_solomon_api': 0,
        'alerts': [],
    },
    'zookeeper': {
        'reactions': [
            mysync_react_maintenance,
            mysync_react_switchover,
            pgsync_react_switchover,
            pgsync_react_delete_timeline,
        ],
    },
    's3': {},
    'dataproc-manager': {},
    'ssh': {},
    'postgresql': {},
}


@all_requests
def default_http_mock(*args, **kwargs):
    """
    Fail all http requests with 500
    This mock helps in detection of not-mocked http services
    """
    return {'status_code': 500, 'content': f'Called with args: {args} kwargs: {kwargs}'}


def get_http_mocks(state):
    """
    Initialize all http mocks
    """
    mocks = list()
    for init_function in [
        certificator,
        conductor,
        dbm,
        deploy_v2,
        dns,
        internal_api,
        juggler,
        mdb_secrets,
        resource_manager,
        solomon,
    ]:
        mocks.extend(init_function(state))

    return mocks


def setup_mocks(mocker, state):
    """
    Setup all non-http mocks
    """
    mocker.patch('signal.signal')
    mocker.patch('signal.alarm')
    mocker.patch('setproctitle.setproctitle')

    retry_stop = mocker.patch('tenacity.BaseRetrying.stop')
    retry_stop.side_effect = lambda *args, **kwargs: True

    for mock in [
        ch_backup_key,
        dns_resolver,
        dataproc_manager,
        gpg,
        instance_group,
        iam,
        iam_jwt,
        loadbalancer,
        lockbox,
        kubernetes_master,
        managed_kubernetes,
        managed_postgresql,
        metadb,
        mlock,
        ssh_key,
        mysync,
        nacl,
        s3,
        ssh,
        sqlserver_repl_cert,
        vpc,
        zookeeper,
        compute,
        disk_placement_groups,
        placement_groups,
        eds,
    ]:
        mock(mocker, state)


def _get_config():
    """
    Get config with all providers turned to mocks
    """
    config = combine_dict(
        deepcopy(DEFAULT_CONFIG),
        {
            'environment_name': 'compute',
            'main': {
                'admin_api_conductor_group': 'mdb_api_admin_compute_preprod',
                'api_sec_key': 'secret-key',
                'client_pub_key': 'public-key',
                'ca_path': '/nonexistent',
                'metadb_dsn': 'metadb-dsn',
                'metadb_hosts': [
                    'metadb-host',
                ],
            },
            'cert_api': {
                'token': 'certificator-token',
                'url': 'http://certificator.test',
            },
            'conductor': {
                'token': 'conductor-token',
                'url': 'http://conductor.test',
            },
            'eds': {
                'enabled': True,
                'zone_id': 'test_zone_id',
                'eds_endpoint': 'test-eds-endpoint.yandex.net:18000',
                'dns_endpoint': 'test-dns-edpoint.yandex.net:443',
                'cert_file': '/nonexistent',
            },
            'dbm': {
                'token': 'dbm-token',
                'url': 'http://dbm.test',
            },
            'slayer_dns': {
                'ca_path': '/nonexistent',
                'token': 'dns-token',
                'url': 'http://dns.test',
            },
            'deploy': {
                'version': 2,
                'url_v2': 'http://deploy-v2.test',
                'token_v2': 'deploy-v2-token',
                'group': 'deploy-group',
            },
            'juggler': {
                'token': 'juggler-token',
                'url': 'http://juggler.test',
            },
            'iam_jwt': {
                'service_account_id': 'test-sa',
                'key_id': 'test-key',
                'private_key': 'dummy',
            },
            'compute': {
                'url': 'compute.test:9051',
                'ca_path': '/nonexistent',
                'service_account_id': 'dummy-sa-id',
                'key_id': 'dummy-key-id',
                'private_key': 'dummy-pk',
                'managed_network_id': 'compute-managed-network-id',
                'folder_id': 'compute-folder-id',
                'use_security_group': True,
            },
            'vpc': {
                'url': 'network.api:9823',
                'token': 'compute-token',
                'ca_path': '/nonexistent',
            },
            's3': {
                'backup': {
                    'access_key_id': 's3-access-key-id',
                    'secret_access_key': 's3-secret-access-key',
                    'endpoint_url': 'http://s3.test',
                    'allow_lifecycle_policies': True,
                },
                'cloud_storage': {
                    'access_key_id': 's3-access-key-id',
                    'secret_access_key': 's3-secret-access-key',
                    'endpoint_url': 'http://s3.test',
                    'allow_lifecycle_policies': True,
                },
                'secure_backups': {
                    'access_key_id': 's3-access-key-id',
                    'secret_access_key': 's3-secret-access-key',
                    'endpoint_url': 'http://s3.test',
                    'secured': True,
                    'folder_id': '',
                    'iam': {
                        'service_account_id': '',
                        'key_id': '',
                        'private_key': '',
                    },
                },
                'access_key_id': 's3-access-key-id',
                'disable_ssl_warnings': True,
                'secret_access_key': 's3-secret-access-key',
                'endpoint_url': 'http://s3.test',
                'idm_endpoint_url': '',
            },
            'solomon': {
                'ca_path': '/nonexistent',
                'token': 'solomon-token',
                'url': 'http://solomon.test',
            },
            'ssh': {
                'private_key': '/ssh-nonexistent',
                'public_key': 'ssh-public-key',
            },
            'internal_api': {
                'url': f'http://{INTERNAL_API_HOST}',
                'ca_path': '/nonexistent',
                'access_id': 'internal-api-access-id',
                'access_secret': 'internal-api-access-secret',
            },
            'resource_manager': {
                'url': 'http://resource-manager.test',
                'ca_path': '/nonexistent',
            },
            'dataproc_manager': {
                'url': 'dataproc-manager.test:443',
                'cert_file': '/nonexistent',
                'sleep_time': 1,
            },
            'instance_group_service': {
                'url': 'instance-group-service.test:443',
                'grpc_timeout': 60,
            },
            'managed_kubernetes_service': {
                'url': 'managed-kubernetes-service.test:443',
                'grpc_timeout': 60,
            },
            'managed_postgresql_service': {
                'url': 'managed-postgres-service.test:443',
                'grpc_timeout': 60,
            },
            'loadbalancer_service': {
                'url': 'lockbox-service.test:443',
                'grpc_timeout': 60,
            },
            'lockbox_service': {
                'url': 'lockbox-service.test:443',
                'grpc_timeout': 60,
            },
            'mlock': {
                'enabled': True,
                'url': 'mlock.test:443',
                'cert_file': '/nonexistent',
                'timeout': 1,
            },
            'postgresql': {
                'sg_service_rules': [
                    {
                        'direction': 'BOTH',
                        'ports_from': 5432,
                        'ports_to': 5432,
                    },
                ],
                'conductor_root_group': 'mdb_test_postgresql',
                'conductor_alt_groups': [
                    {
                        'group_name': 'mdb_test_altgroup',
                        'matcher': {
                            'key': 'flavor',
                            'values': ['alt-test-flavor'],
                        },
                    },
                ],
            },
            'clickhouse': {
                'conductor_root_group': 'mdb_test_clickhouse',
                'conductor_alt_groups': [
                    {
                        'group_name': 'mdb_test_altgroup',
                        'matcher': {
                            'key': 'flavor',
                            'values': ['alt-test-flavor'],
                        },
                    },
                ],
            },
            'greenplum_master': {
                'conductor_root_group': 'mdb_test_greenplum_master',
                'conductor_alt_groups': [
                    {
                        'group_name': 'mdb_test_altgroup',
                        'matcher': {
                            'key': 'flavor',
                            'values': ['alt-test-flavor'],
                        },
                    },
                ],
            },
            'greenplum_segment': {
                'conductor_root_group': 'mdb_test_greenplum_segment',
                'conductor_alt_groups': [
                    {
                        'group_name': 'mdb_test_altgroup',
                        'matcher': {
                            'key': 'flavor',
                            'values': ['alt-test-flavor'],
                        },
                    },
                ],
            },
            'zookeeper': {
                'conductor_root_group': 'mdb_test_zookeeper',
                'conductor_alt_groups': [
                    {
                        'group_name': 'mdb_test_altgroup',
                        'matcher': {
                            'key': 'flavor',
                            'values': ['alt-test-flavor'],
                        },
                    },
                ],
            },
            'mongod': {
                'conductor_root_group': 'mdb_test_mongod',
                'conductor_alt_groups': [
                    {
                        'group_name': 'mdb_test_altgroup',
                        'matcher': {
                            'key': 'flavor',
                            'values': ['alt-test-flavor'],
                        },
                    },
                ],
            },
            'mongos': {
                'conductor_root_group': 'mdb_test_mongos',
                'conductor_alt_groups': [
                    {
                        'group_name': 'mdb_test_altgroup',
                        'matcher': {
                            'key': 'flavor',
                            'values': ['alt-test-flavor'],
                        },
                    },
                ],
            },
            'mongocfg': {
                'conductor_root_group': 'mdb_test_mongocfg',
                'conductor_alt_groups': [
                    {
                        'group_name': 'mdb_test_altgroup',
                        'matcher': {
                            'key': 'flavor',
                            'values': ['alt-test-flavor'],
                        },
                    },
                ],
            },
            'mongoinfra': {
                'conductor_root_group': 'mdb_test_mongoinfra',
                'conductor_alt_groups': [
                    {
                        'group_name': 'mdb_test_altgroup',
                        'matcher': {
                            'key': 'flavor',
                            'values': ['alt-test-flavor'],
                        },
                    },
                ],
            },
            'redis': {
                'conductor_root_group': 'mdb_test_redis',
                'conductor_alt_groups': [
                    {
                        'group_name': 'mdb_test_altgroup',
                        'matcher': {
                            'key': 'flavor',
                            'values': ['alt-test-flavor'],
                        },
                    },
                ],
            },
            'mysql': {
                'conductor_root_group': 'mdb_test_mysql',
                'conductor_alt_groups': [
                    {
                        'group_name': 'mdb_test_altgroup',
                        'matcher': {
                            'key': 'flavor',
                            'values': ['alt-test-flavor'],
                        },
                    },
                ],
            },
            'sqlserver': {
                'conductor_root_group': 'mdb_test_sqlserver',
                'conductor_alt_groups': [
                    {
                        'group_name': 'mdb_test_altgroup',
                        'matcher': {
                            'key': 'flavor',
                            'values': ['alt-test-flavor'],
                        },
                    },
                ],
            },
            'windows_witness': {
                'conductor_root_group': 'mdb_test_windows_witness',
                'conductor_alt_groups': [
                    {
                        'group_name': 'mdb_test_altgroup',
                        'matcher': {
                            'key': 'flavor',
                            'values': ['alt-test-flavor'],
                        },
                    },
                ],
            },
            'kafka': {
                'conductor_root_group': 'mdb_test_kafka',
                'conductor_alt_groups': [
                    {
                        'group_name': 'mdb_test_altgroup',
                        'matcher': {
                            'key': 'flavor',
                            'values': ['alt-test-flavor'],
                        },
                    },
                ],
            },
            'elasticsearch_data': {
                'conductor_root_group': 'mdb_test_elasticsearch_data',
                'conductor_alt_groups': [
                    {
                        'group_name': 'mdb_test_altgroup',
                        'matcher': {
                            'key': 'flavor',
                            'values': ['alt-test-flavor'],
                        },
                    },
                ],
            },
            'elasticsearch_master': {
                'conductor_root_group': 'mdb_test_elasticsearch_master',
                'conductor_alt_groups': [
                    {
                        'group_name': 'mdb_test_altgroup',
                        'matcher': {
                            'key': 'flavor',
                            'values': ['alt-test-flavor'],
                        },
                    },
                ],
            },
            'opensearch_data': {
                'conductor_root_group': 'mdb_test_opensearch_data',
                'conductor_alt_groups': [
                    {
                        'group_name': 'mdb_test_altgroup',
                        'matcher': {
                            'key': 'flavor',
                            'values': ['alt-test-flavor'],
                        },
                    },
                ],
            },
            'opensearch_master': {
                'conductor_root_group': 'mdb_test_opensearch_master',
                'conductor_alt_groups': [
                    {
                        'group_name': 'mdb_test_altgroup',
                        'matcher': {
                            'key': 'flavor',
                            'values': ['alt-test-flavor'],
                        },
                    },
                ],
            },
        },
    )

    validator = jsonschema.Draft4Validator(CONFIG_SCHEMA)
    config_errors = validator.iter_errors(config)
    report_error = jsonschema.exceptions.best_match(config_errors)
    assert report_error is None, f'Unable to load default config: {report_error.message}'

    namespaces = {}
    for section, options in config.items():
        if isinstance(options, collections.abc.Mapping):
            namespaces[section] = SimpleNamespace(**options)
        else:
            namespaces[section] = options
    return SimpleNamespace(**namespaces)


def get_state():
    """
    Get new state with default values
    """
    return deepcopy(DEFAULT_STATE)


def run_task_with_mocks(mocker, task_type, task_args, context, feature_flags, state=None, task_params=None):
    """
    Run worker tasks with mocks (without starting subprocess)
    """
    if not state:
        state = get_state()
    config = _get_config()
    queue = Queue(maxsize=10000)
    task = {
        'cid': 'cid-test',
        'task_id': 'test_id',
        'task_type': task_type,
        'feature_flags': feature_flags,
        'folder_id': 'test_folder',
        'context': context,
        'timeout': 3600,
        'changes': [],
        'cluster_status': 'MODIFYING',
        'create_ts': dt_parser.parse('2022-07-12 12:34:56.082624+03'),
    }
    if task_params:
        task.update(task_params)

    setup_mocks(mocker, state)
    http_mocks = get_http_mocks(state)

    runner = TaskRunner(config, task, task_args, queue)
    # runner.catch_task_failure = False
    with HTTMock(*http_mocks, default_http_mock):
        runner.run()

    messages = []

    while not queue.empty():
        messages.append(queue.get())

    return messages, context, state


def checked_run_task_with_mocks(
    mocker, task_type, task_args, context=None, feature_flags=None, state=None, task_params=None
):
    """
    Run worker tasks with mocks and check that it passed
    """
    if not context:
        context = {}

    if not feature_flags:
        feature_flags = []

    messages, context, state = run_task_with_mocks(
        mocker, task_type, task_args, context, feature_flags, state, task_params
    )

    if messages[-1].error or messages[-1].traceback:
        raise AssertionError(f'run task exit with error: {messages[-1].error}, traceback: {messages[-1].traceback}')

    return messages, context, state
