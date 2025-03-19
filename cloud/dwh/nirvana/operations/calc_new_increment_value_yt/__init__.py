from datetime import datetime

import pytz

from cloud.dwh.lms.controllers.increment_controller import IncrementController
from cloud.dwh.lms.models.metadata import LMSDBLoadMetadataInc, LMSIncrementType
from nirvana import job_context as nv
from dataclasses import dataclass
import cloud.dwh.lms.config as lms_config
from yql.api.v1.client import YqlClient
import json
import logging


@dataclass
class MaxValue:
    value: int
    yt_type: str


def get_max_value(cluster, yql_token, table, column):
    query = f'select max(`{column}`) as max_value from `{table}`'
    logging.info(f"Executing query: {query}")
    yql_client = YqlClient(db=cluster, token=yql_token)
    request = yql_client.query(query, syntax_version=1)
    request.run()
    table = request.table
    column_type = table.column_print_types[0]
    max_value = table.rows[0][0]
    if max_value:
        return MaxValue(value=max_value, yt_type=column_type)


def max_value_to_string(value: MaxValue, metadata: LMSDBLoadMetadataInc):
    ic = IncrementController(metadata=metadata)
    if value.yt_type.startswith("Int"):
        if metadata.increment_type == LMSIncrementType.Datetime:
            new_inc_value = datetime.fromtimestamp(value.value, pytz.utc)
        elif metadata.increment_type == LMSIncrementType.Date:
            dttm = datetime.fromtimestamp(value.value, pytz.utc)
            new_inc_value = datetime(year=dttm.year, month=dttm.month, day=dttm.day, tzinfo=pytz.utc)
        elif metadata.increment_type == LMSIncrementType.Integer:
            new_inc_value = int(value.value)
        else:
            raise ValueError("Invalid increment type")
        return ic.increment_value_to_string(new_inc_value)
    raise ValueError("Increment field in YT must be of an Int type")


def main():
    logging.getLogger().setLevel(logging.INFO)

    job_context = nv.context()
    inputs = job_context.get_inputs()
    outputs = job_context.get_outputs()
    params = job_context.get_parameters()

    lms_config.YAV_OAUTH_TOKEN = params.get("yav-oauth-token")
    lms_config.METADATA_CONN_ID = params.get("metadata-conn-id")
    yql_token = params.get("yql-token")

    metadata_file_path = inputs.get("metadata")
    with open(metadata_file_path, "r") as f:
        metadata = json.load(f)

    table_file_path = inputs.get("table")
    with open(table_file_path, "r") as f:
        mr_table = json.load(f)

    md = LMSDBLoadMetadataInc(
        object_id=metadata["object_id"],
        load_type=metadata["load_type"],
        increment_type=metadata["increment_type"],
        increment_column_name=metadata["increment_column_name"],
        step_back_value=metadata["step_back_value"]
    )

    max_value = get_max_value(
        cluster=mr_table["cluster"],
        yql_token=yql_token,
        column=md.increment_column_name,
        table=mr_table["table"]
    )

    if max_value:
        new_inc_value = max_value_to_string(max_value, md)
        with open(outputs.get('increment_value'), 'w') as f:
            f.write(new_inc_value)
    else:
        with open(outputs.get('increment_value'), 'w') as f:
            f.write("")


if __name__ == "__main__":
    main()
