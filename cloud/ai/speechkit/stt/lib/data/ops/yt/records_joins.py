from cloud.ai.lib.python.datasource.yt.ops import TableMeta
from .common import get_tables_dir

table_records_joins_meta = TableMeta(
    dir_path=get_tables_dir('records_joins'),
    attrs={
        'schema': {
            '$value': [
                {
                    'name': 'record_id',
                    'type': 'string',
                    'sort_order': 'ascending',
                    'required': True,
                },
                {'name': 'id', 'type': 'string', 'sort_order': 'ascending', 'required': True},
                {
                    'name': 'recognition',
                    'type': 'any',
                },
                {
                    'name': 'join_data',
                    'type': 'any',
                },
                {
                    'name': 'received_at',
                    'type': 'string',
                    'required': True,
                },
                {
                    'name': 'other',
                    'type': 'any',
                },
            ],
            '$attributes': {'strict': True, 'unique_keys': True},
        }
    },
)
