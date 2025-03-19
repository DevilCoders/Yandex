from cloud.ai.lib.python.datasource.yt.ops import TableMeta
from .common import get_tables_dir

table_recognitions_meta = TableMeta(
    dir_path=get_tables_dir('recognitions'),
    attrs={
        'schema': {
            '$value': [
                {
                    'name': 'record_id',
                    'type': 'string',
                    'sort_order': 'ascending',
                    'required': True,
                },
                {
                    'name': 'recognition',
                    'type': 'any',
                },
                {
                    'name': 'response_chunks',
                    'type': 'any',
                },
                {
                    'name': 'source',
                    'type': 'any',
                },
                {
                    'name': 'endpoint',
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
            '$attributes': {'strict': True},
        }
    },
)
