"""
Register mocks
"""

import getpass
import io
import json
import os
from collections import namedtuple
from contextlib import contextmanager
from datetime import datetime
from types import SimpleNamespace
from typing import Dict, List, Generator

from dateutil.tz import tzutc

from dbaas_internal_api.apis.config_auth import ConfigAuthProvider as ApiConfigAuthProvider
from dbaas_internal_api.core.auth import AuthError, AuthProvider
from dbaas_internal_api.core.crypto import CryptoProvider
from dbaas_internal_api.core.exceptions import PreconditionFailedError
from dbaas_internal_api.core.id_generators import HostnameGenerator, IDGenerator
from dbaas_internal_api.health.health import HealthInfo, cluster_health_from_mdbh
from dbaas_internal_api.modules.hadoop.traits import HostHealth as HadoopHostHealth
from dbaas_internal_api.utils.cluster_secrets import ClusterSecretsProvider
from dbaas_internal_api.utils.compute import ComputeProvider, HostGroup, HostType, instances, images
from dbaas_internal_api.utils.dataproc_manager.health import ExtendedClusterHealth
from dbaas_internal_api.utils.compute_billing import ComputeBillingProvider, YCComputeInstance
from dbaas_internal_api.utils.identity import IdentityProvider
from dbaas_internal_api.utils.logging_service import LoggingService, LogGroupNotFoundError
from dbaas_internal_api.utils.logging_read_service import LoggingReadService
from dbaas_internal_api.utils.network import (
    NetworkProvider,
    YCNetwork,
    SecurityGroup,
    SecurityGroupRule,
    SecurityGroupRuleDirection,
)
from dbaas_internal_api.utils.resource_manager import ResourceManager
from dbaas_internal_api.utils.types import ClusterHealth
from dbaas_internal_api.utils.s3 import S3Api
from dbaas_internal_api.utils.validation import AbstractURIValidator
from dbaas_internal_api.utils.version import AbstractVersionValidator, Version


class MDBHealthProviderFromFile:
    """
    MDB Health provider taking responses from file
    """

    def __init__(self, config) -> None:
        self.file_path = config['path']

    def get_hosts_health(self, fqdns: List[str], _) -> HealthInfo:
        """
        Retrieves health of specified fqdns
        """
        with open(self.file_path) as inp_file:
            data = json.load(inp_file)

        resp = []
        for host in data.get('hosts', []):
            if host['fqdn'] in fqdns:
                resp.append(host)
        return HealthInfo(resp)

    def get_cluster_health(self, cid: str, _) -> ClusterHealth:
        """
        Retrieves health of specified fqdns
        """
        with open(self.file_path) as inp_file:
            data = json.load(inp_file)

        for cluster in data.get('clusters', []):
            if cluster['cid'] == cid:
                return cluster_health_from_mdbh(cluster.get('status', 'Unknown'))
        return ClusterHealth.unknown


class ConfigAuthProvider(AuthProvider):
    """
    Auth provider mock
    """

    def __init__(self, config):
        self.conf = config['MOCKED_AUTH']

    def ping(self):
        """
        Never raises exceptions (for /ping to work)
        """
        pass

    def authenticate(self, request_obj):
        """
        Reject if token is unknown or missing
        """
        token = request_obj.headers.get('X-YaCloud-SubjectToken')
        if not token or token not in self.conf:
            raise AuthError('Invalid token: {token}'.format(token=token))

        context = SimpleNamespace()
        context.token = token
        context.user = SimpleNamespace()
        context.user.id = self.conf[token]['user_id']
        context.user.type = self.conf[token]['user_type']

        return context

    def authorize(self, action, token, folder_id=None, cloud_id=None, sa_id=None):
        """
        Reject if folder_id/cloud_id is not allowed for token
        """
        if folder_id is not None and folder_id not in self.conf[token]['folders']:
            raise AuthError('No acl for folder {id}'.format(id=folder_id))

        if cloud_id is not None and cloud_id not in self.conf[token]['clouds']:
            raise AuthError('No acl for cloud {id}'.format(id=cloud_id))

        if folder_id is not None and self.conf[token]['folders'][folder_id]['read_only'] and action != 'mdb.all.read':
            raise AuthError('Read-only for folder {id}'.format(id=folder_id))

        if (
            folder_id is not None
            and action == 'mdb.all.forceDelete'
            and not self.conf[token]['folders'][folder_id]['force_delete']
        ):
            raise AuthError('User have no permission {action} for folder {id}'.format(action=action, id=folder_id))

        if action == 'logging.records.write' and not self.conf[token]['folders'][folder_id]['logging.records.write']:
            raise AuthError(f'No permission to write logs to {folder_id}')

        if sa_id is not None and sa_id not in self.conf[token]['service_accounts']:
            raise AuthError('No acl for service_account {id}'.format(id=sa_id))


class DummyAuthProvider(AuthProvider):
    """
    Auth provider allowing everything
    """

    def __init__(self, _):
        self.context = SimpleNamespace()
        self.context.token = 'dummy'
        self.context.user = SimpleNamespace()
        self.context.user.id = getpass.getuser()
        self.context.user.type = 'user_account'

    def ping(self):
        """
        Never raises exceptions (for /ping to work)
        """
        pass

    def authenticate(self, _):
        """
        Authenticate every request
        """
        return self.context

    def authorize(self, *_):
        """
        Authorize every request
        """
        pass


class MockedCryptoProvider(CryptoProvider):
    """
    Non-encrypting provider
    """

    def __init__(self, _):
        pass

    def encrypt(self, data: str):
        """
        Just return plain data
        """
        return {'data': data, 'encryption_version': 0}

    def encrypt_bytes(self, data: bytes):
        """
        Return base64 encoded data
        """
        return self.encrypt(data.decode('utf-8'))

    def gen_random_string(self, length, symbols):
        """
        Return dummy string
        """
        return 'dummy'


class DummyYCIdentityProvider(IdentityProvider):
    """
    Dummy identity provider (cloud = folder)
    """

    def __init__(self, _):
        pass

    def get_cloud_by_folder_ext_id(self, folder_ext_id):
        """
        Return folder_ext_id as cloud_ext_id
        """
        return folder_ext_id

    def get_cloud_permission_stages(self, _):
        """
        Always return empty list
        """
        return []


class ConfigYCIdentityProvider(IdentityProvider):
    """
    Config-based identity provider
    """

    def __init__(self, config):
        self.map = config['CONFIG_YC_IDENTITY']['map']
        self.status_path = config['CONFIG_YC_IDENTITY']['status_path']
        self.feature_flags_path = config['CONFIG_YC_IDENTITY']['feature_flags_path']

    def get_cloud_by_folder_ext_id(self, folder_ext_id):
        """
        Get cloud_ext_id from config map
        """
        return self.map[folder_ext_id]

    def get_cloud_permission_stages(self, _):
        """
        Get permission stages from file
        """
        if os.path.exists(self.feature_flags_path):
            with open(self.feature_flags_path) as inp_file:
                data = json.load(inp_file)

            return data
        return []


class DummySubnetMap:
    """
    Special subnet map (returns subnet for every zone)
    """

    def __init__(self, network_id, folder_id):
        self.network_id = network_id
        self.folder_id = folder_id

    def get(self, zone_id, _):
        """
        Return subnet id for given zone_id
        """
        return {'{net}-{zone}'.format(net=self.network_id, zone=zone_id): self.folder_id}


class DummyYCNetworkProvider(NetworkProvider):
    """
    Dummy network provider
    """

    def get_security_group(self, sg_id: str) -> SecurityGroup:
        return SecurityGroup(id=sg_id, name='', folder_id='', network_id='', rules=[])

    def __init__(self, *_):
        pass

    def get_network(self, network_id):
        """
        Return dummy ycnetwork
        """
        return YCNetwork(network_id=network_id, folder_id='dummy')

    def get_subnets(self, network):
        """
        Return dummy subnets
        """
        return DummySubnetMap(network.network_id, network.folder_id)

    def get_subnet(self, subnet_id, public_ip_used):
        """
        Return dummy subnet
        """
        network, zone = subnet_id.split('-', maxsplit=1)
        return {
            'id': subnet_id,
            'name': subnet_id,
            'networkId': network,
            'zoneId': zone,
            'folderId': 'folder1',
        }

    def get_networks(self, folder_id):
        """
        Return dummy networks
        """
        return [
            YCNetwork(network_id='dummy', folder_id='dummy'),
        ]


class ConfigYCNetworkProvider(NetworkProvider):
    """
    Config-based network provider
    """

    def get_security_group_rules(self, rules: List[dict]) -> List[SecurityGroupRule]:
        return [
            SecurityGroupRule(
                id=rule.get('id', ''),
                description=rule.get('description', ''),
                direction=rule.get('direction', SecurityGroupRuleDirection.UNSPECIFIED),
                ports_from=rule.get('ports_from', 0),
                ports_to=rule.get('ports_to', 0),
                protocol_name=rule.get('protocol_name', 'ANY'),
                protocol_number=rule.get('protocol_number', 0),
                v4_cidr_blocks=rule.get('v4_cidr_blocks', []),
                v6_cidr_blocks=rule.get('v6_cidr_blocks', []),
                predefined_target=rule.get('predefined_target'),
                security_group_id=rule.get('security_group_id'),
            )
            for rule in rules
        ]

    def get_security_group(self, sg_id: str) -> SecurityGroup:
        sg = self.map.get('security_groups', {}).get(sg_id, {})
        return SecurityGroup(
            folder_id=sg.get('folder_id', ''),
            network_id=sg.get('network_id', ''),
            id=sg.get('id', ''),
            name=sg.get('name', ''),
            rules=self.get_security_group_rules(sg.get('rules', [])),
        )

    def __init__(self, config):
        self.map = config
        #  network names which provider should ignore
        self.unresolved = config.get('unresolved_networks')

    def get_subnets(self, network):
        """
        Return subnets for specified folder and network from config
        """
        if network.network_id in self.unresolved:
            return {}
        return self.map.get(network.network_id, {})

    def get_network(self, network_id):
        if network_id in self.unresolved:
            return YCNetwork(network_id='', folder_id=None)
        return YCNetwork(network_id=network_id, folder_id=None)

    def get_subnet(self, subnet_id, public_ip_used):
        """
        Return dummy subnet
        """
        subnet = self.map['subnets'].get(subnet_id)
        if subnet:
            return subnet

        network, zone = subnet_id.split('-')
        return {
            'id': subnet_id,
            'name': subnet_id,
            'networkId': network,
            'zoneId': zone,
            'v4CidrBlock': ['192.168.0.0/16'],
            'folderId': 'folder1',
        }

    def get_networks(self, folder_id):
        """
        Return dummy networks
        """
        return [
            YCNetwork(network_id='dummy', folder_id='dummy'),
        ]


def get_next_val(sequence_path):
    """
    Get next sequence value from file
    """
    if os.path.exists(sequence_path):
        with open(sequence_path) as sequence_file:
            current_value = int(sequence_file.read())
    else:
        current_value = 0

    current_value += 1

    os.makedirs(os.path.dirname(sequence_path), exist_ok=True)

    with open(sequence_path, 'w') as sequence_file:
        sequence_file.write(str(current_value))

    return current_value


class SequenceIdGenerator(IDGenerator):
    """
    Generate id from file-based sequence
    """

    def __init__(self, config):
        self.base_path = config['SEQUENCE_PATH']

    def generate_id(self, id_type):
        """
        Gen id from id_type sequence
        """
        value = get_next_val(os.path.join(self.base_path, id_type))
        return '{id_type}{val}'.format(id_type=id_type, val=value)


class SequenceHostnameGenerator(HostnameGenerator):
    """
    Generate hostname from file-based sequence
    """

    def __init__(self, config):
        self.base_path = config['SEQUENCE_PATH']

    def generate_hostname(self, prefix, suffix):
        """
        Gen hostname from prefix sequence
        """
        value = get_next_val(os.path.join(self.base_path, 'hostname-{prefix}'.format(prefix=prefix)))
        return '{prefix}{value}{suffix}'.format(prefix=prefix, value=value, suffix=suffix)


class FileS3Api(S3Api):
    """
    File-based s3 provider
    """

    def __init__(self, *_, **__):
        # Unfortunately there is no easy way to get config in this context
        from flask import current_app

        self.file_path = current_app.config['S3_PROVIDER_RESPONSE_FILE']

    def list_objects(self, prefix: str, delimiter: str = None) -> dict:
        """
        Dummy list objects
        """
        ret_objs = []
        ret_prefixes = set()
        with open(self.file_path) as inp_file:
            file_contents = json.load(inp_file)
            for obj in file_contents.get('Contents', []):
                if not self._object_match(obj, prefix):
                    continue

                if self._is_leaf_object(obj, prefix, delimiter):
                    obj['LastModified'] = datetime.fromtimestamp(obj['LastModified'], tz=tzutc())
                    ret_objs.append(obj)
                else:
                    ret_prefixes.add(self._get_child_prefix(obj, base_prefix=prefix, delimiter=delimiter))

        ret = {}
        if ret_objs:
            ret['Contents'] = ret_objs
        if ret_prefixes:
            ret['CommonPrefixes'] = [{'Prefix': prefix} for prefix in ret_prefixes]

        return ret

    @staticmethod
    def _object_match(s3_object: dict, prefix: str) -> bool:
        """
        Return True if the passed in S3 object matches filtering criteria.
        """
        return s3_object['Key'].startswith(prefix)

    @staticmethod
    def _is_leaf_object(s3_object: dict, prefix: str, delimiter: str = None) -> bool:
        """
        Return True if the passed in S3 object is leaf object for given prefix and delimiter.
        """
        if not delimiter:
            return True

        return s3_object['Key'].find(delimiter, len(prefix)) == -1

    @staticmethod
    def _get_child_prefix(s3_object: dict, base_prefix: str, delimiter: str) -> str:
        idx = s3_object['Key'].find(delimiter, len(base_prefix))
        return s3_object['Key'][:idx] + delimiter

    def object_exists(self, key: str) -> bool:
        """
        Return True if object exists
        """
        with open(self.file_path) as inp_file:
            file_contents = json.load(inp_file)
            for obj in file_contents.get('Contents', []):
                if obj['Key'] == key:
                    return True

        return False

    @contextmanager
    def get_object_body(self, key, **_):
        """
        Dummy object body getter
        """
        for obj in self.list_objects('')['Contents']:
            if obj['Key'] == key:
                yield io.StringIO(json.dumps(obj['Body']))


class DummyConfigAuthProvider(ApiConfigAuthProvider):
    """
    Dummy config auth provider
    """

    def auth(self, *_):
        """
        Do not check auth
        """
        pass


class DummyLoggingService(LoggingService):
    """
    Dummy logging service
    """

    user_service_account_token = 'logging-service-account-token'

    def __init__(self, *args, **kwargs):
        pass

    def work_as_user_specified_account(self, service_account_id):
        pass

    def get_default(self, folder_id: str):
        Response = namedtuple('Response', ['data'])
        LogGroup = namedtuple('LogGroup', ['id', 'folder_id'])
        response = Response(data=LogGroup(folder_id='folder1', id='log_group_default'))
        return response

    def get(self, log_group_id):
        Response = namedtuple('Response', ['data'])
        LogGroup = namedtuple('LogGroup', ['id', 'folder_id'])
        if log_group_id == 'not_existing_log_group_id':
            raise LogGroupNotFoundError(f'Log group {log_group_id} is not found')
        if log_group_id == 'log_group_from_folder_2_id':
            response = Response(data=LogGroup(folder_id='folder2', id='log_group_2'))
        else:
            response = Response(data=LogGroup(folder_id='folder1', id='log_group_1'))
        return response

    def list(self, folder_id):
        pass


class DummyLoggingReadService(LoggingReadService):
    """
    Dummy logging read service
    """

    def __init__(self, *args, **kwargs):
        pass

    def work_as_user_specified_account(self, service_account_id):
        pass

    def read(self, *args, **kwargs):
        pass


class SequenceClusterSecretsProvider(ClusterSecretsProvider):
    """
    Generate cluster secrets from file-base sequence
    """

    def __init__(self, config):
        self.base_path = config['SEQUENCE_PATH']

    def generate(self):
        value = str(get_next_val(os.path.join(self.base_path, 'cluster-secret'))).encode()
        return value, value


class DummyYCComputeBillingProvider(ComputeBillingProvider):
    """
    Dummy YC.Compute Provider
    """

    def __init__(self, config):
        # pylint: disable=unused-argument
        pass

    def get_billing_metrics(self, instance: YCComputeInstance):
        """
        Get billing metrics for compute instance
        """
        result = [
            {
                'folder_id': instance.folder_id,
                'schema': 'nbs.volume.allocated.v1',
                'tags': {
                    'size': instance.boot_disk.size,
                    'type': instance.boot_disk.type_id,
                },
            },
            {
                'folder_id': instance.folder_id,
                'schema': 'compute.vm.generic.v1',
                'tags': {
                    'cores': instance.resources.cores,
                    'gpus': instance.resources.gpus,
                    'memory': instance.resources.memory,
                    'platform_id': instance.resources.platform_id,
                    'product_ids': ['dummy-product-id'],
                    'public_fips': 0,
                    'sockets': 1,
                },
            },
        ]
        return result


class ConfigResourceManager(ResourceManager):
    """
    Resouse manager provider mock
    """

    def __init__(self, config):
        self.map = config['RESOURCE_MANAGER_CONFIG']

    def service_account_roles(self, _, service_account_id):
        account_roles = self.map.get(service_account_id, [])
        return set(account_roles)


class DataprocManagerMock:
    STATUS_CONVERTER = {
        'UNKNOWN': ClusterHealth.unknown,
        'ALIVE': ClusterHealth.alive,
        'DEAD': ClusterHealth.dead,
        'DEGRADED': ClusterHealth.degraded,
    }

    def __init__(self, config) -> None:
        self.health_path = config['health_path']

    def cluster_health(self, _) -> ExtendedClusterHealth:
        hdfs_in_safemode = False
        if os.path.isfile(self.health_path):
            with open(self.health_path) as file:
                data = json.load(file)
                health = data['health']
                explanation = data.get('explanation', '')
                hdfs_in_safemode = data.get('hdfs_in_safemode', hdfs_in_safemode)
            os.remove(self.health_path)
        else:
            health = 'UNKNOWN'
            explanation = ''
        return ExtendedClusterHealth(
            health=self.STATUS_CONVERTER[health], explanation=explanation, hdfs_in_safemode=hdfs_in_safemode
        )

    def hosts_health(self, _, fqdns: List[str]) -> Dict[str, HadoopHostHealth]:
        return {fqdn: HadoopHostHealth.alive for fqdn in fqdns}


class IamClientMock:
    def __init__(self, config):
        pass

    def issue_iam_token(self, service_account_id):
        return 'logging-service-account-token'


class ComputeQuotaServiceMock:
    def __init__(self, config) -> None:
        gib = 1024.0 * 1024.0 * 1024.0
        self.quota = {
            'compute.images.count': 20.0,
            'compute.ssdNonReplicatedDisks.size': 256 * gib,
            'compute.instanceMemory.size': 128 * gib,
            'compute.instanceGpus.count': 0.0,
            'compute.ssdDisks.size': 5030 * gib,
            'compute.hddDisks.size': 256 * gib,
            'compute.disks.count': 20.0,
            'compute.instances.count': 20.0,
            'compute.hostGroups.count': 0.0,
            'compute.diskPlacementGroups.count': 20.0,
            'compute.placementGroups.count': 20.0,
            'compute.snapshots.count': 20.0,
            'compute.snapshots.size': 256 * gib,
            'compute.instanceCores.count': 2400,
        }
        self.available_cloud_quota = dict()
        for key, value in self.quota.items():
            self.available_cloud_quota[key] = value / 2

    def get(self, cloud_id: str):
        response = namedtuple('GrpcResponse', ['data', 'meta'])
        response.data = self.quota
        return response

    def get_available_compute_quota(self, cloud_id: str):
        return self.available_cloud_quota


class DummyURIValidator(AbstractURIValidator):
    def validate(self, url: str) -> None:
        pass


class DummyVersionValidator(AbstractVersionValidator):
    """
    Version validator.
    """

    def ensure_version_exists(self, package_name: str, version: Version) -> None:
        if len(version.string.split('.')) != 4:
            raise PreconditionFailedError(f'Can\'t create cluster, version \'{version.string}\' is not found')


class ConfigYCComputeProvider(ComputeProvider):
    """
    Config-based compute provider
    """

    def __init__(self, config):
        self.map = config

    def get_host_group(self, host_group_id: str) -> HostGroup:
        """
        Get host group by id
        """
        hg = self.map.get('host_groups', {}).get(host_group_id, {})
        return HostGroup(
            id=hg.get('id', ''),
            name=hg.get('name', ''),
            folder_id=hg.get('folder_id', ''),
            zone_id=hg.get('zone_id', ''),
            host_type_id=hg.get('host_type_id', ''),
            host_type=None,
        )

    def get_host_type(self, host_type_id: str) -> HostType:
        """
        Get host type by id
        """
        ht = self.map.get('host_types', {}).get(host_type_id, {})
        return HostType(
            id=ht.get('id', -1),
            cores=ht.get('cores', -1),
            memory=ht.get('memory', -1),
            disks=ht.get('disks', -1),
            disk_size=ht.get('disk_size', -1),
        )

    def get_instance(self, instance_id: str) -> instances.InstanceModel:
        """
        Get instance by id
        """
        pass

    def get_image(self, image_id: str) -> images.ImageModel:
        """
        Get image by id
        """
        return images.ImageModel(
            id='image_id_1.4.1',
            name='',
            description='Image 1',
            folder_id='folder1',
            min_disk_size=21474836480,
            status=images.ImageStatus.READY,
            created_at=datetime.now(),
            labels={'version': '1.4.1'},
            family='dataproc-image-1-4',
        )

    def list_images(self, folder_id: str) -> Generator[images.ImageModel, None, None]:
        """
        Get images in folder
        """
        compute_images = [
            images.ImageModel(
                id='image_id_1.3.1',
                name='',
                description='Image 1.3',
                folder_id='folder1',
                min_disk_size=21474836480,
                status=images.ImageStatus.READY,
                created_at=datetime.now(),
                labels={'version': '1.3.1'},
                family='dataproc-image-1-3',
            ),
            images.ImageModel(
                id='image_id_1.4.1',
                name='',
                description='Image 1',
                folder_id='folder1',
                min_disk_size=21474836480,
                status=images.ImageStatus.READY,
                created_at=datetime.now(),
                labels={'version': '1.4.1'},
                family='dataproc-image-1-4',
            ),
            images.ImageModel(
                id='image_id_2.0.1',
                name='',
                description='Image 2',
                folder_id='folder1',
                min_disk_size=21474836480,
                status=images.ImageStatus.READY,
                created_at=datetime.now(),
                labels={'version': '2.0.1'},
                family='yandex-dataproc-image-2-0',
            ),
            images.ImageModel(
                id='image_id_99.0.0',
                name='',
                description='Image 99',
                folder_id='folder1',
                min_disk_size=21474836480,
                status=images.ImageStatus.READY,
                created_at=datetime.now(),
                labels={'version': '99.0.0'},
                family='yandex-dataproc-image-99-0',
            ),
        ]
        for image in compute_images:
            yield image
