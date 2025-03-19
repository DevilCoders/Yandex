import abc
import logging
from datetime import timedelta
from typing import Optional

import vh

from cloud.dwh.nirvana.vh.common.operations import get_default_get_mr_directory_op
from cloud.dwh.nirvana.vh.common.operations import get_default_solomon_to_yt_op
from cloud.dwh.nirvana.vh.common.workflows.base import ReactiveWorkflowBase
from cloud.dwh.nirvana.vh.config.base import BaseDeployConfig as DeployConfig
from cloud.dwh.utils import datetimeutils

LOG = logging.getLogger(__name__)


class SolomonToYtReactiveWorkflowBase(ReactiveWorkflowBase, abc.ABC):
    """
    Download data from solomon and upload it to YT.
    Create daily tables for all available days without current day and hourly tables for last two days.

    Parameters (from file `parameters.yaml`):
    * `solomon_project` - solomon project
    * `solomon_selectors` - solomon selectors (e.g. `{cluster='prod', service='api'}`)
    * `solomon_start_datetime` - start time for solomon
    * `destination_path` - YT destination folder
    """

    YT_HOURLY_TABLE_TTL = datetimeutils.DAY * 2 * 1000

    def _get_yt_hourly_table_ttl(self) -> str:
        """
        TTL for hourly tables in milliseconds. Default to 2 days
        """
        return self.YT_HOURLY_TABLE_TTL

    def _get_artifact_success_result(self, deploy_config: DeployConfig) -> Optional[str]:
        """
        Reaction artifact success result. Default to `parameters.destination_path`
        """
        return self._get_environment_parameters(environment=deploy_config.environment)['destination_path']

    def _get_artifact_failure_result(self, deploy_config: DeployConfig) -> Optional[str]:
        """
        Reaction artifact failure result. Default to None
        """
        return None

    def _fill_graph(self, graph: vh.Graph, deploy_config: DeployConfig):
        """
        Fill graph with operations.
        """
        parameters = self._get_environment_parameters(environment=deploy_config.environment)

        output_mr_directory_op = get_default_get_mr_directory_op(deploy_config)
        output_mr_directory_op_result = output_mr_directory_op(
            _options={
                'path': parameters['destination_path'],
            },
        )

        from_dttm_daily = datetimeutils.parse_isoformat_to_msk_dttm(parameters['solomon_start_datetime'])
        from_dttm_daily = from_dttm_daily.replace(microsecond=0)

        daily_solomon_to_yt_op = get_default_solomon_to_yt_op(deploy_config)
        daily_solomon_to_yt_op(
            _name='Daily',
            _inputs={
                'dst': output_mr_directory_op_result['mr_directory'],
            },
            _options={
                'solomon-project': parameters['solomon_project'],
                'solomon-selectors': parameters['solomon_selectors'],
                'solomon-partition-by-labels': parameters['solomon_partition_by_label'],
                'cpu-metric-flag': parameters.get('cpu_metric_flag', False),
                'solomon-default-from-dttm': from_dttm_daily.isoformat(),
                'yt-tables-split-interval': 'DAILY',
            },
        )

        from_dttm_hourly = max(datetimeutils.get_now_msk().replace() - timedelta(milliseconds=self.YT_HOURLY_TABLE_TTL), from_dttm_daily)
        from_dttm_hourly = from_dttm_hourly.replace(microsecond=0)

        hourly_solomon_to_yt_op = get_default_solomon_to_yt_op(deploy_config)
        hourly_solomon_to_yt_op(
            _name='Hourly',
            _inputs={
                'dst': output_mr_directory_op_result['mr_directory'],
            },
            _options={
                'solomon-project': parameters['solomon_project'],
                'solomon-selectors': parameters['solomon_selectors'],
                'solomon-partition-by-labels': parameters['solomon_partition_by_label'],
                'cpu-metric-flag': parameters.get('cpu_metric_flag', False),
                'solomon-default-from-dttm': from_dttm_hourly.isoformat(),
                'yt-tables-split-interval': 'HOURLY',
                'yt-table-ttl': self._get_yt_hourly_table_ttl(),
            },
        )
