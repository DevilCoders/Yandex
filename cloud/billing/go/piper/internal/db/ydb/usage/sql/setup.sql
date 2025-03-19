CREATE TABLE `test/usage/realtime/invalid_metrics`(
    source_name Utf8,
    uploaded_at Uint64,
    reason Utf8,
    metric_id Utf8,
    source_id Utf8,
    reason_comment Utf8,
    hostname Utf8,
    raw_metric Utf8,
    metric Json,
    metric_schema Utf8,
    metric_source_id Utf8,
    metric_resource_id Utf8,
    sequence_id Uint64,

    PRIMARY KEY (source_name, uploaded_at, reason, metric_id)
);

CREATE TABLE `test/usage/realtime/oversized_messages` (
    source_id Utf8,
    seq_no Uint64,
    compressed_message String,
    created_at Datetime,
    PRIMARY KEY (source_id, seq_no)
);
