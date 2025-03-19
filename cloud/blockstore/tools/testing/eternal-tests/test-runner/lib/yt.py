from dataclasses import dataclass
import os
import yt.wrapper as yt

from .errors import Error


@dataclass
class Instance:
    id: str
    ip: str
    compute_node: str


class YtHelper:
    _YT_CLUSTER = 'hahn'
    _YT_PATH_PREFIX = '//home/cloud-nbs/vm-configs-for-load-tests/'

    _YT_VM_AND_DISK_CONFIGURATION = [
        {'name': 'compute_node', 'type': 'string'},
        {'name': 'instance_id', 'type': 'string'},
        {'name': 'instance_ip', 'type': 'string'},
        {'name': 'disk_id', 'type': 'string'},
        {'name': 'disk_type', 'type': 'string'},
        {'name': 'disk_size', 'type': 'uint64'},
        {'name': 'disk_bs', 'type': 'uint64'},
    ]

    def __init__(self, test_suite: str, cluster: str, logger):
        self._test_suite = test_suite
        self._cluster = cluster
        self._logger = logger

        self._yt_result_table_dir_path = os.path.join(self._YT_PATH_PREFIX, test_suite, cluster, "config")
        yt_oauth_token = os.getenv('YT_OAUTH_TOKEN')
        if yt_oauth_token is None:
            raise Error('no YT_OAUTH_TOKEN specified')

        yt.config['proxy']['url'] = self._YT_CLUSTER
        yt.config['token'] = yt_oauth_token

    def init_structure(self) -> None:
        self._logger.info('Initializing YT structure')
        yt.create('table',
                  self._yt_result_table_dir_path,
                  recursive=True,
                  ignore_existing=True,
                  attributes={'schema': self._YT_VM_AND_DISK_CONFIGURATION})

    def fetch_config(self) -> dict:
        self._logger.info('Fetching config from YT')
        for row in yt.read_table(self._yt_result_table_dir_path, format='json'):
            return {
                'id': row['instance_id'],
                'network_interfaces': [{'primary_v6_address': {'address': row['instance_ip']}}],
                'compute_node': row['compute_node']
            }
        raise Error('Cannot find test config on YT')

    def publish_config(self, instance, disk, disk_cfg):
        row = {
            'compute_node': instance.compute_node,
            'instance_id': instance.id,
            'instance_ip': instance.ip,
            'disk_id': disk.id,
            'disk_type': disk_cfg.type,
            'disk_size': disk_cfg.size,
            'disk_bs': disk_cfg.bs,
        }
        yt.write_table(yt.TablePath(self._yt_result_table_dir_path, append=False), [row], format='json')
