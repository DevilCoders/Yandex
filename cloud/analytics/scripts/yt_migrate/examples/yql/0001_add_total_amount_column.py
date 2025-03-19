from cloud.analytics.scripts.yt_migrate import Table

from yql.api.v1.client import YqlClient
from yql.client.parameter_value_builder import YqlParameterValueBuilder as ValueBuilder

TABLE_NAME = 'ya_consumption_daily'


def upgrade():
    table = Table(name=TABLE_NAME)
    query = """
DECLARE $table_path AS String;

INSERT INTO $table_path WITH TRUNCATE SELECT
    dwh_created_at,
    ya_activity_time,
    ya_activity_id,
    email,
    avg_daily_consumption_in_last_period,
    compared_to_prev_period_percentage,
    amount_consumed,
    type_name,
    product,
    subproduct,
    compared_to_prev_period,
    is_grant,
    CAST(null AS Double) AS total_amount
FROM $table_path
ORDER BY dwh_created_at, email
"""
    parameters = {
        '$table_path': ValueBuilder.make_utf8(table.table_path),
    }
    client = YqlClient(db=table.yt_cluster)
    request = client.query(query=query)
    request.run(parameters=ValueBuilder.build_json_map(parameters))
    results = request.get_results()
    if not results.is_success:
        last_error = ''
        if results.errors:
            last_error = results.errors[-1]
            for error in results.errors:
                print(error)
        raise RuntimeError('YQL query failed: {}'.format(last_error))
