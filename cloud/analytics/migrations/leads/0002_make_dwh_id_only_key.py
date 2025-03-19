from cloud.analytics.scripts.yt_migrate import Table
import yt.wrapper as yt

from yql.api.v1.client import YqlClient
from yql.client.parameter_value_builder import YqlParameterValueBuilder as ValueBuilder

TABLE_NAME = 'dyn_table'


def upgrade():
    table = Table(name=TABLE_NAME)
    schema = yt.get(f'{table.table_path}/@schema')
    for field in schema:
        if field['name'] in ('email', 'crm_id', 'mkto_id'):
            del field['sort_order']

    query = """
DECLARE $table_path AS String;

SELECT dwh_id FROM (
SELECT
    COUNT(*) AS count,
    `dwh_id`
FROM $table_path
GROUP BY `dwh_id`
) WHERE count > 1;
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
    first_result = list(results)[0].rows

    yt.config['backend'] = 'rpc'
    with yt.Transaction(type='tablet'):
        for row in first_result:
            dwh_id = row[0]
            duplicates = list(
                yt.select_rows(f'dwh_id, email, crm_id, mkto_id FROM [{table.table_path}] WHERE dwh_id = "{dwh_id}" ORDER BY dwh_updated_at LIMIT 2')
            )
            print('Removing row {}'.format(duplicates[0]))
            print(table.table_path)
            yt.delete_rows(table.table_path, (duplicates[0],))

    tmp_table_name = f'{table.table_path}_0002'
    with yt.Transaction():
        yt.create('table', tmp_table_name, attributes={'schema': schema})
        yt.run_sort(table.table_path, tmp_table_name,
                    sort_by=['dwh_id'],
                    spec={
                        "partition_job_io": {
                            "table_writer": {
                                "block_size": 256 * 2 ** 10,
                                "desired_chunk_size": 100 * 2 ** 20
                            }
                        },
                        "merge_job_io": {
                            "table_writer": {
                                "block_size": 256 * 2 ** 10,
                                "desired_chunk_size": 100 * 2 ** 20
                            }
                        },
                        "sort_job_io": {
                            "table_writer": {
                                "block_size": 256 * 2 ** 10,
                                "desired_chunk_size": 100 * 2 ** 20
                            }
                        },
                    })

    yt.alter_table(tmp_table_name, dynamic=True)

    with yt.Transaction():
        yt.remove(table.table_path)
        yt.move(tmp_table_name, table.table_path)

    yt.mount_table(table.table_path, sync=True)
