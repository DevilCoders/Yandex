import yt.wrapper as yt
from cloud.analytics.scripts.yt_migrate import Table

TABLE_NAME = 'dyn_table'


def upgrade():
    table = Table(name=TABLE_NAME)
    with yt.Transaction():
        current_schema = yt.get('{}/@schema'.format(table.table_path))
        current_schema += [
            {'name': 'mkto_ba_created_at', 'type': 'uint64'},
            {'name': 'mkto_ba_paid_at', 'type': 'uint64'},
            {'name': 'mkto_ba_paid_real_consumption_prob', 'type': 'double'},
            {'name': 'mkto_client_name', 'type': 'string'},
            {'name': 'mkto_first_consumption_date', 'type': 'uint32'},
            {'name': 'mkto_first_paid_consumption_date', 'type': 'uint32'},
            {'name': 'mkto_grant1_amount', 'type': 'double'},
            {'name': 'mkto_grant1_end_date', 'type': 'uint64'},
            {'name': 'mkto_grant1_id', 'type': 'string'},
            {'name': 'mkto_grant1_remaining', 'type': 'uint64'},
            {'name': 'mkto_grant1_start_date', 'type': 'uint64'},
            {'name': 'mkto_sales_person', 'type': 'string'},
            {'name': 'mkto_segment', 'type': 'string'},
            {'name': 'mkto_test_group_index', 'type': 'double'},
        ]
        yt.unmount_table(table.yt_table_path, sync=True)
        yt.alter_table(
            table.yt_table_path,
            schema=current_schema,
        )
    yt.mount_table(table.yt_table_path, sync=True)
