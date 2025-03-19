import yt.wrapper as yt
from cloud.analytics.scripts.yt_migrate import Table

TABLE_NAME = 'dyn_table'


def upgrade():
    table = Table(name=TABLE_NAME)
    with yt.Transaction():
        current_schema = yt.get('{}/@schema'.format(table.table_path))
        current_schema += [
            {'name': 'isv_status', 'type': 'string'},
            {'name': 'var_status', 'type': 'string'},
            {'name': 'isv_start_date', 'type': 'uint64'},
            {'name': 'isv_end_date', 'type': 'uint64'},
            {'name': 'var_start_date', 'type': 'uint64'},
            {'name': 'var_end_date', 'type': 'uint64'},
        ]
        yt.unmount_table(table.yt_table_path, sync=True)
        yt.alter_table(
            table.yt_table_path,
            schema=current_schema,
        )
    yt.mount_table(table.yt_table_path, sync=True)

