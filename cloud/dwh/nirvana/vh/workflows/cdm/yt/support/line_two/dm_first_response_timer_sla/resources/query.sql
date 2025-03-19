$issues_table = {{ param["issues_ods_table"] -> quote() }};
$issue_slas_table = {{ param["issue_slas_ods_table"] -> quote() }};
$dst_table = {{ input1 -> table_quote() }};

$timer_response_to_user = 3656;
$timer_stopped = 'STOPPED';

$slas = (
    SELECT
        issue_slas.issue_id                         AS issue_id,
        issue_slas.timer_id                         AS timer_id,
        issue_slas.fail_at                          AS fail_at,
        issue_slas.fail_threshold_ms                AS fail_threshold_ms,
        issue_slas.paused_duration_ms               AS paused_duration_ms,
        issue_slas.spent_ms                         AS spent_ms,
        issue_slas.spent_hours                      AS spent_hours,
        issue_slas.started_at                       AS started_at,
        issue_slas.stopped_at                       AS stopped_at,
        issue_slas.violation_status                 AS violation_status,
        issue_slas.warn_at                          AS warn_at,
        issue_slas.warn_threshold_ms                AS warn_threshold_ms,
        issue_slas.clock_status                     AS clock_status,
        issues.payment_tariff                       AS payment_tariff
    FROM $issue_slas_table AS issue_slas
        LEFT JOIN $issues_table AS issues
            ON issue_slas.issue_id = issues.issue_id
    WHERE
        timer_id = $timer_response_to_user
        AND clock_status = $timer_stopped
);

INSERT INTO $dst_table WITH TRUNCATE
SELECT
    slas.payment_tariff     AS payment_tariff,
    slas.issue_id           AS issue_id,
    slas.timer_id           AS timer_id,
    slas.fail_at            AS fail_at,
    slas.fail_threshold_ms  AS fail_threshold_ms,
    slas.paused_duration_ms AS paused_duration_ms,
    slas.spent_ms           AS spent_ms,
    slas.spent_hours        AS spent_hours,
    slas.started_at         AS started_at,
    slas.stopped_at         AS stopped_at,
    slas.violation_status   AS violation_status,
    slas.warn_at            AS warn_at,
    slas.warn_threshold_ms  AS warn_threshold_ms,
    slas.clock_status       AS clock_status
FROM $slas AS slas
ORDER BY issue_id, timer_id
;
