from cloud.ai.speechkit.stt.lib.data.model.dao import percentiles
from cloud.ai.lib.python.datasource.yt.ops import TableMeta
from .common import get_tables_dir

table_metrics_eval_records_meta = TableMeta(
    dir_path=get_tables_dir('metrics/eval/records'),
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
                    'name': 'channel',
                    'type': 'int64',
                    'sort_order': 'ascending',
                    'required': True,
                },
                {
                    'name': 'metric_name',
                    'type': 'string',
                    'sort_order': 'ascending',
                    'required': True,
                },
                {
                    'name': 'text_transformations',
                    'type': 'string',
                    'sort_order': 'ascending',
                    'required': True,
                },
                {
                    'name': 'api',
                    'type': 'string',
                    'sort_order': 'ascending',
                    'required': True,
                },
                {
                    'name': 'model',
                    'type': 'string',
                    'sort_order': 'ascending',
                    'required': True,
                },
                {
                    'name': 'method',
                    'type': 'string',
                    'sort_order': 'ascending',
                    'required': True,
                },
                {
                    'name': 'eval_id',
                    'type': 'string',
                    'sort_order': 'ascending',
                    'required': True,
                },
                {
                    'name': 'metric_value',
                    'type': 'double',
                    'required': True,
                },
                {
                    'name': 'metric_data',
                    'type': 'any',
                },
                {
                    'name': 'slices',
                    'type': 'any',
                },
                {
                    'name': 'calc_data',
                    'type': 'any',
                },
                {
                    'name': 'hypothesis',
                    'type': 'any',
                },
                {
                    'name': 'reference',
                    'type': 'any',
                },
                {
                    'name': 'recognition_endpoint',
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
            '$attributes': {
                'strict': True,
                'unique_keys': True,
            },
        },
    },
)

table_metrics_eval_tags_meta = TableMeta(
    dir_path=get_tables_dir('metrics/eval/tags'),
    attrs={
        'schema': {
            '$value': [
                {
                    'name': 'tag',
                    'type': 'string',
                    'sort_order': 'ascending',
                    'required': True,
                },
                {
                    'name': 'slice',
                    'type': 'string',
                    'sort_order': 'ascending',
                    'required': True,
                },
                {
                    'name': 'metric_name',
                    'type': 'string',
                    'sort_order': 'ascending',
                    'required': True,
                },
                {
                    'name': 'aggregation',
                    'type': 'string',
                    'sort_order': 'ascending',
                    'required': True,
                },
                {
                    'name': 'text_transformations',
                    'type': 'string',
                    'sort_order': 'ascending',
                    'required': True,
                },
                {
                    'name': 'api',
                    'type': 'string',
                    'sort_order': 'ascending',
                    'required': True,
                },
                {
                    'name': 'model',
                    'type': 'string',
                    'sort_order': 'ascending',
                    'required': True,
                },
                {
                    'name': 'method',
                    'type': 'string',
                    'sort_order': 'ascending',
                    'required': True,
                },
                {
                    'name': 'eval_id',
                    'type': 'string',
                    'sort_order': 'ascending',
                    'required': True,
                },
                {
                    'name': 'metric_value',
                    'type': 'double',
                    'required': True,
                },
                {
                    'name': 'calc_data',
                    'type': 'any',
                },
                {
                    'name': 'recognition_endpoint',
                    'type': 'any',
                },
                {
                    'name': 'received_at',
                    'type': 'string',
                    'required': True,
                },
                {
                    'name': 'received_at_ts',  # for Datalens
                    'type': 'int64',
                    'required': True,
                },
                {
                    'name': 'other',
                    'type': 'any',
                },
            ],
            '$attributes': {
                'strict': True,
                'unique_keys': True,
            },
        },
        'optimize_for': 'scan',  # for CHYT + Datalens
    },
)

table_metrics_markup_meta = TableMeta(
    dir_path=get_tables_dir('metrics/markup/v1'),
    attrs={
        'schema': {
            '$value':
                [
                    {
                        'name': 'markup_id',
                        'type': 'string',
                        'sort_order': 'ascending',
                        'required': True,
                    },
                    {
                        'name': 'markup_type',
                        'type': 'string',
                        'required': True,
                    },
                    {
                        'name': 'markup_metadata',
                        'type': 'any',
                    },
                    {
                        'name': 'language',
                        'type': 'string',
                        'required': True,
                    },
                    {
                        'name': 'records_durations_sum_minutes',
                        'type': 'double',
                    },
                    {
                        'name': 'markup_duration_minutes',
                        'type': 'double',
                    },
                    {
                        'name': 'filtered_transcript_bits_ratio',
                        'type': 'double',
                    },
                    {
                        'name': 'accuracy',
                        'type': 'double',
                    },
                    {
                        'name': 'not_evaluated_records_ratio',
                        'type': 'double',
                    },
                    {
                        'name': 'record_markup_speed_mean',
                        'type': 'double',
                    },
                    {
                        'name': 'step_to_metrics',
                        'type': 'any',
                    },
                    {
                        'name': 'transcript_cost',
                        'type': 'double',
                    },
                    {
                        'name': 'transcript_toloker_speed_mean',
                        'type': 'double',
                    },
                    {
                        'name': 'transcript_all_assignments_honeypots_quality_mean',
                        'type': 'double',
                    },
                    {
                        'name': 'transcript_accepted_assignments_honeypots_quality_mean',
                        'type': 'double',
                    },
                    {
                        'name': 'check_transcript_cost',
                        'type': 'double',
                    },
                    {
                        'name': 'check_transcript_toloker_speed_mean',
                        'type': 'double',
                    },
                    {
                        'name': 'check_transcript_all_assignments_honeypots_quality_mean',
                        'type': 'double',
                    },
                    {
                        'name': 'check_transcript_accepted_assignments_honeypots_quality_mean',
                        'type': 'double',
                    },
                    {
                        'name': 'check_asr_transcript_cost',
                        'type': 'double',
                    },
                    {
                        'name': 'check_asr_transcript_toloker_speed_mean',
                        'type': 'double',
                    },
                    {
                        'name': 'check_asr_transcript_all_assignments_honeypots_quality_mean',
                        'type': 'double',
                    },
                    {
                        'name': 'check_asr_transcript_accepted_assignments_honeypots_quality_mean',
                        'type': 'double',
                    },
                    {
                        'name': 'classification_cost',
                        'type': 'double',
                    },
                    {
                        'name': 'classification_toloker_speed_mean',
                        'type': 'double',
                    },
                    {
                        'name': 'classification_all_assignments_honeypots_quality_mean',
                        'type': 'double',
                    },
                    {
                        'name': 'classification_accepted_assignments_honeypots_quality_mean',
                        'type': 'double',
                    },
                    {
                        'name': 'quality_evaluation_cost',
                        'type': 'double',
                    },
                    {
                        'name': 'quality_evaluation_toloker_speed_mean',
                        'type': 'double',
                    },
                    {
                        'name': 'quality_evaluation_all_assignments_honeypots_quality_mean',
                        'type': 'double',
                    },
                    {
                        'name': 'quality_evaluation_accepted_assignments_honeypots_quality_mean',
                        'type': 'double',
                    },
                    {
                        'name': 'transcript_feedback_loop',
                        'type': 'any',
                    },
                    {
                        'name': 'transcript_feedback_loop_markup_attempts_eq_1',
                        'type': 'int64',
                    },
                    {
                        'name': 'transcript_feedback_loop_markup_attempts_eq_2',
                        'type': 'int64',
                    },
                    {
                        'name': 'transcript_feedback_loop_markup_attempts_eq_3',
                        'type': 'int64',
                    },
                    {
                        'name': 'transcript_feedback_loop_markup_attempts_gt_3',
                        'type': 'int64',
                    },
                    {
                        'name': 'transcript_feedback_loop_confidence_mean',
                        'type': 'double',
                    },
                    {
                        'name': 'transcript_feedback_loop_quality_unknown',
                        'type': 'int64',
                    },
                    {
                        'name': 'transcript_feedback_loop_quality_ok',
                        'type': 'int64',
                    },
                    {
                        'name': 'transcript_feedback_loop_quality_not_ok',
                        'type': 'int64',
                    },
                    {
                        'name': 'bits_stats',
                        'type': 'any',
                    },
                    {
                        'name': 'nirvana_workflow_url',
                        'type': 'string',
                    },
                ] +
                [
                    {
                        'name': f'bit_duration_seconds_p{p}',
                        'type': 'double',
                    } for p in percentiles
                ] +
                [
                    {
                        'name': f'bit_overlap_p{p}',
                        'type': 'double',
                    } for p in percentiles
                ] +
                [
                    {
                        'name': 'received_at',
                        'type': 'string',
                        'required': True,
                    },
                    {
                        'name': 'received_at_ts',  # for Datalens
                        'type': 'int64',
                        'required': True,
                    },
                ],
            '$attributes': {
                'strict': True,
                'unique_keys': True,
            },
        },
        'optimize_for': 'scan',  # for CHYT + Datalens
    },
)
