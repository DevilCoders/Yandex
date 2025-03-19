from cloud.ai.lib.python.datasource.yt.ops import TableMeta
from .common import get_tables_dir

table_markups_assignments_meta = TableMeta(
    dir_path=get_tables_dir('markups_assignments'),
    attrs={
        'schema': {
            '$value': [
                {
                    'name': 'id',
                    'type': 'string',
                    'sort_order': 'ascending',
                    'required': True,
                },
                {
                    'name': 'markup_id',
                    'type': 'string',
                },
                {
                    'name': 'markup_step',
                    'type': 'string',
                },
                {
                    'name': 'pool_id',
                    'type': 'string',
                },
                {
                    'name': 'data',
                    'type': 'any',
                },
                {
                    'name': 'tasks',
                    'type': 'any',
                },
                {
                    'name': 'validation_data',
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
