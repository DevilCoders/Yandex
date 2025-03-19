from cloud.ai.lib.python.datasource.yt.ops import TableMeta
from .common import get_tables_dir

table_records_tags_meta = TableMeta(
    dir_path=get_tables_dir('records_tags'),
    attrs={
        'schema': {
            '$value': [
                {'name': 'record_id', 'type': 'string', 'sort_order': 'ascending', 'required': True},
                {'name': 'id', 'type': 'string', 'sort_order': 'ascending', 'required': True},
                {'name': 'action', 'type': 'string', 'sort_order': 'ascending', 'required': True},
                {
                    'name': 'data',
                    'type': 'any',
                },
                {
                    'name': 'received_at',
                    'type': 'string',
                    'required': True,
                },
            ],
            '$attributes': {'strict': True, 'unique_keys': True},
        }
    },
)
