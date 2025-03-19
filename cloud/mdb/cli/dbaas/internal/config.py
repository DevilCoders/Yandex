#!/usr/bin/env python3
import os
import os.path
import sys
from copy import deepcopy
from typing import Mapping

from click import ClickException
from cloud.mdb.cli.dbaas.internal.common import delete_key, get_key, update_key
from cloud.mdb.cli.common.yaml import dump_yaml, load_yaml

DBAAS_TOOL_ROOT_PATH = os.path.dirname(os.path.realpath(os.path.dirname(sys.path[0])))
YAML_CONFIG_PATH = os.path.join(DBAAS_TOOL_ROOT_PATH, 'config.yaml')

CERTIFICATES = [
    {
        'name': 'CA.pem',
        'url': 'https://crls.yandex.net/allCAs.pem',
    },
]

ENVIRONMENTS = [
    {
        'name': 'porto-test',
        'vtype': 'porto',
        'prod': False,
        'oauth_token_url': 'https://oauth.yandex-team.ru/authorize?response_type=token&client_id=8cdb2f6a0dca48398c6880312ee2f78d',
        'defaults': {
            'timezone': 'Europe/Moscow',
            'ui': {
                'url': 'https://yc-test.yandex-team.ru',
                'accessible_folder_id': 'mdb-junk',
            },
            'admin_ui': {
                'url': 'https://ui.db.yandex-team.ru',
            },
            'walle_ui': {
                'url': 'https://wall-e.yandex-team.ru',
            },
            'iam': {
                'grpc_endpoint': 'ts.cloud.yandex-team.ru:4282',
                'grpc_server_name': 'ts.cloud.yandex-team.ru',
                'rest_endpoint': 'https://iam.cloud.yandex-team.ru',
                'ca_path': os.path.join(DBAAS_TOOL_ROOT_PATH, 'CA.pem'),
                'jwt_audience': 'https://iam.api.cloud.yandex.net/iam/v1/tokens',
            },
            'access_service': {
                'grpc_endpoint': 'as.cloud.yandex-team.ru:4286',
                'grpc_server_name': 'as.cloud.yandex-team.ru',
                'ca_path': os.path.join(DBAAS_TOOL_ROOT_PATH, 'CA.pem'),
            },
            'identity': {
                'rest_endpoint': 'https://iam.cloud.yandex-team.ru',
            },
            'compute': {
                'cloud': 'foorkhlv2jt6khpv69ik',
                'folder': 'mdb-junk',
                'zones': 'sas,vla',
            },
            'resourcemanager': {
                'grpc_endpoint': 'rm.cloud.yandex-team.ru:443',
                'ca_path': os.path.join(DBAAS_TOOL_ROOT_PATH, 'CA.pem'),
            },
            'cms': {
                'grpc_endpoint': 'mdb-cmsgrpcapi-test.db.yandex.net:443',
                'ca_path': os.path.join(DBAAS_TOOL_ROOT_PATH, 'CA.pem'),
            },
            'cmsdb': {
                'hosts': [
                    'cms-db-test01h.db.yandex.net',
                    'cms-db-test01k.db.yandex.net',
                    'cms-db-test01f.db.yandex.net',
                ],
                'user': 'cms',
                'password': 'sec-01e43k7ekz33txbvzaphkez8h0',
            },
            'intapi': {
                'rest_endpoint': 'https://internal-api-test.db.yandex-team.ru',
                'grpc_endpoint': 'mdb-internal-api-test.db.yandex.net',
                'rest_authorization': 'yacloud_subject_token',
                'ca_path': os.path.join(DBAAS_TOOL_ROOT_PATH, 'CA.pem'),
                'crypto_secrets': 'sec-01e2nke1dhyzkah31026kqqv55',
                'id_prefix': 'mdb',
                'resource_preset': 's2.micro',
                'disk_type': 'local-ssd',
            },
            'metadb': {
                'hosts': [
                    'meta-test01h.db.yandex.net',
                    'meta-test01f.db.yandex.net',
                    'meta-test01k.db.yandex.net',
                ],
                'dbname': 'dbaas_metadb',
                'user': 'dbaas_api',
                'password': 'sec-01e0t86sjs56a54v5v46t5k4sh',
            },
            'deploy': {
                'name': 'Deploy',
                'rest_endpoint': 'https://deploy-api-test.db.yandex-team.ru',
                'rest_authorization': 'oauth',
                'oauth_token_url': 'https://oauth.yandex-team.ru/authorize?response_type=token&client_id=84ffe5ac974e4b8baa2cdab9fcac56bd',
            },
            'deploydb': {
                'hosts': [
                    'deploy-db-test01h.db.yandex.net',
                    'deploy-db-test01k.db.yandex.net',
                    'deploy-db-test01f.db.yandex.net',
                ],
                'user': 'deploy_api',
                'password': 'sec-01dvwm7sy75gjhtgak94e1e9qa',
            },
            'dbm': {
                'hosts': [
                    'dbm-test01h.db.yandex.net',
                    'dbm-test01f.db.yandex.net',
                    'dbm-test01k.db.yandex.net',
                ],
                'user': 'dbm',
                'password': 'sec-01e1xw4g2e8j92634gg9s5qx69',
            },
            'dbm_api': {
                'name': 'DBM',
                'rest_endpoint': 'https://mdb-test.db.yandex-team.ru',
                'rest_authorization': 'oauth',
                'token': 'sec-01dtvjgd4g995dee3emyjwmh9q',
            },
            'worker': {
                'hosts': [
                    'dbaas-worker-test01h.db.yandex.net',
                    'dbaas-worker-test01f.db.yandex.net',
                    'dbaas-worker-test01k.db.yandex.net',
                ],
            },
            'katandb': {
                'hosts': [
                    'katan-db-test01h.db.yandex.net',
                    'katan-db-test01f.db.yandex.net',
                    'katan-db-test01k.db.yandex.net',
                ],
                'user': 'katan_imp',
                'password': 'sec-01e0znvgfezs4910080dwrr2c2',
            },
            'certificator': {
                'name': 'MDB Secrets',
                'rest_endpoint': 'https://mdb-secrets-test.db.yandex.net',
                'rest_authorization': 'oauth',
                'token': 'sec-01dtvcq2fm970j0kdzmpcb1vxy',
            },
            'conductor': {
                'name': 'Conductor',
                'rest_endpoint': 'https://c.yandex-team.ru',
                'rest_authorization': 'oauth',
                'token': 'sec-01e08a5d2esg78h2bav4egbbp9',
            },
            'juggler': {
                'name': 'Juggler',
                'api_endpoint': 'http://juggler-api.search.yandex.net',
                'rest_endpoint': 'http://juggler-api.search.yandex.net:8998',
                'oauth_token_url': 'https://oauth.yandex-team.ru/authorize?response_type=token&client_id=cd178dcdc31a4ed79f42467f2d89b0d0',
            },
            'solomon': {
                'name': 'Solomon',
                'project': 'internal-mdb',
                'service': 'internal-mdb_dbaas',
                'rest_endpoint': 'https://solomon.yandex-team.ru',
                'rest_authorization': 'oauth',
                'oauth_token_url': 'https://oauth.yandex-team.ru/authorize?response_type=token&client_id=1c0c37b3488143ff8ce570adb66b9dfa',
            },
            'monitoring': {
                'url': 'https://monitoring.yandex-team.ru',
            },
            'jaeger': {
                'url': 'https://jaeger.yt.yandex-team.ru/mdb-testing',
            },
            'cluster_references': [
                {
                    'name': 'UI - overview',
                    'url': '{ui_url}/folders/{accessible_folder_id}/managed-{cluster_type}/cluster/{cluster_id}',
                },
                {
                    'name': 'UI - monitoring',
                    'url': '{ui_url}/folders/{folder_id}/managed-{cluster_type}/cluster/{cluster_id}?section=monitoring',
                },
                {
                    'name': 'Admin UI',
                    'url': '{admin_ui_url}/meta/cluster/{cluster_id}',
                },
                {
                    'name': 'Monitoring - cluster dashboard',
                    'url': '{monitoring_url}/projects/internal-mdb/dashboards/mon8ej42s7hqqa375ka2?p.service=mdb&p.cid={cluster_id}',
                    'cluster_types': ['clickhouse'],
                },
                {
                    'name': 'Solomon - cluster dashboard',
                    'url': '{solomon_url}/?project=internal-mdb&service=mdb&dashboard=internal-mdb-cluster-{cluster_type}&cluster=mdb_{cluster_id}',
                    'cluster_types': ['mysql', 'redis'],
                },
                {
                    'name': 'Solomon - cluster dashboard',
                    'url': '{solomon_url}/?project=internal-mdb&service=mdb&dashboard=mdb-test-cluster-{cluster_type}&cid={cluster_id}',
                    'cluster_types': ['clickhouse', 'mongodb'],
                },
                {
                    'name': 'Solomon - cluster dashboard',
                    'url': '{solomon_url}/?project=internal-mdb&service=mdb&dashboard=internal-mdb-cluster-postgres&cluster=mdb_{cluster_id}',
                    'cluster_types': ['postgresql'],
                },
                {
                    'name': 'Solomon - host dashboard',
                    'url': '{solomon_url}/?project=internal-mdb&cluster=internal-mdb_dom0&service=dom0&host=by_cid_container&dc=by_cid_container'
                    '&dashboard=internal-mdb-porto-instance&l.container={host}&l.cid={cluster_id}',
                },
                {
                    'name': 'YaSM (Golovan) dashboard',
                    'url': 'https://yasm.yandex-team.ru/template/panel/dbaas_{cluster_type}_metrics/cid={cluster_id}',
                    'cluster_types': ['clickhouse', 'mongodb', 'mysql', 'redis'],
                },
            ],
            'host_references': [
                {
                    'name': 'UI',
                    'url': '{ui_url}/folders/{accessible_folder_id}/{service_name}/cluster/{cluster_id}?section=hosts',
                },
                {
                    'name': 'Admin UI',
                    'url': '{admin_ui_url}/meta/host/{host}',
                },
                {
                    'name': 'Solomon dashboard',
                    'url': '{solomon_url}/?project=internal-mdb&cluster=internal-mdb_dom0&service=dom0&host=by_cid_container&dc=by_cid_container'
                    '&dashboard=internal-mdb-porto-instance&l.container={host}&l.cid={cluster_id}',
                },
            ],
            'dom0_references': [
                {
                    'name': 'Admin UI',
                    'url': '{admin_ui_url}/dbm/dom0host/{dom0}',
                },
                {
                    'name': 'Wall-E UI',
                    'url': '{walle_ui_url}/host/{dom0}',
                },
            ],
            'task_references': [
                {
                    'name': 'Admin UI',
                    'url': '{admin_ui_url}/meta/workerqueue/{task_id}',
                },
            ],
        },
    },
    {
        'name': 'porto-prod',
        'vtype': 'porto',
        'prod': True,
        'oauth_token_url': 'https://oauth.yandex-team.ru/authorize?response_type=token&client_id=8cdb2f6a0dca48398c6880312ee2f78d',
        'defaults': {
            'timezone': 'Europe/Moscow',
            'ui': {
                'url': 'https://yc.yandex-team.ru',
                'accessible_folder_id': 'mdb-junk',
            },
            'admin_ui': {
                'url': 'https://ui-prod.db.yandex-team.ru',
            },
            'walle_ui': {
                'url': 'https://wall-e.yandex-team.ru',
            },
            'iam': {
                'grpc_endpoint': 'ts.cloud.yandex-team.ru:4282',
                'grpc_server_name': 'ts.cloud.yandex-team.ru',
                'rest_endpoint': 'https://iam.cloud.yandex-team.ru',
                'ca_path': os.path.join(DBAAS_TOOL_ROOT_PATH, 'CA.pem'),
                'jwt_audience': 'https://iam.api.cloud.yandex.net/iam/v1/tokens',
            },
            'access_service': {
                'grpc_endpoint': 'as.cloud.yandex-team.ru:4286',
                'grpc_server_name': 'as.cloud.yandex-team.ru',
                'ca_path': os.path.join(DBAAS_TOOL_ROOT_PATH, 'CA.pem'),
            },
            'identity': {
                'rest_endpoint': 'https://iam.cloud.yandex-team.ru',
            },
            'compute': {
                'cloud': 'foorkhlv2jt6khpv69ik',
                'folder': 'mdb-junk',
                'zones': 'sas,vla',
            },
            'resourcemanager': {
                'grpc_endpoint': 'rm.cloud.yandex-team.ru:443',
                'ca_path': os.path.join(DBAAS_TOOL_ROOT_PATH, 'CA.pem'),
            },
            'cms': {
                'grpc_endpoint': 'mdb-cmsgrpcapi.db.yandex.net:443',
                'ca_path': os.path.join(DBAAS_TOOL_ROOT_PATH, 'CA.pem'),
            },
            'cmsdb': {
                'hosts': [
                    'cms-db-prod01h.db.yandex.net',
                    'cms-db-prod01k.db.yandex.net',
                    'cms-db-prod01f.db.yandex.net',
                ],
                'user': 'cms',
                'password': 'sec-01e57r9nya4fa439znnh3fas3d',
            },
            'intapi': {
                'rest_endpoint': 'https://api.db.yandex-team.ru',
                'grpc_endpoint': 'mdb-internal-api.db.yandex.net',
                'rest_authorization': 'yacloud_subject_token',
                'ca_path': os.path.join(DBAAS_TOOL_ROOT_PATH, 'CA.pem'),
                'crypto_secrets': 'sec-01e2nn6xs83e156x4h6m0eqkzk',
                'id_prefix': 'mdb',
                'resource_preset': 's2.micro',
                'disk_type': 'local-ssd',
            },
            'metadb': {
                'hosts': [
                    'meta01h.db.yandex.net',
                    'meta01k.db.yandex.net',
                    'meta01f.db.yandex.net',
                ],
                'dbname': 'dbaas_metadb',
                'user': 'dbaas_api',
                'password': 'sec-01dzb4n5389sajwy4qxngxr5jm',
            },
            'deploy': {
                'name': 'Deploy',
                'rest_endpoint': 'https://deploy-api.db.yandex-team.ru',
                'rest_authorization': 'oauth',
                'oauth_token_url': 'https://oauth.yandex-team.ru/authorize?response_type=token&client_id=84ffe5ac974e4b8baa2cdab9fcac56bd',
            },
            'deploydb': {
                'hosts': [
                    'deploy-db01h.db.yandex.net',
                    'deploy-db01k.db.yandex.net',
                    'deploy-db01f.db.yandex.net',
                ],
                'user': 'deploy_api',
                'password': 'sec-01dz63d661txzmh0k50q72z246',
            },
            'dbm': {
                'hosts': [
                    'dbmdb01h.db.yandex.net',
                    'dbmdb01k.db.yandex.net',
                    'dbmdb01f.db.yandex.net',
                ],
                'user': 'dbm',
                'password': 'sec-01e9dw760ymp8a0na65557ynfx',
            },
            'dbm_api': {
                'name': 'DBM',
                'rest_endpoint': 'https://mdb.yandex-team.ru',
                'rest_authorization': 'oauth',
                'token': 'sec-01dz8nty9ashkz92bneexjg7z8',
            },
            'worker': {
                'hosts': [
                    'dbaas-worker01e.db.yandex.net',
                    'dbaas-worker01f.db.yandex.net',
                    'dbaas-worker01h.db.yandex.net',
                ],
            },
            'katandb': {
                'hosts': [
                    'katan-db01k.db.yandex.net',
                    'katan-db01f.db.yandex.net',
                    'katan-db01h.db.yandex.net',
                ],
                'user': 'katan_imp',
                'password': 'sec-01e1bvec6r8nb5edw1c9g09jne',
            },
            'certificator': {
                'name': 'MDB Secrets',
                'rest_endpoint': 'https://mdb-secrets.db.yandex.net',
                'rest_authorization': 'oauth',
                'token': 'sec-01dz8nj71nswha3f2agnahfqmt',
            },
            'conductor': {
                'name': 'Conductor',
                'rest_endpoint': 'https://c.yandex-team.ru',
                'rest_authorization': 'oauth',
                'token': 'sec-01e08a5d2esg78h2bav4egbbp9',
            },
            'juggler': {
                'name': 'Juggler',
                'api_endpoint': 'http://juggler-api.search.yandex.net',
                'rest_endpoint': 'http://juggler-api.search.yandex.net:8998',
                'oauth_token_url': 'https://oauth.yandex-team.ru/authorize?response_type=token&client_id=cd178dcdc31a4ed79f42467f2d89b0d0',
            },
            'solomon': {
                'name': 'Solomon',
                'project': 'internal-mdb',
                'service': 'internal-mdb_dbaas',
                'rest_endpoint': 'https://solomon.yandex-team.ru',
                'rest_authorization': 'oauth',
                'oauth_token_url': 'https://oauth.yandex-team.ru/authorize?response_type=token&client_id=1c0c37b3488143ff8ce570adb66b9dfa',
            },
            'monitoring': {
                'url': 'https://monitoring.yandex-team.ru',
            },
            'jaeger': {
                'url': 'https://jaeger.yt.yandex-team.ru/mdb-prod',
            },
            'cluster_references': [
                {
                    'name': 'UI - overview',
                    'url': '{ui_url}/folders/{accessible_folder_id}/{service_name}/cluster/{cluster_id}',
                },
                {
                    'name': 'UI - monitoring',
                    'url': '{ui_url}/folders/{folder_id}/{service_name}/cluster/{cluster_id}?section=monitoring',
                },
                {
                    'name': 'Admin UI',
                    'url': '{admin_ui_url}/meta/cluster/{cluster_id}',
                },
                {
                    'name': 'Monitoring - cluster dashboard',
                    'url': '{monitoring_url}/projects/internal-mdb/dashboards/mon8ej42s7hqqa375ka2?p.service=mdb&p.cid={cluster_id}',
                    'cluster_types': ['clickhouse'],
                },
                {
                    'name': 'Solomon - cluster dashboard',
                    'url': '{solomon_url}/?project=internal-mdb&service=mdb&dashboard=internal-mdb-cluster-{cluster_type}&cluster=mdb_{cluster_id}',
                    'cluster_types': ['mysql', 'redis'],
                },
                {
                    'name': 'Solomon - cluster dashboard',
                    'url': '{solomon_url}/?project=internal-mdb&service=mdb&dashboard=mdb-prod-cluster-{cluster_type}&cid={cluster_id}',
                    'cluster_types': ['clickhouse', 'mongodb'],
                },
                {
                    'name': 'Solomon - cluster dashboard',
                    'url': '{solomon_url}/?project=internal-mdb&service=mdb&dashboard=internal-mdb-cluster-postgres&cluster=mdb_{cluster_id}',
                    'cluster_types': ['postgresql'],
                },
                {
                    'name': 'Solomon - host dashboard',
                    'url': '{solomon_url}/?project=internal-mdb&cluster=internal-mdb_dom0&service=dom0&host=by_cid_container&dc=by_cid_container'
                    '&dashboard=internal-mdb-porto-instance&l.container={host}&l.cid={cluster_id}',
                },
                {
                    'name': 'YaSM (Golovan) dashboard',
                    'url': 'https://yasm.yandex-team.ru/template/panel/dbaas_{cluster_type}_metrics/cid={cluster_id}',
                    'cluster_types': ['clickhouse', 'mongodb', 'mysql', 'redis'],
                },
            ],
            'host_references': [
                {
                    'name': 'UI',
                    'url': '{ui_url}/folders/{accessible_folder_id}/{service_name}/cluster/{cluster_id}?section=hosts',
                },
                {
                    'name': 'Admin UI',
                    'url': '{admin_ui_url}/meta/host/{host}',
                },
                {
                    'name': 'DB maintenance',
                    'url': 'https://mdb.yandex-team.ru/containers#fqdn%3D{host}',
                },
                {
                    'name': 'Solomon dashboard',
                    'url': '{solomon_url}/?project=internal-mdb&cluster=internal-mdb_dom0&service=dom0&host=by_cid_container&dc=by_cid_container'
                    '&dashboard=internal-mdb-porto-instance&l.container={host}&l.cid={cluster_id}',
                },
            ],
            'dom0_references': [
                {
                    'name': 'Admin UI',
                    'url': '{admin_ui_url}/dbm/dom0host/{dom0}',
                },
                {
                    'name': 'Wall-E UI',
                    'url': '{walle_ui_url}/host/{dom0}',
                },
            ],
            'task_references': [
                {
                    'name': 'Admin UI',
                    'url': '{admin_ui_url}/meta/workerqueue/{task_id}',
                },
            ],
        },
    },
    {
        'name': 'compute-preprod',
        'vtype': 'compute',
        'prod': False,
        'oauth_token_url': 'https://oauth.yandex.ru/authorize?response_type=token&client_id=1a6990aa636648e9b2ef855fa7bec2fb',
        'defaults': {
            'timezone': 'Europe/Moscow',
            'ui': {
                'url': 'https://console-preprod.cloud.yandex.ru',
                'accessible_folder_id': 'aoefrj1d3vpohedl4fgc',  # folder 'd0uble' in cloud 'yc-mdb-dev'
            },
            'admin_ui': {
                'url': 'https://ui-preprod.db.yandex-team.ru',
            },
            'iam': {
                'grpc_endpoint': 'iam.private-api.cloud-preprod.yandex.net:4283',
                'grpc_server_name': 'iam.private-api.cloud-preprod.yandex.net',
                'rest_endpoint': 'https://identity.private-api.cloud-preprod.yandex.net:14336',
                'ca_path': os.path.join(DBAAS_TOOL_ROOT_PATH, 'CA.pem'),
                'jwt_audience': 'https://iam.api.cloud.yandex.net/iam/v1/tokens',
            },
            'token_service': {
                'grpc_endpoint': 'ts.private-api.cloud-preprod.yandex.net:4282',
                'grpc_server_name': 'ts.private-api.cloud-preprod.yandex.net',
                'ca_path': os.path.join(DBAAS_TOOL_ROOT_PATH, 'CA.pem'),
            },
            'access_service': {
                'grpc_endpoint': 'as.private-api.cloud-preprod.yandex.net:4286',
                'grpc_server_name': 'as.private-api.cloud-preprod.yandex.net',
                'ca_path': os.path.join(DBAAS_TOOL_ROOT_PATH, 'CA.pem'),
            },
            'identity': {
                'rest_endpoint': 'https://identity.private-api.cloud-preprod.yandex.net:14336',
            },
            'compute': {
                'grpc_endpoint': 'compute-api.cloud-preprod.yandex.net:9051',
                'ca_path': os.path.join(DBAAS_TOOL_ROOT_PATH, 'CA.pem'),
                'zones': 'ru-central1-a,ru-central1-b,ru-central1-c',
            },
            'resourcemanager': {
                'grpc_endpoint': 'rm.private-api.cloud-preprod.yandex.net:4284',
                'ca_path': os.path.join(DBAAS_TOOL_ROOT_PATH, 'CA.pem'),
            },
            'cms': {
                'grpc_endpoint': 'mdb-cmsgrpcapi.private-api.cloud-preprod.yandex.net:443',
                'ca_path': os.path.join(DBAAS_TOOL_ROOT_PATH, 'CA.pem'),
            },
            'cmsdb': {
                'hosts': [
                    'mdb-cmsdb-preprod01-rc1a.cloud-preprod.yandex.net',
                    'mdb-cmsdb-preprod01-rc1c.cloud-preprod.yandex.net',
                    'mdb-cmsdb-preprod01-rc1b.cloud-preprod.yandex.net',
                ],
                'user': 'cms',
                'password': 'sec-01eybc6y3rmsdwvmm4qq924gfz',
            },
            'vpc': {
                'grpc_endpoint': 'network-api-internal.private-api.cloud-preprod.yandex.net:9823',
                'ca_path': os.path.join(DBAAS_TOOL_ROOT_PATH, 'CA.pem'),
            },
            'intapi': {
                'rest_endpoint': 'https://mdb.private-api.cloud-preprod.yandex.net',
                'grpc_endpoint': 'mdb-internal-api.private-api.cloud-preprod.yandex.net',
                'rest_authorization': 'yacloud_subject_token',
                'ca_path': os.path.join(DBAAS_TOOL_ROOT_PATH, 'CA.pem'),
                'crypto_secrets': 'sec-01e2nmk3xttsh6xyzeen72995w',
                'id_prefix': 'e4u',
                'overlay_host_suffix': 'mdb.cloud-preprod.yandex.net',
                'underlay_host_suffix': 'db.yandex.net',
                'resource_preset': 's2.micro',
                'disk_type': 'network-ssd',
            },
            'metadb': {
                'hosts': [
                    'meta-dbaas-preprod01f.cloud-preprod.yandex.net',
                    'meta-dbaas-preprod01h.cloud-preprod.yandex.net',
                    'meta-dbaas-preprod01k.cloud-preprod.yandex.net',
                ],
                'dbname': 'dbaas_metadb',
                'user': 'dbaas_api',
                'password': 'sec-01dwetjj2e78zwgtjv0kv7b9g0',
            },
            'deploy': {
                'name': 'Deploy',
                'rest_endpoint': 'https://mdb-deploy-api.private-api.cloud-preprod.yandex.net',
                'rest_authorization': 'oauth',
                'oauth_token_url': 'https://oauth.yandex-team.ru/authorize?response_type=token&client_id=84ffe5ac974e4b8baa2cdab9fcac56bd',
            },
            'deploydb': {
                'hosts': [
                    'mdb-deploy-db-preprod01f.cloud-preprod.yandex.net',
                    'mdb-deploy-db-preprod01h.cloud-preprod.yandex.net',
                    'mdb-deploy-db-preprod01k.cloud-preprod.yandex.net',
                ],
                'user': 'deploy_api',
                'password': 'sec-01dwh2a76vjprzdj8anyrrspdj',
            },
            'worker': {
                'hosts': [
                    'worker-dbaas-preprod01f.cloud-preprod.yandex.net',
                    'worker-dbaas-preprod01h.cloud-preprod.yandex.net',
                    'worker-dbaas-preprod01k.cloud-preprod.yandex.net',
                ],
                'api_key': 'sec-01f87nkh4phah07s5e7q0d2wa2',
            },
            'katandb': {
                'hosts': [
                    'katan-db-preprod01f.cloud-preprod.yandex.net',
                    'katan-db-preprod01h.cloud-preprod.yandex.net',
                    'katan-db-preprod01k.cloud-preprod.yandex.net',
                ],
                'user': 'katan_imp',
                'password': 'sec-01e1bvhmb0s6ykgq69qhgbra27',
            },
            'conductor': {
                'name': 'Conductor',
                'rest_endpoint': 'https://c.yandex-team.ru',
                'rest_authorization': 'oauth',
                'token': 'sec-01e08a5d2esg78h2bav4egbbp9',
            },
            'juggler': {
                'name': 'Juggler',
                'api_endpoint': 'http://juggler-api.search.yandex.net',
                'rest_endpoint': 'http://juggler-api.search.yandex.net:8998',
                'oauth_token_url': 'https://oauth.yandex-team.ru/authorize?response_type=token&client_id=cd178dcdc31a4ed79f42467f2d89b0d0',
            },
            'solomon': {
                'name': 'Solomon',
                'ca_path': os.path.join(DBAAS_TOOL_ROOT_PATH, 'CA.pem'),
                'project': 'yandexcloud',
                'service': 'yandexcloud_dbaas',
                'rest_endpoint': 'https://solomon.cloud.yandex-team.ru',
                'rest_authorization': 'oauth',
                'oauth_token_url': 'https://oauth.yandex-team.ru/authorize?response_type=token&client_id=1c0c37b3488143ff8ce570adb66b9dfa',
            },
            'jaeger': {
                'url': 'https://jaeger-ydb.private-api.ycp.cloud-preprod.yandex.net',
            },
            'cluster_references': [
                {
                    'name': 'UI - overview',
                    'url': '{ui_url}/folders/{accessible_folder_id}/{service_name}/cluster/{cluster_id}',
                },
                {
                    'name': 'UI - monitoring',
                    'url': '{ui_url}/folders/{folder_id}/{service_name}/cluster/{cluster_id}?section=monitoring',
                },
                {
                    'name': 'Admin UI',
                    'url': '{admin_ui_url}/meta/cluster/{cluster_id}',
                },
                {
                    'name': 'Solomon - cluster dashboard',
                    'url': '{solomon_url}/?project=yandexcloud&service=yandexcloud_dbaas&dashboard=cloud-mdb-cluster-{cluster_type}&cluster=mdb_{cluster_id}',
                    'cluster_types': ['mysql', 'redis'],
                },
                {
                    'name': 'Solomon - cluster dashboard',
                    'url': '{solomon_url}/?project=yandexcloud&service=yandexcloud_dbaas&dashboard=mdb-preprod-cluster-{cluster_type}&cid={cluster_id}',
                    'cluster_types': ['clickhouse', 'mongodb'],
                },
                {
                    'name': 'Solomon - cluster dashboard',
                    'url': '{solomon_url}/?project=yandexcloud&service=yandexcloud_dbaas&dashboard=cloud-mdb-cluster-postgres&cluster=mdb_{cluster_id}',
                    'cluster_types': ['postgresql'],
                },
                {
                    'name': 'Solomon - host dashboard',
                    'url': '{solomon_url}/?cluster=mdb_{cluster_id}&host={host}&project=yandexcloud&service=yandexcloud_dbaas&dashboard=cloud-mdb-instance-system',
                },
            ],
            'host_references': [
                {
                    'name': 'UI - cluster hosts',
                    'url': '{ui_url}/folders/{accessible_folder_id}/{service_name}/cluster/{cluster_id}?section=hosts',
                },
                {
                    'name': 'UI - compute instance',
                    'url': '{ui_url}/folders/aoed5i52uquf5jio0oec/compute/instance/{vtype_id}',
                },
                {
                    'name': 'Admin UI',
                    'url': '{admin_ui_url}/meta/host/{host}',
                },
                {
                    'name': 'Solomon dashboard',
                    'url': '{solomon_url}/?cluster=mdb_{cluster_id}&host={host}&project=yandexcloud&service=yandexcloud_dbaas&dashboard=cloud-mdb-instance-system',
                },
            ],
            'task_references': [
                {
                    'name': 'Admin UI',
                    'url': '{admin_ui_url}/meta/workerqueue/{task_id}',
                },
            ],
        },
    },
    {
        'name': 'compute-prod',
        'vtype': 'compute',
        'prod': True,
        'oauth_token_url': 'https://oauth.yandex.ru/authorize?response_type=token&client_id=1a6990aa636648e9b2ef855fa7bec2fb',
        'defaults': {
            'timezone': 'Europe/Moscow',
            'ui': {
                'url': 'https://console.cloud.yandex.ru',
                'accessible_folder_id': 'b1gv0739n43h1ru2tsr3',  # folder 'd0uble' in cloud 'yc-mdb-dev'
            },
            'admin_ui': {
                'url': 'https://ui-compute-prod.db.yandex-team.ru',
            },
            'iam': {
                'grpc_endpoint': 'iam.private-api.cloud.yandex.net:4283',
                'grpc_server_name': 'iam.private-api.cloud.yandex.net',
                'rest_endpoint': 'https://identity.private-api.cloud.yandex.net:14336',
                'ca_path': os.path.join(DBAAS_TOOL_ROOT_PATH, 'CA.pem'),
                'jwt_audience': 'https://iam.api.cloud.yandex.net/iam/v1/tokens',
            },
            'token_service': {
                'grpc_endpoint': 'ts.private-api.cloud.yandex.net:4282',
                'grpc_server_name': 'ts.private-api.cloud.yandex.net',
                'ca_path': os.path.join(DBAAS_TOOL_ROOT_PATH, 'CA.pem'),
            },
            'access_service': {
                'grpc_endpoint': 'as.private-api.cloud.yandex.net:4286',
                'grpc_server_name': 'as.private-api.cloud.yandex.net',
                'ca_path': os.path.join(DBAAS_TOOL_ROOT_PATH, 'CA.pem'),
            },
            'identity': {
                'rest_endpoint': 'https://identity.private-api.cloud.yandex.net:14336',
            },
            'compute': {
                'grpc_endpoint': 'compute-api.cloud.yandex.net:9051',
                'ca_path': os.path.join(DBAAS_TOOL_ROOT_PATH, 'CA.pem'),
                'cloud': None,
                'folder': None,
                'zones': 'ru-central1-a,ru-central1-b,ru-central1-c',
            },
            'resourcemanager': {
                'grpc_endpoint': 'rm.private-api.cloud.yandex.net:4284',
                'ca_path': os.path.join(DBAAS_TOOL_ROOT_PATH, 'CA.pem'),
            },
            'cms': {
                'grpc_endpoint': 'mdb-cmsgrpcapi.private-api.cloud.yandex.net:443',
                'ca_path': os.path.join(DBAAS_TOOL_ROOT_PATH, 'CA.pem'),
            },
            'cmsdb': {
                'hosts': [
                    'mdb-cmsdb01-rc1a.yandexcloud.net',
                    'mdb-cmsdb01-rc1b.yandexcloud.net',
                    'mdb-cmsdb01-rc1c.yandexcloud.net',
                ],
                'user': 'cms',
                'password': 'sec-01f2h24pmd7dk07r6vrwb2mw1y',
            },
            'vpc': {
                'grpc_endpoint': 'network-api.private-api.cloud.yandex.net:9823',
                'ca_path': os.path.join(DBAAS_TOOL_ROOT_PATH, 'CA.pem'),
            },
            'intapi': {
                'rest_endpoint': 'https://mdb.private-api.cloud.yandex.net',
                'grpc_endpoint': 'mdb-internal-api.private-api.cloud.yandex.net',
                'rest_authorization': 'yacloud_subject_token',
                'ca_path': os.path.join(DBAAS_TOOL_ROOT_PATH, 'CA.pem'),
                'crypto_secrets': 'sec-01e2nnb4bc2db586casf0n9wtm',
                'id_prefix': 'c9q',
                'overlay_host_suffix': 'mdb.yandexcloud.net',
                'underlay_host_suffix': 'db.yandex.net',
                'resource_preset': 's2.micro',
                'disk_type': 'network-ssd',
            },
            'metadb': {
                'hosts': [
                    'meta-dbaas01f.yandexcloud.net',
                    'meta-dbaas01h.yandexcloud.net',
                    'meta-dbaas01k.yandexcloud.net',
                ],
                'dbname': 'dbaas_metadb',
                'user': 'dbaas_api',
                'password': 'sec-01egfwt4tjk1xvx1nwxqq3dq3g',
            },
            'deploy': {
                'name': 'Deploy',
                'rest_endpoint': 'https://mdb-deploy-api.private-api.yandexcloud.net',
                'rest_authorization': 'oauth',
                'oauth_token_url': 'https://oauth.yandex-team.ru/authorize?response_type=token&client_id=84ffe5ac974e4b8baa2cdab9fcac56bd',
            },
            'deploydb': {
                'hosts': [
                    'mdb-deploy-db01f.yandexcloud.net',
                    'mdb-deploy-db01h.yandexcloud.net',
                    'mdb-deploy-db01k.yandexcloud.net',
                ],
                'user': 'deploy_api',
                'password': 'sec-01e48pznq82te0f9bwcd96mqg0',
            },
            'worker': {
                'hosts': [
                    'worker-dbaas01h.yandexcloud.net',
                    'worker-dbaas01k.yandexcloud.net',
                    'worker-dbaas01f.yandexcloud.net',
                ],
                'api_key': 'sec-01f9c3bb319254ze30qzvqtrzt',
            },
            'katandb': {
                'hosts': [
                    'mdb-katandb01-rc1a.yandexcloud.net',
                    'mdb-katandb01-rc1b.yandexcloud.net',
                    'mdb-katandb01-rc1c.yandexcloud.net',
                ],
                'user': 'katan_imp',
                'password': 'sec-01e1bvvbbc4vfyw72n123x7fa7',
            },
            'conductor': {
                'name': 'Conductor',
                'rest_endpoint': 'https://c.yandex-team.ru',
                'rest_authorization': 'oauth',
                'token': 'sec-01e08a5d2esg78h2bav4egbbp9',
            },
            'juggler': {
                'name': 'Juggler',
                'api_endpoint': 'http://juggler-api.search.yandex.net',
                'rest_endpoint': 'http://juggler-api.search.yandex.net:8998',
                'oauth_token_url': 'https://oauth.yandex-team.ru/authorize?response_type=token&client_id=cd178dcdc31a4ed79f42467f2d89b0d0',
            },
            'solomon': {
                'name': 'Solomon',
                'ca_path': os.path.join(DBAAS_TOOL_ROOT_PATH, 'CA.pem'),
                'project': 'yandexcloud',
                'service': 'yandexcloud_dbaas',
                'rest_endpoint': 'https://solomon.cloud.yandex-team.ru',
                'rest_authorization': 'oauth',
                'oauth_token_url': 'https://oauth.yandex-team.ru/authorize?response_type=token&client_id=1c0c37b3488143ff8ce570adb66b9dfa',
            },
            'jaeger': {
                'url': 'https://jaeger.private-api.ycp.cloud.yandex.net',
            },
            'cluster_references': [
                {
                    'name': 'UI - overview',
                    'url': '{ui_url}/folders/{accessible_folder_id}/{service_name}/cluster/{cluster_id}',
                },
                {
                    'name': 'UI - monitoring',
                    'url': '{ui_url}/folders/{folder_id}/{service_name}/cluster/{cluster_id}?section=monitoring',
                },
                {
                    'name': 'Admin UI',
                    'url': '{admin_ui_url}/meta/cluster/{cluster_id}',
                },
                {
                    'name': 'Solomon - cluster dashboard',
                    'url': '{solomon_url}/?project=yandexcloud&service=yandexcloud_dbaas&dashboard=cloud-mdb-cluster-{cluster_type}&cluster=mdb_{cluster_id}',
                    'cluster_types': ['mysql', 'redis'],
                },
                {
                    'name': 'Solomon - cluster dashboard',
                    'url': '{solomon_url}/?project=yandexcloud&service=yandexcloud_dbaas&dashboard=mdb-prod-cluster-{cluster_type}&cid={cluster_id}',
                    'cluster_types': ['clickhouse', 'mongodb'],
                },
                {
                    'name': 'Solomon - cluster dashboard',
                    'url': '{solomon_url}/?project=yandexcloud&service=yandexcloud_dbaas&dashboard=cloud-mdb-cluster-postgres&cluster=mdb_{cluster_id}',
                    'cluster_types': ['postgresql'],
                },
                {
                    'name': 'Solomon - host dashboard',
                    'url': '{solomon_url}/?cluster=mdb_{cluster_id}&host={host}&project=yandexcloud&service=yandexcloud_dbaas&dashboard=cloud-mdb-instance-system',
                },
            ],
            'host_references': [
                {
                    'name': 'UI - cluster hosts',
                    'url': '{ui_url}/folders/{accessible_folder_id}/{service_name}/cluster/{cluster_id}?section=hosts',
                },
                {
                    'name': 'UI - compute instance',
                    'url': '{ui_url}/folders/b1gdepbkva865gm1nbkq/compute/instance/{vtype_id}',
                },
                {
                    'name': 'Admin UI',
                    'url': '{admin_ui_url}/meta/host/{host}',
                },
                {
                    'name': 'Solomon dashboard',
                    'url': '{solomon_url}/?cluster=mdb_{cluster_id}&host={host}&project=yandexcloud&service=yandexcloud_dbaas&dashboard=cloud-mdb-instance-system',
                },
            ],
            'task_references': [
                {
                    'name': 'Admin UI',
                    'url': '{admin_ui_url}/meta/workerqueue/{task_id}',
                },
            ],
        },
    },
    {
        'name': 'israel',
        'vtype': 'compute',
        'prod': True,
        'oauth_token_url': None,
        'defaults': {
            'timezone': 'UTC',
            'ui': {
                'url': 'https://console.cloudil.co.il',
                'accessible_folder_id': 'yc.mdb.test',
            },
            'iam': {},
            'compute': {
                'cloud': 'yc.mdb.serviceCloud',
                'folder': 'yc.mdb.test',
                'zones': 'il1-a',
            },
            'intapi': {
                'id_prefix': 'cam',
                'resource_preset': 'c3-c2-m4',
                'disk_type': 'network-ssd',
            },
            'metadb': {
                'hosts': [
                    'metadb01-il1-a.mdb-cp.yandexcloud.co.il',
                    'metadb02-il1-a.mdb-cp.yandexcloud.co.il',
                    'metadb03-il1-a.mdb-cp.yandexcloud.co.il',
                ],
                'dbname': 'dbaas_metadb',
                'user': 'dbaas_api',
                'password': 'sec-01g43g9901m1kb4nyfx9h4bq38',
            },
            'deploydb': {
                'hosts': [
                    'deploydb02-il1-a.mdb-cp.yandexcloud.co.il',
                    'deploydb01-il1-a.mdb-cp.yandexcloud.co.il',
                    'deploydb03-il1-a.mdb-cp.yandexcloud.co.il',
                ],
                'user': 'deploy_api',
                'password': 'sec-01g4cg082v5j5pjcyevd3mg3b9',
            },
            'juggler': {
                'name': 'Juggler',
                'api_endpoint': 'http://juggler-api.search.yandex.net',
                'oauth_token_url': 'https://oauth.yandex-team.ru/authorize?response_type=token&client_id=cd178dcdc31a4ed79f42467f2d89b0d0',
            },
            'solomon': {},
            'cluster_references': [
                {
                    'name': 'UI - overview',
                    'url': '{ui_url}/folders/{accessible_folder_id}/{service_name}/cluster/{cluster_id}',
                },
                {
                    'name': 'UI - monitoring',
                    'url': '{ui_url}/folders/{folder_id}/{service_name}/cluster/{cluster_id}?section=monitoring',
                },
            ],
            'host_references': [
                {
                    'name': 'UI',
                    'url': '{ui_url}/folders/{accessible_folder_id}/{service_name}/cluster/{cluster_id}?section=hosts',
                },
            ],
            'task_references': [],
            'jumphost': {
                'host': 'metadb01-il1-a.mdb-cp.yandexcloud.co.il',
                'port': 22,
                'user': 'root',
            },
        },
    },
    {
        'name': 'dc-preprod',
        'vtype': 'aws',
        'prod': False,
        'defaults': {
            'timezone': 'UTC',
            'ui': {
                'url': 'https://app.yadc.io',
                'accessible_folder_id': 'mdb-junk',
            },
            'admin_ui': {
                'url': 'https://ui-preprod.yadc.io',
            },
            'backstage_ui': {
                'url': 'https://backstage.preprod.mdb.internal.yadc.io',
            },
            'iam': {
                'grpc_endpoint': 'iam.internal.yadc.io:4283',
                'grpc_server_name': 'iam.internal.yadc.io',
                'jwt_audience': 'https://iam.api.cloud.yandex.net/iam/v1/tokens',
                'api_key': 'sec-01f73eqd86582dbxns9048wf6c',
            },
            'token_service': {
                'grpc_endpoint': 'iam.internal.yadc.io:4282',
                'grpc_server_name': 'iam.internal.yadc.io',
            },
            'access_service': {
                'grpc_endpoint': 'iam.internal.yadc.io:4286',
                'grpc_server_name': 'iam.internal.yadc.io',
            },
            'resourcemanager': {
                'grpc_endpoint': 'iam.internal.yadc.io:4284',
                'grpc_server_name': 'iam.internal.yadc.io',
            },
            'compute': {
                'cloud': 'mdb-junk',
                'folder': 'mdb-junk',
                'zones': 'eu-central1-a',
            },
            'intapi': {
                'id_prefix': 'cho',
            },
            'metadb': {
                'hosts': [
                    'preprod-metadb.cjse2ukweu21.eu-central-1.rds.amazonaws.com',
                ],
                'port': 5432,
                'dbname': 'metadb',
                'user': 'meta_admin',
                'password': 'sec-01g45cc2qh4b5rnkq5vsa6dd15',
            },
            'deploydb': {
                'hosts': 'preprod-deploydb.cjse2ukweu21.eu-central-1.rds.amazonaws.com',
                'user': 'deploy_admin',
                'port': 5432,
            },
            'vpcdb': {
                'hosts': [
                    'preprod-vpcdb.cjse2ukweu21.eu-central-1.rds.amazonaws.com',
                ],
                'port': 5432,
                'dbname': 'vpcdb',
                'user': 'vpc_admin',
                'password': 'sec-01g4w24rd9bnat714wp6tmzjak',
            },
            'billingdb': {
                'hosts': [
                    'preprod-billingdb.cjse2ukweu21.eu-central-1.rds.amazonaws.com',
                ],
                'port': 5432,
                'dbname': 'billingdb',
                'user': 'billing_admin',
                'password': 'sec-01g8xpdg1thjdsvd595bt8vx8g',
            },
            'juggler': {},
            'solomon': {},
            'monitoring': {},
            'jaeger': {},
            'cluster_references': [
                {
                    'name': 'UI - overview',
                    'url': '{ui_url}/{cluster_type}/{cluster_id}',
                },
                {
                    'name': 'UI - monitoring',
                    'url': '{ui_url}/{cluster_type}/{cluster_id}/monitoring',
                },
                {
                    'name': 'Admin UI',
                    'url': '{admin_ui_url}/meta/cluster/{cluster_id}',
                },
                {
                    'name': 'Backstage UI',
                    'url': '{backstage_ui_url}/ui/meta/clusters/{cluster_id}',
                },
                {
                    'name': 'Grafana - cluster dashboard',
                    'url': 'https://grafana.infra.double.tech/d/VIPlum77k/clickhouse-dashboard?orgId=1&var-environment=MDB%20Preprod&var-filter=cid%7C%3D%7C{cluster_id}',
                    'cluster_types': ['clickhouse'],
                },
            ],
            'host_references': [
                {
                    'name': 'Admin UI',
                    'url': '{admin_ui_url}/meta/host/{host}',
                },
                {
                    'name': 'Backstage UI',
                    'url': '{backstage_ui_url}/ui/meta/hosts/{host}',
                },
            ],
            'task_references': [
                {
                    'name': 'Admin UI',
                    'url': '{admin_ui_url}/meta/workerqueue/{task_id}',
                },
                {
                    'name': 'Backstage UI',
                    'url': '{backstage_ui_url}/ui/meta/worker_tasks/{task_id}',
                },
            ],
            'jumphost': {
                'host': 'controlplane-jump01c.euc1.datacloud.db.yandex.net',
                'port': 22,
                'user': 'ubuntu',
            },
        },
    },
    {
        'name': 'dc-prod',
        'vtype': 'aws',
        'prod': False,
        'defaults': {
            'timezone': 'UTC',
            'ui': {
                'url': 'https://app.double.cloud',
                'accessible_folder_id': 'dogfood',
            },
            'admin_ui': {
                'url': 'https://ui-prod.mdb.double.tech',
            },
            'backstage_ui': {
                'url': 'https://backstage.prod.mdb.internal.double.tech',
            },
            'iam': {
                'grpc_endpoint': 'prod.iam.internal.double.tech:4283',
                'grpc_server_name': 'prod.iam.internal.double.tech',
                'jwt_audience': 'https://iam.api.cloud.yandex.net/iam/v1/tokens',
                'api_key': 'sec-01g4far8sgrbrgs3xfewgybybt',
            },
            'token_service': {
                'grpc_endpoint': 'prod.iam.internal.double.tech:4282',
                'grpc_server_name': 'prod.iam.internal.double.tech',
            },
            'access_service': {
                'grpc_endpoint': 'prod.iam.internal.double.tech:4286',
                'grpc_server_name': 'prod.iam.internal.double.tech',
            },
            'resourcemanager': {
                'grpc_endpoint': 'prod.iam.internal.double.tech:4284',
                'grpc_server_name': 'prod.iam.internal.double.tech',
            },
            'compute': {
                'cloud': 'dogfood',
                'folder': 'dogfood',
                'zones': 'eu-central1-a',
            },
            'intapi': {
                'id_prefix': 'mdb',
            },
            'metadb': {
                'hosts': [
                    'prod-metadb.cx0f6l6slwsj.eu-central-1.rds.amazonaws.com',
                ],
                'port': 5432,
                'dbname': 'metadb',
                'user': 'meta_admin',
                'password': 'sec-01g45cj7dv62vdvhwf77g1e513',
            },
            'deploydb': {
                'hosts': 'prod-deploydb.cx0f6l6slwsj.eu-central-1.rds.amazonaws.com',
                'user': 'deploy_admin',
                'port': 5432,
            },
            'vpcdb': {
                'hosts': [
                    'prod-vpcdb.cx0f6l6slwsj.eu-central-1.rds.amazonaws.com',
                ],
                'port': 5432,
                'dbname': 'vpcdb',
                'user': 'vpc_admin',
                'password': 'sec-01g4w4jb6bsfw1084wp7319ame',
            },
            'billingdb': {
                'hosts': [
                    'prod-billingdb.cx0f6l6slwsj.eu-central-1.rds.amazonaws.com',
                ],
                'port': 5432,
                'dbname': 'billingdb',
                'user': 'billing_admin',
                'password': 'sec-01g8xpw90zb239vw0c81dcwgw1',
            },
            'juggler': {},
            'solomon': {},
            'monitoring': {},
            'jaeger': {},
            'cluster_references': [
                {
                    'name': 'UI - overview',
                    'url': '{ui_url}/{cluster_type}/{cluster_id}',
                },
                {
                    'name': 'UI - monitoring',
                    'url': '{ui_url}/{cluster_type}/{cluster_id}/monitoring',
                },
                {
                    'name': 'Admin UI',
                    'url': '{admin_ui_url}/meta/cluster/{cluster_id}',
                },
                {
                    'name': 'Backstage UI',
                    'url': '{backstage_ui_url}/ui/meta/clusters/{cluster_id}',
                },
                {
                    'name': 'Grafana - cluster dashboard',
                    'url': 'https://grafana.infra.double.tech/d/VIPlum77k/clickhouse-dashboard?orgId=1&var-environment=MDB%20Prod&var-filter=cid%7C%3D%7C{cluster_id}',
                    'cluster_types': ['clickhouse'],
                },
            ],
            'host_references': [
                {
                    'name': 'Admin UI',
                    'url': '{admin_ui_url}/meta/host/{host}',
                },
                {
                    'name': 'Backstage UI',
                    'url': '{backstage_ui_url}/ui/meta/hosts/{host}',
                },
            ],
            'task_references': [
                {
                    'name': 'Admin UI',
                    'url': '{admin_ui_url}/meta/workerqueue/{task_id}',
                },
                {
                    'name': 'Backstage UI',
                    'url': '{backstage_ui_url}/ui/meta/worker_tasks/{task_id}',
                },
            ],
            'jumphost': {
                'host': 'cp-jump-prod.datacloud.db.yandex.net',
                'port': 22,
                'user': 'ubuntu',
            },
        },
    },
]

CONFIG_PRESETS = [
    {
        'name': 'porto test',
        'environment': 'porto-test',
        'auth_type': 'oauth_token',
        'config': {},
    },
    {
        'name': 'porto prod, admin mode',
        'environment': 'porto-prod',
        'auth_type': 'oauth_token',
        'config': {
            'intapi': {
                'rest_endpoint': 'https://api-admin-dbaas01k.db.yandex.net',
            },
        },
    },
    {
        'name': 'porto prod, support mode',
        'environment': 'porto-prod',
        'auth_type': 'oauth_token',
        'config': {},
    },
    {
        'name': 'compute preprod, admin mode',
        'environment': 'compute-preprod',
        'auth_type': 'user_key',
        'config': {
            'intapi': {
                'rest_endpoint': 'https://api-admin-dbaas-preprod01k.cloud-preprod.yandex.net',
            },
        },
    },
    {
        'name': 'compute preprod, support mode',
        'environment': 'compute-preprod',
        'auth_type': 'user_key',
        'config': {
            'metadb': {
                'user': 'dbaas_support',
                'password': 'sec-01dws6czdg4tkyy8ajmqb4r969',
            },
        },
    },
    {
        'name': 'compute prod, admin mode',
        'environment': 'compute-prod',
        'auth_type': 'user_key',
        'config': {
            'intapi': {
                'rest_endpoint': 'https://api-admin-dbaas01k.yandexcloud.net',
            },
        },
    },
    {
        'name': 'compute prod, support mode',
        'environment': 'compute-prod',
        'auth_type': 'user_key',
        'config': {
            'metadb': {
                'user': 'dbaas_support',
                'password': 'sec-01e9995a3sbs8pjhxz9bc9ka44',
            },
        },
    },
    {
        'name': 'israel',
        'environment': 'israel',
        'auth_type': 'none',
        'config': {},
    },
    {
        'name': 'dc preprod',
        'environment': 'dc-preprod',
        'auth_type': 'none',
        'config': {},
    },
    {
        'name': 'dc prod',
        'environment': 'dc-prod',
        'auth_type': 'none',
        'config': {},
    },
]

_UNSET = object()


def load_config(ctx, profile_name=None, config_key_values=None):
    """
    Read config file and store its contents into the context.
    """
    _load_config(ctx, profile_name)
    for key, value in config_key_values or []:
        update_key(ctx.obj['config'], key, value)


def get_config(ctx):
    """
    Return config from the context.
    """
    return ctx.obj['config']


def get_config_key(ctx, key_path, default=None):
    """
    Return config key value.
    """
    return get_key(ctx.obj['config'], key_path, default=default)


def update_config_key(ctx, key_path, value):
    """
    Update config key value in the currently active profile.
    """
    profiles = ctx.obj['profiles']
    active_profile = profiles[_active_profile(ctx)]
    update_key(active_profile, key_path, value)

    yaml_config = dict(profiles=profiles, default_profile=_default_profile(ctx))
    dump_yaml(yaml_config, YAML_CONFIG_PATH)


def delete_config_key(ctx, key_path):
    """
    Delete config key in the currently active profile.
    """
    profiles = ctx.obj['profiles']
    active_profile = profiles[_active_profile(ctx)]
    delete_key(active_profile, key_path)

    yaml_config = dict(profiles=profiles, default_profile=_default_profile(ctx))
    dump_yaml(yaml_config, YAML_CONFIG_PATH)


def get_vtype(ctx):
    """
    Return virtualization type of the currently active configuration profile.
    """
    return ctx.obj['config']['vtype']


def config_option(ctx, section, option, default=_UNSET):
    """
    Return config option value.
    """
    result = ctx.obj['config'].get(section, {}).get(option, default)
    if result == _UNSET:
        raise ClickException(f'Required option "{section}.{option}" is missing in the config file.')

    return result


def get_profiles(ctx):
    result = []

    active_profile = _active_profile(ctx)
    for name, config in ctx.obj['profiles'].items():
        _process_config(name, config)
        result.append(
            {
                'name': name,
                'environment': config.get('environment'),
                'int api': config.get('intapi', {}).get('rest_endpoint'),
                'active': name == active_profile,
            }
        )

    return result


def get_profile_names(ctx):
    return list(ctx.obj['profiles'].keys())


def activate_profile(ctx, name):
    profiles = ctx.obj['profiles']
    if name not in profiles:
        raise ClickException(f'Profile "{name}" does not exist.')

    yaml_config = dict(profiles=profiles, default_profile=name)
    dump_yaml(yaml_config, YAML_CONFIG_PATH)


def create_profile(
    ctx,
    name,
    config_preset,
    auth_config,
    folder,
):
    profile_config = {
        'config_preset': config_preset['name'],
        'environment': config_preset['environment'],
        'iam': auth_config,
        'compute': {
            'folder': folder,
        },
    }
    profile_config.update(config_preset['config'])

    profiles = ctx.obj['profiles']
    profiles[name] = profile_config

    yaml_config = dict(profiles=profiles, default_profile=name)
    dump_yaml(yaml_config, YAML_CONFIG_PATH)


def delete_profile(ctx, name):
    profiles = ctx.obj['profiles']
    try:
        del profiles[name]
    except KeyError:
        raise ClickException(f'Profile "{name}" does not exist.')

    default_profile = _default_profile(ctx)
    if name == default_profile and profiles:
        default_profile = next(iter(profiles.keys()))

    yaml_config = dict(profiles=profiles, default_profile=default_profile)
    dump_yaml(yaml_config, YAML_CONFIG_PATH)


def rename_profile(ctx, name, new_name):
    profiles = ctx.obj['profiles']

    if name not in profiles:
        raise ClickException(f'Profile "{name}" does not exist.')

    if new_name in profiles:
        raise ClickException(f'Profile "{new_name}" already exists.')

    profiles[new_name] = profiles.pop(name)

    default_profile = _default_profile(ctx)
    if default_profile == name:
        default_profile = new_name

    yaml_config = dict(profiles=profiles, default_profile=default_profile)
    dump_yaml(yaml_config, YAML_CONFIG_PATH)


def get_environment_name(ctx):
    """
    Get environment name of the currently active profile.
    """
    return get_config(ctx)['environment']


def get_environment(name):
    """
    Get environment by name.
    """
    for env in ENVIRONMENTS:
        if name == env['name']:
            return env

    raise RuntimeError(f'Environment "{name}" not found.')


def get_config_preset(name):
    """
    Get config preset by name.
    """
    for preset in CONFIG_PRESETS:
        if name == preset['name']:
            return preset

    raise RuntimeError(f'Config preset "{name}" not found.')


def _load_config(ctx, profile_name=None):
    if os.path.exists(os.path.expanduser(YAML_CONFIG_PATH)):
        yaml_config = load_yaml(YAML_CONFIG_PATH) or {}
        profiles = yaml_config.get('profiles', {})
        default_profile = yaml_config.get('default_profile')
        profile_name = profile_name or default_profile
        try:
            config = profiles[profile_name]
        except KeyError:
            raise ClickException(f'Profile "{profile_name}" does not exist.')
        config['profile'] = profile_name
        config['default_profile'] = default_profile

        _process_config(profile_name, config)

    else:
        profiles = {}
        config = {}

    ctx.obj['profiles'] = profiles
    ctx.obj['config'] = config


def _active_profile(ctx):
    """
    Return name of the active profile.
    """
    return ctx.obj['config'].get('profile')


def _default_profile(ctx):
    """
    Return name of the default profile.
    """
    return ctx.obj['config'].get('default_profile')


def _update_profile_option(profile, option, value):
    if isinstance(value, Mapping):
        current_value = profile.get(option, {})
        merge_config(current_value, value)
        value = current_value

    profile[option] = value


def _process_config(profile_name, config):
    try:
        # determine environment and apply defaults
        env_config = get_environment(config['environment'])

        defaults = deepcopy(env_config['defaults'])
        defaults['environment'] = env_config['name']
        defaults['vtype'] = env_config['vtype']
        defaults['prod'] = env_config['prod']

        merged_config = merge_config(defaults, config)
        config.update(merged_config)

        # expand file paths
        if 'ca_path' in config['iam']:
            config['iam']['ca_path'] = os.path.expanduser(config['iam']['ca_path'])

        if 'ca_path' in config['intapi']:
            config['intapi']['ca_path'] = os.path.expanduser(config['intapi']['ca_path'])

        if 'ca_path' in config['solomon']:
            config['solomon']['ca_path'] = os.path.expanduser(config['solomon']['ca_path'])

    except Exception:
        print(
            f'Profile "{profile_name}" has incomplete configuration. '
            f'Please re-initialize with "dbaas -p \'{profile_name}\' init".',
            file=sys.stderr,
        )


def merge_config(base, update):
    for key in update:
        if key in base and isinstance(base[key], dict) and isinstance(update[key], dict):
            merge_config(base[key], update[key])
        else:
            base[key] = update[key]
    return base
