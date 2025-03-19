import abc
import logging
from typing import Optional
from typing import Tuple

import vh
from cloud.dwh.nirvana.vh.common.operations import get_default_clickhouse_query_op
from cloud.dwh.nirvana.vh.common.operations import get_default_get_mr_table_op
from cloud.dwh.nirvana.vh.common.operations import get_default_python_to_tsv_json_text_op
from cloud.dwh.nirvana.vh.common.operations import get_default_single_option_to_text_output_op
from cloud.dwh.nirvana.vh.common.operations import get_default_yt_to_clickhouse_op
from cloud.dwh.nirvana.vh.common.workflows.base import ReactiveWorkflowBase
from cloud.dwh.nirvana.vh.config.base import BILLING_CLICKHOUSE_CLUSTER_ID
from cloud.dwh.nirvana.vh.config.base import BILLING_CLICKHOUSE_PASSWORD
from cloud.dwh.nirvana.vh.config.base import BILLING_CLICKHOUSE_USER
from cloud.dwh.nirvana.vh.config.base import BaseDeployConfig as DeployConfig
from library.python import resource

LOG = logging.getLogger(__name__)


class YtToClickHouseReactiveWorkflowBase(ReactiveWorkflowBase, abc.ABC):
    """
    Download data from YT and upload it to ClickHouse.
    1. Create distributed table system.tablesD
    2. Select all trash table names
    3. Drop them
    4. Uploads data to temporary table and then renames it.
    5. Creates distributed table on cluster if it not exists

    Parameters (from file `parameters.json`):
    * `source_table` - source table path in YT
    * `destination_table` - destination table name in ClickHouse (with prefix, e.g. `billing.cube_prod`)
    * `primary_key` - primary key for result table in ClickHouse (separated with comma, e.g. `field1,field2,field3`)
    * `partition_key` - partition key for result table in ClickHouse (separated with comma, e.g. `field1,field2,field3`)
    """

    @abc.abstractmethod
    def _get_mdb_cluster_id(self) -> str:
        """Destination ClickHouse cluster id"""
        pass

    @abc.abstractmethod
    def _get_mdb_cluster_host(self) -> str:
        """Destination ClickHouse cluster host (without port, e.g. `c-mdb4rd721uh94u42qlf1.rw.db.yandex.net`)"""
        pass

    @abc.abstractmethod
    def _get_clickhouse_user(self) -> str:
        """Destination ClickHouse user"""
        pass

    @abc.abstractmethod
    def _get_clickhouse_password(self) -> str:
        """Name of secret with ClickHouse password"""
        pass

    def _get_clickhouse_params(self) -> dict:
        return {
            'ch-host': self._get_mdb_cluster_host(),
            'ch-user': self._get_clickhouse_user(),
            'ch-password': self._get_clickhouse_password()
        }

    def _get_recreate_distributed_table_query(self, destination_table: str) -> str:
        database, table_name = destination_table.split('.')
        query = "DROP TABLE IF EXISTS {database}.{table}D \n" \
                "ON CLUSTER {cluster_id}; \n" \
                "CREATE TABLE {database}.{table}D \n" \
                "ON CLUSTER '{cluster_id}' \n" \
                "AS {database}.{table} \n" \
                "ENGINE = Distributed('{cluster_id}', '{database}', '{table}'); \n"
        return query.format(database=database, table=table_name, cluster_id=self._get_mdb_cluster_id())

    def _get_select_trash_tables_query(self, destination_table: str) -> str:
        database, table_name = destination_table.split('.')
        query = "SELECT DISTINCT database || '.' || name  \n" \
                "FROM clusterAllReplicas('{cluster_id}', system.tables) \n" \
                "WHERE database = '{database}' \n" \
                "AND name LIKE '{table_name}\\_%' \n" \
                "AND metadata_modification_time < yesterday()"
        return query.format(database=database, table_name=table_name.replace("_", "\\_"), cluster_id=self._get_mdb_cluster_id())

    def _get_drop_query_python_generator(self) -> str:
        return resource. \
            find('common/resources/generate_drop_tables_query.py'). \
            decode('utf-8'). \
            format(cluster_id=self._get_mdb_cluster_id())

    def _get_artifact_success_result(self, deploy_config: DeployConfig) -> Optional[str]:
        """
        Reaction artifact success result. Default to `parameters.destination_path`
        """
        return self._get_environment_parameters(environment=deploy_config.environment)['destination_table']

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

        # 2. select trash tables
        select_trash_query_op = get_default_single_option_to_text_output_op(deploy_config)
        select_trash_query_result = select_trash_query_op(
            _options={
                'input': self._get_select_trash_tables_query(parameters['destination_table'])
            },
        )

        select_trash_op = get_default_clickhouse_query_op(deploy_config)
        select_trash_result = select_trash_op(
            _inputs={
                'query': select_trash_query_result['output']
            },
            _options=self._get_clickhouse_params()
        )

        # 3. drop trash tables
        python_drop_query_op = get_default_python_to_tsv_json_text_op(deploy_config)
        python_drop_query_result = python_drop_query_op(
            _inputs={
                'files': [select_trash_result['output']]
            },
            _options={
                'code': self._get_drop_query_python_generator()
            }
        )

        drop_trash_op = get_default_clickhouse_query_op(deploy_config)
        drop_trash_result = drop_trash_op(
            _inputs={
                'query': python_drop_query_result['output_text']
            },
            _options=self._get_clickhouse_params()
        )

        # 4. export YT to CH
        input_mr_table_op = get_default_get_mr_table_op(deploy_config)
        input_mr_table_op_result = input_mr_table_op(
            _name='Input table',
            _options={
                'table': parameters['source_table'],
            },
            _after=[drop_trash_result]
        )

        yt_to_clickhouse_op = get_default_yt_to_clickhouse_op(deploy_config)
        yt_to_clickhouse_result = yt_to_clickhouse_op(
            _inputs={
                'yt_table': input_mr_table_op_result['outTable'],
            },
            _options={
                'mdb_cluster_id': self._get_mdb_cluster_id(),
                'ch_user': self._get_clickhouse_user(),
                'ch_password': self._get_clickhouse_password(),
                'ch_table': parameters['destination_table'],
                'ch_primary_key': parameters['primary_key'],
                'ch_partition_key': parameters['partition_key'],
            },
        )

        # 5. Create distributed table

        create_query_op = get_default_single_option_to_text_output_op(deploy_config)
        create_query_result = create_query_op(
            _options={
                'input': self._get_recreate_distributed_table_query(parameters['destination_table'])
            },
            _after=[yt_to_clickhouse_result]
        )

        create_op = get_default_clickhouse_query_op(deploy_config)
        create_op(
            _inputs={
                'query': create_query_result['output']
            },
            _options=self._get_clickhouse_params()
        )


class YtToBillingClickHouseReactiveWorkflowBase(YtToClickHouseReactiveWorkflowBase):

    @abc.abstractmethod
    def _get_workflow_tags(self) -> Tuple[str, ...]:
        pass

    def _get_mdb_cluster_id(self) -> str:
        return BILLING_CLICKHOUSE_CLUSTER_ID

    def _get_clickhouse_user(self) -> str:
        return BILLING_CLICKHOUSE_USER

    def _get_clickhouse_password(self) -> str:
        return BILLING_CLICKHOUSE_PASSWORD

    def _get_mdb_cluster_host(self) -> str:
        return "c-{}.rw.db.yandex.net".format(self._get_mdb_cluster_id())
