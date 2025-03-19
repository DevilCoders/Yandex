from cloud.ai.lib.python.datasource.yt.ops import TableMeta


def get_tables_dir(dir: str) -> str:
    return f'//home/mlcloud/speechkit/ASREXP-778/{dir}'


table_source_meta = TableMeta(
    dir_path=get_tables_dir('source/v2'),
    attrs={
        'schema': {
            '$value': [
                {
                    'name': 'index',
                    'type': 'int64',
                    'sort_order': 'ascending',
                    'required': True,
                },
                {
                    'name': 'url',
                    'type': 'string',
                },
                {
                    'name': 'honeypots_urls',
                    'type': 'any',
                },
                {
                    'name': 'honeypots_active',
                    'type': 'any',
                }
            ],
            '$attributes': {'strict': True, 'unique_keys': True},
        }
    },
)

table_submissions_meta = TableMeta(
    dir_path=get_tables_dir('submissions'),
    attrs={
        'schema': {
            '$value': [
                {
                    'name': 'login',
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
                    'name': 'records_s3_bucket',
                    'type': 'string',
                },
                {
                    'name': 'records_s3_dir_key',
                    'type': 'string',
                },
                {
                    'name': 'received_at',
                    'type': 'string',
                },
            ],
            '$attributes': {'strict': True, 'unique_keys': True},
        }
    },
)

table_evaluation_entries_meta = TableMeta(
    dir_path=get_tables_dir('evaluation_entries'),
    attrs={
        'schema': {
            '$value': [
                {
                    'name': 'login',
                    'type': 'string',
                    'sort_order': 'ascending',
                    'required': True,
                },
                {
                    'name': 'submission_id',
                    'type': 'string',
                    'sort_order': 'ascending',
                    'required': True,
                },
                {
                    'name': 'evaluation_id',
                    'type': 'string',
                    'sort_order': 'ascending',
                    'required': True,
                },
                {
                    'name': 'audio_index',
                    'type': 'int64',
                    'sort_order': 'ascending',
                    'required': True,
                },
                {
                    'name': 'left_audio_url',
                    'type': 'string',
                },
                {
                    'name': 'right_audio_url',
                    'type': 'string',
                },
                {
                    'name': 'reference_audio_url',
                    'type': 'string',
                },
                {
                    'name': 'chosen_audio_url',
                    'type': 'string',
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
                    'name': 'received_at',
                    'type': 'string',
                }
            ],
            '$attributes': {'strict': True, 'unique_keys': False},
        }
    },
)

table_evaluations_meta = TableMeta(
    dir_path=get_tables_dir('evaluations'),
    attrs={
        'schema': {
            '$value': [
                {
                    'name': 'login',
                    'type': 'string',
                    'sort_order': 'ascending',
                    'required': True,
                },
                {
                    'name': 'submission_id',
                    'type': 'string',
                    'sort_order': 'ascending',
                    'required': True,
                },
                {
                    'name': 'evaluation_id',
                    'type': 'string',
                    'sort_order': 'ascending',
                    'required': True,
                },
                {
                    'name': 'submitted_at',
                    'type': 'string',
                },
                {
                    'name': 'evaluated_at',
                    'type': 'string',
                },
                {
                    'name': 'score',
                    'type': 'double',
                },
                {
                    'name': 'wins',
                    'type': 'int64',
                },
                {
                    'name': 'total',
                    'type': 'int64',
                },
            ],
            '$attributes': {'strict': True, 'unique_keys': True},
        }
    },
)
