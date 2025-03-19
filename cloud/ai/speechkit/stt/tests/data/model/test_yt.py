import unittest

from cloud.ai.speechkit.stt.lib.data.ops.yt import (
    table_records_meta,
    table_records_audio_meta,
    table_markups_pools_meta,
    table_metrics_markup_meta,
)


class TestYT(unittest.TestCase):
    def test_schemas(self):
        self.assertEqual({
            'schema': {
                '$value': [
                    {
                        'name': 'id',
                        'type': 'string',
                        'sort_order': 'ascending',
                        'required': True,
                    },
                    {
                        'name': 's3_obj',
                        'type': 'any',
                    },
                    {
                        'name': 'mark',
                        'type': 'string',
                    },
                    {
                        'name': 'source',
                        'type': 'any',
                    },
                    {
                        'name': 'req_params',
                        'type': 'any',
                    },
                    {
                        'name': 'audio_params',
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
        }, table_records_meta.attrs)

        self.assertEqual({
            'schema': {
                '$value': [
                    {
                        'name': 'record_id',
                        'type': 'string',
                        'sort_order': 'ascending',
                        'required': True,
                    },
                    {
                        'name': 'audio',
                        'type': 'string',
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
        }, table_records_audio_meta.attrs)

        self.assertEqual({
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
                        'name': 'params',
                        'type': 'any',
                    },
                    {
                        'name': 'toloka_environment',
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
        }, table_markups_pools_meta.attrs)

        self.assertEqual({
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
                        {
                            'name': 'bit_duration_seconds_p10',
                            'type': 'double',
                        },
                        {
                            'name': 'bit_duration_seconds_p25',
                            'type': 'double',
                        },
                        {
                            'name': 'bit_duration_seconds_p50',
                            'type': 'double',
                        },
                        {
                            'name': 'bit_duration_seconds_p75',
                            'type': 'double',
                        },
                        {
                            'name': 'bit_duration_seconds_p90',
                            'type': 'double',
                        },
                        {
                            'name': 'bit_duration_seconds_p95',
                            'type': 'double',
                        },
                        {
                            'name': 'bit_duration_seconds_p99',
                            'type': 'double',
                        },
                        {
                            'name': 'bit_overlap_p10',
                            'type': 'double',
                        },
                        {
                            'name': 'bit_overlap_p25',
                            'type': 'double',
                        },
                        {
                            'name': 'bit_overlap_p50',
                            'type': 'double',
                        },
                        {
                            'name': 'bit_overlap_p75',
                            'type': 'double',
                        },
                        {
                            'name': 'bit_overlap_p90',
                            'type': 'double',
                        },
                        {
                            'name': 'bit_overlap_p95',
                            'type': 'double',
                        },
                        {
                            'name': 'bit_overlap_p99',
                            'type': 'double',
                        },
                        {
                            'name': 'received_at',
                            'type': 'string',
                            'required': True,
                        },
                        {
                            'name': 'received_at_ts',
                            'type': 'int64',
                            'required': True,
                        },
                    ],
                '$attributes': {'strict': True, 'unique_keys': True},
            },
            'optimize_for': 'scan',
        }, table_metrics_markup_meta.attrs)
