import os
import yt.wrapper as yt

from .errors import Error


class YtHelper:
    _YT_CLUSTER = 'hahn'
    _YT_PATH_PREFIX = '//home/cloud-nbs/fio-performance-test-suite/'

    _YT_TEST_REPORT_TABLE_SCHEMA = [
        {'name': 'disk_id', 'type': 'string'},
        {'name': 'disk_type', 'type': 'string'},
        {'name': 'disk_size', 'type': 'uint64'},
        {'name': 'iodepth', 'type': 'uint16'},
        {'name': 'read_iops', 'type': 'double'},
        {'name': 'read_bw', 'type': 'uint64'},
        {'name': 'read_clat', 'type': 'double'},
        {'name': 'write_iops', 'type': 'double'},
        {'name': 'write_bw', 'type': 'uint64'},
        {'name': 'write_clat_mean', 'type': 'double'},
    ]
    _YT_ANNOUNCES_TABLE_SCHEMA = [
        {'name': 'test_suite', 'type': 'string'},
        {'name': 'cluster', 'type': 'string'},
        {'name': 'version', 'type': 'string'},
        {'name': 'date', 'type': 'string'},
    ]

    def __init__(self, test_suite: str, cluster: str, nbs_version: str, date: str, logger):
        self._test_suite = test_suite
        self._cluster = cluster
        self._nbs_version = nbs_version
        self._date = date
        self._logger = logger

        self._yt_result_table_dir_path = os.path.join(self._YT_PATH_PREFIX, test_suite, cluster, nbs_version)
        self._yt_result_table_path = os.path.join(self._yt_result_table_dir_path, date)
        self._yt_latest_result_table_symlink_path = os.path.join(self._yt_result_table_dir_path, 'latest')
        self._yt_latest_version_symlink_path = os.path.join(self._YT_PATH_PREFIX, test_suite, cluster, 'latest')
        self._yt_announces_table_path = os.path.join(self._YT_PATH_PREFIX, 'announces')

        yt_oauth_token = os.getenv('YT_OAUTH_TOKEN')
        if yt_oauth_token is None:
            raise Error('no YT_OAUTH_TOKEN specified')

        yt.config['proxy']['url'] = self._YT_CLUSTER
        yt.config['token'] = yt_oauth_token

    def init_structure(self) -> None:
        self._logger.info('Initializing YT structure')
        yt.create('map_node',
                  self._yt_result_table_dir_path,
                  recursive=True,
                  ignore_existing=True)
        yt.create('table',
                  self._yt_result_table_path,
                  ignore_existing=True,
                  attributes={'schema': self._YT_TEST_REPORT_TABLE_SCHEMA})
        yt.create('table',
                  self._yt_announces_table_path,
                  ignore_existing=True,
                  attributes={'schema': self._YT_ANNOUNCES_TABLE_SCHEMA})

    def publish_test_report(self, test_config: dict, test_case_report_read: dict, test_report_write: dict) -> None:
        self._logger.info('Publishing test report of test worst case read performance to YT')
        row = {
            'disk_id': test_config['disk_id'],
            'disk_size': test_config['disk_size'],
            'disk_type': 'network-ssd',
            'iodepth': test_config['iodepth'],
            'read_iops': test_case_report_read['read_iops'],
            'read_bw': test_case_report_read['read_bw'],
            'read_clat': test_case_report_read['read_clat'],
            'write_iops': test_report_write['jobs'][0]['write']['iops'],
            'write_bw': test_report_write['jobs'][0]['write']['bw'],
            'write_clat_mean': test_report_write['jobs'][0]['write']['clat']['mean'],
        }
        yt.write_table(yt.TablePath(self._yt_result_table_path, append=True), [row], format='json')

    def announce_test_run(self) -> None:
        self._logger.info('Announcing test run for dashboard')
        row = {
            'test_suite': self._test_suite,
            'cluster': self._cluster,
            'version': self._nbs_version,
            'date': self._date,
        }
        yt.write_table(yt.TablePath(self._yt_announces_table_path, append=True), [row], format='json')
        yt.link(self._yt_result_table_path, self._yt_latest_result_table_symlink_path, force=True)
        self._logger.info(f'{self._yt_result_table_dir_path}, {self._yt_latest_version_symlink_path}')
        yt.link(self._yt_result_table_dir_path, self._yt_latest_version_symlink_path, force=True)
