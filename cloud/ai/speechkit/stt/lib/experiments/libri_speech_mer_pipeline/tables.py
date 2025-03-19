from cloud.ai.lib.python.datasource.yt.ops import TableMeta


def get_tables_dir(dir: str) -> str:
    return f'//home/mlcloud/speechkit/libri_speech_mer/{dir}'


table_recognitions_meta = TableMeta(
    dir_path=get_tables_dir('recognitions'),
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
                    'name': 'dataset',
                    'type': 'string',
                },
                {
                    'name': 'model',
                    'type': 'string',
                },
                {
                    'name': 'record',
                    'type': 'string',
                },
                {
                    'name': 'noise',
                    'type': 'any',
                },
                {
                    'name': 'reference',
                    'type': 'string',
                },
                {
                    'name': 'hypothesis',
                    'type': 'string',
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

table_markups_sbs_meta = TableMeta(
    dir_path=get_tables_dir('markups_sbs'),
    attrs={
        'schema': {
            '$value': [
                {
                    'name': 'recognition_id_left',
                    'type': 'string',
                    'sort_order': 'ascending',
                    'required': True,
                },
                {
                    'name': 'recognition_id_right',
                    'type': 'string',
                    'sort_order': 'ascending',
                    'required': True,
                },
                {
                    'name': 'dataset',
                    'type': 'string',
                },
                {
                    'name': 'record',
                    'type': 'string',
                },
                {
                    'name': 'reference',
                    'type': 'string',
                },
                {
                    'name': 'hypothesis_left',
                    'type': 'string',
                },
                {
                    'name': 'hypothesis_right',
                    'type': 'string',
                },
                {
                    'name': 'prior',
                    'type': 'string',
                },
                {
                    'name': 'choice',
                    'type': 'string',
                },
                {
                    'name': 'model_left',
                    'type': 'string'
                },
                {
                    'name': 'model_right',
                    'type': 'string'
                },
                {
                    'name': 'noise_left',
                    'type': 'any',
                },
                {
                    'name': 'noise_right',
                    'type': 'any',
                },
                {
                    'name': 'pool_id',
                    'type': 'string',
                },
                {
                    'name': 'assignment_id',
                    'type': 'string',
                },
                {
                    'name': 'task',
                    'type': 'any',
                },
                {
                    'name': 'markup_metrics',
                    'type': 'any',
                },
                {
                    'name': 'received_at',
                    'type': 'string',
                    'required': True,
                },
            ],
            '$attributes': {'strict': True},
        }
    },
)

table_markups_check_meta = TableMeta(
    dir_path=get_tables_dir('markups_check'),
    attrs={
        'schema': {
            '$value': [
                {
                    'name': 'recognition_id',
                    'type': 'string',
                    'sort_order': 'ascending',
                    'required': True,
                },
                {
                    'name': 'dataset',
                    'type': 'string',
                },
                {
                    'name': 'record',
                    'type': 'string',
                },
                {
                    'name': 'reference',
                    'type': 'string',
                },
                {
                    'name': 'hypothesis',
                    'type': 'string',
                },
                {
                    'name': 'ok',
                    'type': 'boolean',
                },
                {
                    'name': 'model',
                    'type': 'string'
                },
                {
                    'name': 'noise',
                    'type': 'any',
                },
                {
                    'name': 'pool_id',
                    'type': 'string',
                },
                {
                    'name': 'assignment_id',
                    'type': 'string',
                },
                {
                    'name': 'task',
                    'type': 'any',
                },
                {
                    'name': 'received_at',
                    'type': 'string',
                    'required': True,
                },
            ],
            '$attributes': {'strict': True},
        }
    },
)

table_name = 'table'
