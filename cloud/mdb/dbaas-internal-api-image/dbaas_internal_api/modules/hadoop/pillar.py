"""
Hadoop pillar generator
"""
from copy import deepcopy
from typing import Dict, List, Optional, Any

import semver
from flask import current_app

from ...core.base_pillar import BasePillar
from ...core.exceptions import ParseConfigError, DbaasError
from ...utils.helpers import merge_dict
from ...utils.types import ClusterInfo
from .constants import MASTER_SUBCLUSTER_TYPE, MY_CLUSTER_TYPE
from .traits import ClusterService


# pylint: disable=too-many-public-methods
class HadoopPillar(BasePillar):
    """
    Hadoop pillar.
    """

    # pylint: disable=too-many-instance-attributes

    def __init__(self, pillar: Optional[Dict] = None) -> None:
        super().__init__(pillar or HadoopPillar._make_default_pillar(None))

    @staticmethod
    def _make_default_pillar(image_config: Optional[dict] = None) -> dict:
        """
        Create default pillar for image with version_id.
        """

        if not image_config:
            for image_config in current_app.config['VERSIONS'][MY_CLUSTER_TYPE].values():
                if image_config.get('default'):  # type: ignore
                    break

        pillar_version = image_config['supported_pillar']  # type: ignore
        return deepcopy(current_app.config['HADOOP_PILLARS'][pillar_version])

    @staticmethod
    def make(image_config: Optional[dict]) -> "HadoopPillar":
        """
        Create new HadoopPillar for image with version_id.
        """
        pillar_data = HadoopPillar._make_default_pillar(image_config)
        pillar_data['data'].setdefault('auxiliary', {})['default_properties'] = deepcopy(
            pillar_data['data']['unmanaged'].get('properties', {})
        )
        return HadoopPillar(pillar_data)

    @property
    def _hadoop(self):
        return self._pillar['data']['unmanaged']

    @property
    def _auxiliary(self):
        return self._pillar['data'].setdefault('auxiliary', {})

    @property
    def s3(self) -> dict:
        """
        Get optional overrides s3 endpoint and region
        """
        return self._hadoop.get('s3', {})

    @property
    def user_s3_bucket(self) -> Optional[str]:
        """
        Get user S3 bucket
        """
        return self._hadoop.get('s3_bucket')

    @user_s3_bucket.setter
    def user_s3_bucket(self, name: str) -> None:
        """
        Set user S3 bucket
        """
        self._hadoop['s3_bucket'] = name

    @property
    def services(self) -> List[ClusterService]:
        """Get cluster supported services"""
        services = self._hadoop.get('services')
        if not services:
            return []
        return ClusterService.to_enums(services)

    @services.setter
    def services(self, services: List[ClusterService]) -> None:
        if not services:
            return
        services = sorted(list(set(services)), key=lambda srvc: srvc.name)
        self._hadoop['services'] = ClusterService.to_strings(services)

    @property
    def properties(self) -> Dict:
        """Get cluster properties including default ones"""
        return self.get_properties(include_defaults=True)

    @property
    def user_properties(self) -> Dict:
        """Get cluster properties set by user"""
        return self.get_properties(include_defaults=False)

    @user_properties.setter
    def user_properties(self, properties: Dict) -> None:
        self._auxiliary['user_properties'] = {}
        self._merge_properties(self._auxiliary['user_properties'], properties)

        self._hadoop['properties'] = deepcopy(self._auxiliary.get('default_properties', {}))
        self._merge_properties(self._hadoop['properties'], properties)

    def get_properties(self, include_defaults: bool) -> Dict:
        result = {}
        # Condition "'user_properties' not in self._hadoop" is for existing clusters
        # that do not have user_properties in their pillar. Returning 'properties'
        # for them when include_defaults=False is still correct because currently
        # our defaults are empty.
        if include_defaults or 'user_properties' not in self._auxiliary:
            properties = self._hadoop.get('properties', {})
        else:
            properties = self._auxiliary['user_properties']
        for prefix, props in properties.items():
            for property_name, property_value in props.items():
                result[f'{prefix}:{property_name}'] = property_value
        return result

    @staticmethod
    def _merge_properties(current_properties: Dict, properties: Dict) -> None:
        """Fill current_properties stored within pillar with user provided values"""
        for k, property_value in properties.items():
            try:
                prefix, property_name = k.split(':')
                merge_dict(current_properties, {prefix: {property_name: property_value}})
            except ValueError:
                raise ParseConfigError('property keys must have format like `prefix:property_name`')

    @property
    def initialization_actions(self) -> List[Dict]:
        return self._hadoop.get('initialization_actions', [])

    @initialization_actions.setter
    def initialization_actions(self, initialization_actions: List[Dict]) -> None:
        init_acts = []
        for init_act in initialization_actions:
            new = {'uri': init_act['uri']}
            if init_act['timeout']:
                new['timeout'] = init_act['timeout']
            if init_act['args']:
                new['args'] = init_act['args']
            init_acts.append(new)
        self._hadoop['initialization_actions'] = init_acts

    @property
    def config(self) -> Dict:
        """Get cluster config"""
        rez = deepcopy(self._hadoop)
        if self.services:
            rez['services'] = self.services
        rez['properties'] = self.properties
        return rez

    @property
    def monitoring(self) -> dict:
        """
        Get env-dependent monitoring config properties
        """
        return self._hadoop.get('monitoring', {})

    @property
    def logging(self) -> dict:
        return self._hadoop.get('logging', {})

    @property
    def logs_in_object_storage(self) -> Optional[bool]:
        """
        Temporary flag to mark old clusters that do not send logs to default log group.
        To be removed after full transition to cloud loggging
        """
        return self._hadoop.get('logs_in_object_storage')

    @property
    def log_group_id(self) -> Optional[str]:
        return self._hadoop.get('logging', {}).get('group_id')

    @log_group_id.setter
    def log_group_id(self, log_group_id: Optional[str]) -> None:
        if 'logging' not in self._hadoop:
            self._hadoop['logging'] = {}
        self._hadoop['logging']['group_id'] = log_group_id

    @property
    def logging_service_url(self) -> Optional[str]:
        return self._hadoop.get('logging', {}).get('url')

    @logging_service_url.setter
    def logging_service_url(self, url: str) -> None:
        if 'logging' not in self._hadoop:
            self._hadoop['logging'] = {}
        self._hadoop['logging']['url'] = url

    @property
    def agent(self) -> dict:
        """
        Get env-dependent agent config properties
        """
        return self._hadoop.get('agent', {})

    @property
    def agent_cid(self) -> str:
        """Get agent cid"""
        return self._hadoop['agent']['cid']

    @agent_cid.setter
    def agent_cid(self, cid: str) -> None:
        """Set agent cid for cluster pillar"""
        if cid:
            if not self._hadoop.get('agent'):
                self._hadoop['agent'] = {'cid': cid}
            else:
                self._hadoop['agent']['cid'] = cid

    @property
    def semantic_version(self) -> str:
        """Get cluster semantic version"""
        version = self._hadoop['version']
        if len(version.split('.')) == 2:
            return f'{version}.0'
        else:
            return version

    @property
    def version_prefix(self) -> str:
        """Get user requested version"""
        return self._hadoop.get('version_prefix', self._hadoop['version'])

    @version_prefix.setter
    def version_prefix(self, version_prefix: Optional[str]) -> None:
        """Set user requested version for cluster"""
        self._hadoop['version_prefix'] = version_prefix

    @property
    def version(self) -> str:
        """Get cluster version"""
        return self._hadoop['version']

    @version.setter
    def version(self, version: Optional[str]) -> None:
        """Set image version for cluster"""
        if not version:
            version = current_app.config['HADOOP_DEFAULT_IMAGE']
        self._hadoop['version'] = version

    @property
    def schema_version(self) -> str:
        """Get schema version. Also a major part of a version_id"""
        return self._hadoop['version'].split('.')[0]

    @property
    def image(self) -> Optional[str]:
        """Get cluster image"""
        return self._hadoop.get('image')

    @image.setter
    def image(self, image: Optional[str]) -> None:
        self._hadoop['image'] = image

    @property
    def ssh_public_keys(self) -> List[str]:
        """Get cluster ssh_public_keys"""
        return self._pillar.get('ssh_authorized_keys', self._hadoop.get('ssh_public_keys', []))

    @ssh_public_keys.setter
    def ssh_public_keys(self, ssh_public_keys: Optional[List[str]]) -> None:
        """Set public ssh keys for cluster"""
        if ssh_public_keys:
            # set ssh pulbic keys for dataproc 1.x without cloud-init
            self._hadoop['ssh_public_keys'] = ssh_public_keys
            # set ssh public keys for dataproc 2.x with cloud-init
            self._pillar['ssh_authorized_keys'] = ssh_public_keys

    @property
    def zone_id(self) -> str:
        """Get cluster zoneId"""
        return self._hadoop['topology']['zone_id']

    @zone_id.setter
    def zone_id(self, zone_id: str) -> None:
        """Set zoneId for cluster"""
        if zone_id:
            self._hadoop['topology']['zone_id'] = zone_id

    @property
    def service_account_id(self) -> str:
        """Get cluster ServiceAccountId"""
        return self._hadoop['service_account_id']

    @service_account_id.setter
    def service_account_id(self, service_account_id: str) -> None:
        """Set ServiceAccountId for cluster"""
        if service_account_id:
            self._hadoop['service_account_id'] = service_account_id

    @property
    def _subclusters(self) -> Dict:
        """Get all subclusters (with master)"""
        return self._hadoop['topology']['subclusters']

    @property
    def subclusters(self) -> List[Dict]:
        """Get subclusters without master subcluster"""
        return [sub for sub in self._subclusters.values() if sub.get('role') != MASTER_SUBCLUSTER_TYPE]

    def get_subcluster(self, subcid: str) -> Optional[Dict]:
        """Get subcluster by subcid"""
        return deepcopy(self._subclusters.get(subcid))

    def set_subcluster(self, subcluster: Dict) -> None:
        """Add or set subcluster in pillar"""
        subcid = subcluster.get('subcid')
        if not subcid:
            raise ParseConfigError('Subcluster must contain subcid')
        self._subclusters[subcid] = deepcopy(subcluster)
        if subcluster['role'] == MASTER_SUBCLUSTER_TYPE:
            self._hadoop['subcluster_main_id'] = subcid
        self.update_topology_revision()

    @property
    def network_id(self) -> str:
        """Get network ID"""
        return self._hadoop['topology']['network_id']

    def set_network_id(self, network_id) -> None:
        """Add or set subcluster in pillar"""
        self._hadoop['topology']['network_id'] = network_id
        self.update_topology_revision()

    def remove_subcluster(self, subcid: str) -> None:
        """Remove subcluster with provided subcid from pillar"""
        self._subclusters.pop(subcid, None)
        self.labels.pop(subcid, None)
        self.update_topology_revision()

    @property
    def subclusters_names(self) -> List[str]:
        """Return list if subclusters names"""
        return [sub['name'] for sub in self._subclusters.values()]

    @property
    def subcluster_main(self) -> Dict:
        """Get main subcluster"""
        subcid = self._hadoop.get('subcluster_main_id')
        return self._subclusters[subcid]

    @property
    def topology_revision(self) -> int:
        """Return current revision of pillar"""
        return self._hadoop['topology']['revision']

    def update_topology_revision(self) -> None:
        """Sets new revision for cluster pillar. Call after modify or create"""
        prev = self._hadoop['topology'].get('revision', 0)
        self._hadoop['topology']['revision'] = prev + 1

    @property
    def labels(self) -> Dict:
        """Get labels"""
        return self._hadoop.get('labels')

    @labels.setter
    def labels(self, labels: Dict) -> None:
        """Set labels for cluster"""
        if labels:
            self._hadoop['labels'] = labels

    @property
    def ui_proxy(self) -> bool:
        """
        Whether UI Proxy feature is enabled
        """
        return self._hadoop.get('ui_proxy', False)

    @ui_proxy.setter
    def ui_proxy(self, enabled: bool) -> None:
        """
        Enable or disable UI proxy feature
        """
        self._hadoop['ui_proxy'] = enabled

    @property
    def bypass_knox(self) -> bool:
        """
        Whether UI Proxy should send requests bypassing Knox
        """
        default = semver.VersionInfo.parse(self.semantic_version).major > 1
        return self._hadoop.get('bypass_knox', default)

    def is_lightweight(self) -> bool:
        """
        Returns true for cluster without datanode
        """
        return ClusterService.hdfs not in self.services


def get_cluster_pillar(cluster: Any) -> HadoopPillar:
    """
    Return Hadoop pillar by cluster
    """
    if isinstance(cluster, Dict):
        return HadoopPillar(cluster['value'])
    elif isinstance(cluster, ClusterInfo):
        return HadoopPillar(cluster.value)
    raise DbaasError('Unknown type of cluster object for creating HadoopPillar: {type(cluster)}')
