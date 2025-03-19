import abc
import json
import logging

from library.python import resource

from cloud.dwh.nirvana.vh.common.operations import get_default_python_to_tsv_json_text_op
from cloud.dwh.nirvana.vh.common.operations import get_default_single_option_to_json_output_op
from cloud.dwh.nirvana.vh.common.operations import get_default_single_option_to_text_output_op
from cloud.dwh.nirvana.vh.common.operations import get_default_yql_with_json_output_op
from cloud.dwh.nirvana.vh.common.operations import get_push_to_solomon_op
from cloud.dwh.nirvana.vh.common.workflows.base import ReactiveWorkflowBase
from cloud.dwh.nirvana.vh.config.base import BaseDeployConfig as DeployConfig

LOG = logging.getLogger(__name__)


class LagMonitoringWorkflowBase(ReactiveWorkflowBase, abc.ABC):

    def maybe_add_lag_monitor(self, deploy_config: DeployConfig, parameters: dict, after=None):
        if "lag_monitor" not in parameters:
            return

        parameters["lag_monitor"]["table"] = parameters["lag_monitor"].get("table") or parameters["destination_path"]
        parameters = parameters["lag_monitor"]

        # 1. yql-select metric
        yql_op = get_default_yql_with_json_output_op(deploy_config)
        yql_op_result = yql_op(
            _name='Lag Monitor. Run query',
            _options={
                'query': self._get_monitoring_yql_query(deploy_config.yt_cluster,
                                                        parameters["table"], parameters["dttm_column"]),
            },
            _after=[after] if after else None
        )

        # 2. Prepare labels for solomon metric
        option_to_json_op = get_default_single_option_to_json_output_op(deploy_config)
        solomon_labels = option_to_json_op(
            _name='Lag Monitor. Prepare labels',
            _options={
                "input": json.dumps(self._get_solomon_labels(deploy_config))
            }
        )

        # 3. Timezone as input
        text_op = get_default_single_option_to_text_output_op(deploy_config)
        text_op = text_op(_options={"input": parameters["timezone"]})

        # 4. Prepare solomon metric
        python_op = get_default_python_to_tsv_json_text_op(deploy_config)
        solomon_metric = python_op(
            _inputs={
                'files': [yql_op_result['output'], solomon_labels["output"], text_op["output"]]
            },
            _options={
                'code': self._get_solomon_metric_generator()
            }
        )

        # 5. Send metric to solomon
        push_to_solomon_op = get_push_to_solomon_op(deploy_config)
        push_to_solomon_op(
            _name='Lag Monitor. Send to Solomon',
            _inputs={
                'sensors': solomon_metric['output_json']
            }
        )

    @staticmethod
    def _get_solomon_labels(deploy_config: DeployConfig):
        return {
            "layer": deploy_config.module.split('.')[0],
            "workflow_name": '.'.join(deploy_config.module.split('.')[1:]),
            "sensor": "lag_by_dttm_column"
        }

    @staticmethod
    def _get_monitoring_yql_query(cluster, table_path, column_name) -> str:
        return f'USE {cluster};\n' \
               f'$table = "{table_path}";\n' \
               '$parse_date = DateTime::Parse("%Y-%m-%d");\n' \
               '$get_ts_from_datetime = ($dttm) -> (Datetime::ToMilliseconds($dttm));\n' \
               '$get_ts_from_date_string = ($dt) -> (DateTime::ToMilliseconds(DateTime::MakeDatetime($parse_date($dt))));\n' \
               '\n' \
               'SELECT DateTime::ToSeconds(CurrentUtcTimestamp()) AS now,\n' \
               '       now_ms - ' \
               '       LEAST(CASE FormatType(TypeOf(max_value))\n' \
               '        WHEN "Optional<Timestamp>"      THEN $get_ts_from_datetime(cast(max_value as Datetime))\n' \
               '        WHEN "Uint32?"                  THEN CAST(max_value AS Uint64) * 1000 \n' \
               '        WHEN "Optional<Uint32>"         THEN CAST(max_value AS Uint64) * 1000 \n' \
               '        WHEN "Int64?"                   THEN CAST(max_value AS Uint64) * 1000 \n' \
               '        WHEN "Optional<Int64>"          THEN CAST(max_value AS Uint64) * 1000 \n' \
               '        WHEN "String?"                  THEN $get_ts_from_date_string(CAST(max_value AS String))\n' \
               '        WHEN "Optional<String>"         THEN $get_ts_from_date_string(CAST(max_value AS String))\n' \
               '        WHEN "Datetime?"                THEN $get_ts_from_datetime(CAST(max_value AS Datetime))\n' \
               '        WHEN "Optional<Datetime>"       THEN $get_ts_from_datetime(CAST(max_value AS Datetime))\n' \
               '        WHEN "TzDatetime?"              THEN $get_ts_from_datetime(CAST(max_value AS TzDatetime))\n' \
               '        WHEN "Optional<TzDatetime>"     THEN $get_ts_from_datetime(CAST(max_value AS TzDatetime))\n' \
               '        WHEN "Date?"                    THEN $get_ts_from_datetime(CAST(max_value AS Date))\n' \
               '        WHEN "Optional<Date>"           THEN $get_ts_from_datetime(CAST(max_value AS Date))\n' \
               '       ELSE ENSURE(1u, 1=0, "unexpected type of column")\n' \
               '       END, now_ms) AS delta\n' \
               f'FROM (SELECT MAX(`{column_name}`) AS max_value, DateTime::ToMilliseconds(CurrentUtcTimestamp()) AS now_ms FROM $table);'

    @staticmethod
    def _get_solomon_metric_generator() -> str:
        return resource.find('common/resources/generate_solomon_metric_for_lag_monitoring.py').decode('utf-8')
