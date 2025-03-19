"""
Override default configuration with ~/.dataproc-infra-test.yaml
"""

import os

import requests
import yaml

from retrying import retry

from .helpers.utils import combine_dict


@retry(wait_exponential_multiplier=1000, wait_exponential_max=10000, stop_max_attempt_number=5)
def ensure_yandex_internal_ca_exists(file_path: str):
    if not os.path.exists(os.path.dirname(file_path)):
        os.makedirs(os.path.dirname(file_path))
    if not os.path.exists(file_path):
        with open(file_path, 'wb') as outfile:
            url = 'https://crls.yandex.net/allCAs.pem'
            response = requests.get(url)
            outfile.write(response.content)


CONF_OVERRIDE = {
    'compute_driver': {
        'port': 8123,
        'network_id': 'c64vs98keiqc7f24pvkd',
        'resources': {
            'cores': 4,
            'memory': 8589934592,
            'core_fraction': 100,
            'root_disk_size': 21474836480,
            'platform': 'standard-v2',
            'zone': 'ru-central1-b',
        },
    },
    'put_to_context': {
        'serviceAccountId1': 'bfb851nkcljpfr008rer',
        'serviceAccountId2': 'bfbr5jplob3brsvm2sse',  # with editor role, mandatory for instance groups
        'serviceAccountId3': 'bfbokdiavbh95tjh1r8l',
        'serviceAccountId4': 'bfbuh09603g65b1rq8cj',  # sqlserver sa with storage:uploader
        'outputBucket1': 'dataproc-infra-test',
        'outputBucket2': 'dataproc-infra-test-2',
        'zoneWithHostGroup': 'ru-central1-c',
        'subnetWithHostGroup': 'fo2efq4nd4sac5m85tnt',
        'hostGroupId': 'd9hm1bmv9i3pluoqh87n',
        'securityGroupId': 'c6452e60lj7i7iuklajq',
        'kafkaSecurityGroupId': 'c64ifdrvmcdinld7q4ov',
        'logGroupId': 'af3mpa8k58cnb5q28071',
        'wrongLogGroupId': 'af37ejm1mb0iggudg2v5',
        'wrongLogGroupFolderId': 'aoed5i52uquf5jio0oec',
        'folderId': 'aoeb0d5hocqev4i6rmmf',
    },
    'test_managed': {
        'cloudId': 'aoe9shbqc2v314v7fp3d',
        'folderId': 'aoeb0d5hocqev4i6rmmf',
        'networkId': 'c649e7rrl7f5pgpc9o6t',
        'subnetIds': ['bucf61l8dveckbu8o9q2', 'blt9umjegpgq4mponnnu', 'fo230niudcoomnmtjob3'],
        'zoneId': 'ru-central1-b',
        'securityGroupId': 'c64ifdrvmcdinld7q4ov',
    },
    'test_redis': {
        'zoneId': 'ru-central1-a',
    },
    'test_kubernetes': {
        'cloudId': 'aoe9shbqc2v314v7fp3d',
        'folderId': 'aoe78c3212fbi4rpre7n',
        'networkId': 'c64fs7250h41729o2ctn',
        'service_subnet_ids': ['bucd0genetip31dsb8bg'],
        'user_subnet_ids': ['bucoh5ado1m561oem27r'],
        'zoneId': 'ru-central1-a',
        'securityGroupId': 'c64flud531iut72r93b1',
        'kubernetes_cluster_id': 'c49b5vcsdbj2hlf7rq8a',
        'postgresql_cluster_id': 'e4uvie24ohcnil2i3rqf',
        'secrets_folder_id': 'aoe78c3212fbi4rpre7n',
        'kubernetes_cluster_service_account_id': 'bfbj38c2d18rt6ifvns1',
        'kubernetes_node_service_account_id': 'bfbj38c2d18rt6ifvns1',
        'postgresql_hostname': 'c-e4uvie24ohcnil2i3rqf.rw.mdb.cloud-preprod.yandex.net',
    },
    'test_sqlserver': {
        'cloudId': 'aoe9shbqc2v314v7fp3d',
        'folderId': 'aoeb0d5hocqev4i6rmmf',
        'networkId': 'c64cm3cps7st9c7ks01f',
        'subnetId': ['bucdan01bhisj80jkmlj', 'bltnas5si736upc8ucq9', 'fo2jij82vfj1qb8m8ika'],
        'zoneId': ['ru-central1-a', 'ru-central1-b', 'ru-central1-c'],
    },
    'test_dataproc': {
        'zoneId': 'ru-central1-b',
        'sshPublicKeys': 'ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAII7JOBFU5LGCd/ET220neX7MiWIXHnZI9ZfFjjgnPMmh',
        'resources': {'resourcePresetId': 's2.micro', 'diskTypeId': 'network-ssd', 'diskSize': 21474836480},
        'subnetId': 'bltbgdthcleu7gmaplia',
        'cloudId': 'aoe9shbqc2v314v7fp3d',
        'folderId': 'aoeb0d5hocqev4i6rmmf',
        'productId': 'a6qhu09ehnvq16jcp2h0',
        'subnetWithoutNatId': 'fo2p80qu9ckaeul56i49',
        'subnetWithoutNatZone': 'ru-central1-c',
        'securityGroupId': 'c6452e60lj7i7iuklajq',
        'control_plane_log_group_id': 'af3mpa8k58cnb5q28071',
        'logging_url': 'api.cloud-preprod.yandex.net:443',
    },
    'dns': {
        'ca_path': '/opt/yandex/allCA.pem',
        'token': '',
        'token_secret_id': 'sec-01dmwzbjyn9xgrm862ps8v94xa',
        'url': 'https://dns-api.yandex.net/',
        'account': 'robot-dnsapi-mdb',
        'ttl': 60,
        'name_server': '2a02:6b8::1001',
    },
    'compute': {
        'ca_path': '/opt/yandex/allCA.pem',
        'folder_id': 'aoeb0d5hocqev4i6rmmf',
        'dataplane_folder_id': 'aoeb0d5hocqev4i6rmmf',
        'geo_map': {},
        'compute_grpc_url': 'compute-api.cloud-preprod.yandex.net:9051',
        'instance_group_grpc_url': 'instance-group.private-api.ycp.cloud-preprod.yandex.net:443',
        'vpc_grpc_url': 'api-adapter.private-api.ycp.cloud-preprod.yandex.net:443',
        'token': '',
        'sa_secret_id': 'sec-01fc8bkfjybxy07m72d6a8hry3',
        'dataproc_user_sa_secret_id': 'sec-01fq20c9c95f3a7b1940gr5rkm',
        'image_folder_id': '',
        'grpc_timeout': 30,
    },
    's3': {
        'backup_secret_id': 'ver-01fvynr24p8pnteja3gkm7vccj',
        'backup': {
            'endpoint': 'https://s3-private.mds.yandex.net',
            'virtual_addressing_style': True,
            'path_addressing_style': False,
        },
        'endpoint_url': 'storage.cloud-preprod.yandex.net',
        'region_name': 'ru-central1',
        'bucket_name': 'dataproc-infra-test',
    },
    'solomon_cloud_secret': 'ver-01enr0m5b29avx16nga0g72nkg',
    'token_service_address': 'ts.private-api.cloud-preprod.yandex.net:4282',
    'dataproc_agent_config_override': {'jobs': {'dataproc_manager': {'get_new_jobs_interval': '1s'}}},
    'common': {
        'ya_command': '../../../ya',
        'yc_command': './staging/bin/yc',
        'yav_oauth': '',
        'cert_secret_id': 'sec-01dt4eqq879wqa6xam0mtb80w4',
        'docker_registry_secret_id': 'sec-01dt4fggfp4gg5cmzwbkzym8n1',
        'cr_secret_id': 'sec-01efh76mspfgpjvez0gc66eb08',
        'pguser_postgres_secret_id': 'sec-01dx0s1kbpexya0bt1vqfmetkb',
        'saltapi_password_secret_id': 'sec-01e6bfg8akvmrgyagsedx080ey',
        'tvmtool_secret_id': 'sec-01e6bxczm5dcnyq4fs1n0af46a',
        'dhparam_secret_id': 'sec-01e6ek50nb9e38snshmn3zyc45',
        'certificator_secret_id': 'sec-01f3t32trb9te2fcg9sgt0wgvy',
        'compute_token_secret_id': 'sec-01dw7q2mqq4hpkynqxj4cvfwmy',
        'worker_dummmy_key_id': 'sec-01f91wpr4bt4h4fapwgax9gyrw',
        'cp_logging_sa_key_secret_id': 'sec-01fk5zrfpehkwsd4m0x4844pgw',
    },
}

PATH = os.getenv('CONFIG_PATH') or os.path.expanduser('~/.dataproc-infra-test.yaml')
try:
    if os.path.exists(PATH):
        with open(PATH) as CONF:
            CONF_OVERRIDE = combine_dict(CONF_OVERRIDE, yaml.safe_load(CONF))
except Exception as exc:
    print('Unable to load local config: {exc}'.format(exc=repr(exc)))

if not os.path.exists(CONF_OVERRIDE['dns']['ca_path']):
    CONF_OVERRIDE['dns']['ca_path'] = 'staging/allCAs.pem'
    ensure_yandex_internal_ca_exists(CONF_OVERRIDE['dns']['ca_path'])

if not os.path.exists(CONF_OVERRIDE['compute']['ca_path']):
    CONF_OVERRIDE['compute']['ca_path'] = 'staging/allCAs.pem'
    ensure_yandex_internal_ca_exists(CONF_OVERRIDE['compute']['ca_path'])
