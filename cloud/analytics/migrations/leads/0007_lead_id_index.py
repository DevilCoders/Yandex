import yt.wrapper as yt
import yt.yson as yson
from cloud.analytics.scripts.yt_migrate import Table

TABLE_NAME = 'dyn_table_mkto_lead_id_dwh_id'


def upgrade():
    table = Table(name=TABLE_NAME)
    schema = yson.YsonList([
        {'name': 'mkto_lead_id', 'required': True, 'type': 'uint64', 'sort_order': 'ascending'},
        {'name': 'dwh_id', 'required': True, 'type': 'string'},
    ])
    schema.attributes['unique_keys'] = True
    schema.attributes['strict'] = True
    yt.create('table', table.yt_table_path, attributes={"schema": schema, "dynamic": True})
    yt.mount_table(table.yt_table_path, sync=True)
