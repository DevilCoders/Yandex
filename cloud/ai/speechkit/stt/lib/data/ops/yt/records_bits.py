from cloud.ai.lib.python.datasource.yt.ops import TableMeta
from .common import get_tables_dir

table_records_bits_meta = TableMeta(
    dir_path=get_tables_dir('records_bits'),
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
                    'name': 'split_id',
                    'type': 'string',
                    'sort_order': 'ascending',
                    'required': True,
                },
                {
                    'name': 'id',
                    'type': 'string',
                    'sort_order': 'ascending',
                    'required': True,
                },
                {
                    'name': 'split_data',
                    'type': 'any',
                },
                {
                    'name': 'bit_data',
                    'type': 'any',
                },
                {
                    'name': 'convert_data',
                    'type': 'any',
                },
                {
                    'name': 'mark',
                    'type': 'string',
                },
                {
                    'name': 's3_obj',
                    'type': 'any',
                },
                {'name': 'audio_params', 'type': 'any'},
                {
                    'name': 'received_at',
                    'type': 'string',
                    'required': True,
                },
                {'name': 'other', 'type': 'any'},
            ],
            '$attributes': {'strict': True, 'unique_keys': True},
        }
    },
)

table_records_bits_audio_meta = TableMeta(
    dir_path=get_tables_dir('records_bits_audio'),
    attrs={
        'schema': {
            '$value': [
                {
                    'name': 'bit_id',
                    'type': 'string',
                    'sort_order': 'ascending',
                    'required': True,
                },
                {
                    'name': 'audio',
                    'type': 'string',
                    'required': True,
                },
                {
                    'name': 'hash',
                    'type': 'string',
                    'required': True,
                },
                {
                    'name': 'hash_version',
                    'type': 'string',
                    'required': True,
                },
            ],
            '$attributes': {'strict': True, 'unique_keys': True},
        }
    },
)
