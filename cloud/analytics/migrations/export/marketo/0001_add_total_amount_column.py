import yt.wrapper as yt
from cloud.analytics.scripts.yt_migrate import Table

TABLE_NAME = 'ya_consumption_daily'


def upgrade():
    table = Table(name=TABLE_NAME)
    with yt.Transaction():
        current_schema = yt.get('{}/@schema'.format(table.table_path))
        current_schema.append(
            {'name': 'total_amount', 'type': 'double'}
        )
        yt.alter_table(
            table.yt_table_path,
            schema=current_schema,
        )
        # If schema changed outside of YQL we should remove this attribute
        # https://wiki.yandex-team.ru/market/projects/hdp2yt/faq/ #11
        yql_row_spec = '{}/@_yql_row_spec'.format(table.table_path)
        if yt.exists(yql_row_spec):
            yt.remove(yql_row_spec)
