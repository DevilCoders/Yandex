import os
import random
import time
from typing import Set
import yt.wrapper as yt

from .test_cases import TestCase
from .errors import Error


class YtHelper:

    _YT_CLUSTER = 'hahn'
    _YT_PATH_PREFIX = '//home/cloud-nbs/fio-performance-test-suite/'
    _YT_AGGREGATED_PATH_PREFIX = '//home/cloud-nbs/aggregated-fio-results/'

    _YT_TEST_REPORT_TABLE_SCHEMA = [
        {'name': 'compute_node', 'type': 'string'},
        {'name': 'disk_id', 'type': 'string'},
        {'name': 'disk_type', 'type': 'string'},
        {'name': 'disk_size', 'type': 'uint64'},
        {'name': 'disk_bs', 'type': 'uint64'},
        {'name': 'iodepth', 'type': 'uint16'},
        {'name': 'bs', 'type': 'uint64'},
        {'name': 'rw', 'type': 'string'},
        {'name': 'read_iops', 'type': 'double'},
        {'name': 'read_bw', 'type': 'uint64'},
        {'name': 'read_clat_mean', 'type': 'double'},
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
    _YT_AGGREGATED_TABLE_SCHEMA = [
        {'name': 'version', 'type': 'string'},
        {'name': 'date', 'type': 'string'},
        {'name': 'compute_node', 'type': 'string'},
        {'name': 'disk_id', 'type': 'string'},
        {'name': 'disk_type', 'type': 'string'},
        {'name': 'disk_size', 'type': 'uint64'},
        {'name': 'disk_bs', 'type': 'uint64'},
        {'name': 'iodepth', 'type': 'uint16'},
        {'name': 'bs', 'type': 'uint64'},
        {'name': 'rw', 'type': 'string'},
        {'name': 'read_iops', 'type': 'double'},
        {'name': 'read_bw', 'type': 'uint64'},
        {'name': 'read_clat_mean', 'type': 'double'},
        {'name': 'write_iops', 'type': 'double'},
        {'name': 'write_bw', 'type': 'uint64'},
        {'name': 'write_clat_mean', 'type': 'double'},
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
        self._yt_aggregated_results_table_path = os.path.join(self._YT_AGGREGATED_PATH_PREFIX, test_suite, cluster)

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
        yt.create('table',
                  self._yt_aggregated_results_table_path,
                  recursive=True,
                  ignore_existing=True,
                  attributes={'schema': self._YT_AGGREGATED_TABLE_SCHEMA})

    def fetch_completed_test_cases(self) -> Set[TestCase]:
        self._logger.info('Fetching completed test cases from YT')
        return {
            TestCase(
                row['disk_type'],
                row['disk_size'],
                row['disk_bs'],
                row['rw'],
                row['bs'],
                row['iodepth'],
                '/dev/vdb',
                50,
            )
            for row in yt.read_table(self._yt_result_table_path, format='json')
        }

    def publish_test_report(self, compute_node: str, disk_id: str, test_case: TestCase, fio_report: dict) -> None:
        self._logger.info(f'Publishing test report of test case <{test_case.name}> to YT')
        row = {
            'compute_node': compute_node,
            'disk_id': disk_id,
            'disk_size': test_case.disk_size,
            'disk_type': test_case.disk_type,
            'disk_bs': test_case.disk_bs,
            'rw': test_case.rw,
            'bs': test_case.bs,
            'iodepth': test_case.iodepth,
            'read_iops': fio_report['jobs'][0]['read']['iops'],
            'read_bw': fio_report['jobs'][0]['read']['bw'],
            'read_clat_mean': fio_report['jobs'][0]['read']['clat']['mean'],
            'write_iops': fio_report['jobs'][0]['write']['iops'],
            'write_bw': fio_report['jobs'][0]['write']['bw'],
            'write_clat_mean': fio_report['jobs'][0]['write']['clat']['mean'],
        }
        self.yt_retry(lambda: yt.write_table(
            yt.TablePath(self._yt_result_table_path, append=True),
            [row],
            format='json'
        ))
        additional = {
            'version': self._nbs_version,
            'date': self._date
        }
        row = {**additional, **row}
        self.yt_retry(lambda: yt.write_table(
            yt.TablePath(self._yt_aggregated_results_table_path, append=True),
            [row],
            format='json'
        ))

    def announce_test_run(self) -> None:
        self._logger.info('Announcing test run for dashboard')
        row = {
            'test_suite': self._test_suite,
            'cluster': self._cluster,
            'version': self._nbs_version,
            'date': self._date,
        }
        self.yt_retry(lambda: yt.write_table(
            yt.TablePath(self._yt_announces_table_path, append=True),
            [row],
            format='json'
        ))
        yt.link(self._yt_result_table_path, self._yt_latest_result_table_symlink_path, force=True)
        yt.link(self._yt_result_table_dir_path, self._yt_latest_version_symlink_path, force=True)

    def finalize_tables(self) -> None:
        tabs = [
            self._yt_result_table_path,
            self._yt_announces_table_path,
            self._yt_aggregated_results_table_path,
        ]

        for tab in tabs:
            self.yt_retry(lambda: yt.run_merge(tab, tab, spec={"combine_chunks": True}))

    def yt_retry(self, op) -> None:
        self.yt_retry_impl(op, 10, 1)

    def yt_retry_impl(self, op, attempts, delay) -> None:
        try:
            op()
        except yt.YtError as e:
            retriable_codes = [211, 402]
            retriable = e.find_matching_error(
                predicate=lambda err: int(err.code) in retriable_codes
            ) is not None

            if attempts == 1 or not retriable:
                raise

            time.sleep(delay)
            self.yt_retry_impl(
                op,
                attempts - 1,
                delay + random.uniform(delay, 4 * delay) / 2)


class YtHelperStub:

    def init_structure(*args) -> None:
        pass

    def fetch_completed_test_cases(*args) -> Set[TestCase]:
        return set()

    def publish_test_report(*args) -> None:
        pass

    def announce_test_run(*args) -> None:
        pass

    def finalize_tables(self) -> None:
        pass
