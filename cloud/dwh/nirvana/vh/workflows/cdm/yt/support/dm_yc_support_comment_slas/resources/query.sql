$issues_table = {{ param["issues_ods_table"] -> quote() }};
$issue_slas_table = {{ param["issue_slas_ods_table"] -> quote() }};
$issue_components_table = {{ param["issue_components_ods_table"] -> quote() }};
$support_issues_table = {{ param["support_issues_ods_table"] -> quote() }};
$support_comments_table = {{ param["support_comments_ods_table"] -> quote() }};
$startrek_comments_table = {{ param["startrek_comments_ods_table"] -> quote() }};
$startrek_users_pii_table = {{ param["startrek_users_pii_ods_table"] -> quote() }};
$staff_users_table = {{ param["staff_users_ods_table"] -> quote() }};
$staff_users_pii_table = {{ param["staff_users_pii_ods_table"] -> quote() }};
$dst_table = {{ input1 -> table_quote() }};

$to_seconds = ($field) -> (DateTime::ToSeconds($field));
$from_seconds = ($field) -> (DateTime::FromSeconds($field));

$timer_response_to_user = 3052;
$timer_stopped = 'STOPPED';

$tracker_component_id = '5eec84b1aaf5824bfbb617bc';

$response_interval = ($time_hours) -> (
    CASE
        WHEN $time_hours < 1 THEN 'less then 1 hour'
        WHEN $time_hours >= 1 AND $time_hours < 3 THEN '1-2 hours'
        WHEN $time_hours >= 3 AND $time_hours < 5 THEN '3-4 hours'
        WHEN $time_hours >= 5 AND $time_hours < 9 THEN '5-8 hours'
        WHEN $time_hours >= 9 AND $time_hours < 25 THEN '9-24 hours'
        ELSE '25+ hours'
    END
);

$tracker_labeled_issues = (
    SELECT issue_id
    FROM $issue_components_table
    WHERE component_id = $tracker_component_id
);

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
        $response_interval(issue_slas.spent_hours)  AS response_interval
    FROM $issue_slas_table AS issue_slas
        LEFT ONLY JOIN $tracker_labeled_issues AS tracker_issues
            ON issue_slas.issue_id = tracker_issues.issue_id
        LEFT JOIN $issues_table AS issues
            ON issue_slas.issue_id = issues.issue_id
    WHERE
        timer_id = $timer_response_to_user
        AND clock_status = $timer_stopped
        AND issue_slas.stopped_at <= issues.resolved_at
);


$comments = (
    SELECT
        si.startrek_issue_id    AS issue_id,
        sc.created_at           AS created_at,
        sc.direction            AS direction,
        sc.iam_user_id          AS iam_user_id,
        sc.id                   AS support_comment_id,

        $from_seconds($to_seconds(sc.created_at) + CAST(1 AS UInt32)) AS created_at_second_after,
        $from_seconds($to_seconds(sc.created_at) - CAST(1 AS UInt32)) AS created_at_second_before,
    FROM $support_comments_table AS sc
        INNER JOIN $support_issues_table AS si
            ON sc.ticket_id = si.id
    UNION ALL
    SELECT
        c.issue_id                  AS issue_id,
        c.created_at                AS created_at,
        CASE
            WHEN Yson::LookupString(c.emails_info,"createdBy",Yson::Options(false as Strict)) IS NOT NULL THEN 'incoming'
            WHEN Yson::YPath(c.email,"/info/from",Yson::Options(false as Strict)) IS NOT NULL THEN 'outgoing'
            ELSE NULL
        END                         AS direction,
        NULL                        AS iam_user_id,
        c.short_id                  AS support_comment_id
    FROM $startrek_comments_table   AS c
    JOIN $issues_table              AS i
        on c.issue_id=i.issue_id
    WHERE email_created_by = 'cloud@support.yandex.ru'
    AND c.issue_id NOT IN (
        SELECT DISTINCT
            si.startrek_issue_id
        FROM $support_comments_table AS sc
        INNER JOIN $support_issues_table AS si
            ON sc.ticket_id = si.id
    )
);

$comment_slas = (
    SELECT
        c.support_comment_id    AS support_comment_id,
        c.issue_id              AS issue_id,
        slas.timer_id           AS timer_id,
        slas.fail_at            AS fail_at,
        slas.fail_threshold_ms  AS fail_threshold_ms,
        slas.paused_duration_ms AS paused_duration_ms,
        slas.spent_ms           AS spent_ms,
        slas.started_at         AS started_at,
        slas.stopped_at         AS stopped_at,
        slas.violation_status   AS violation_status,
        slas.warn_at            AS warn_at,
        slas.warn_threshold_ms  AS warn_threshold_ms,
        slas.clock_status       AS clock_status,
        slas.response_interval  AS response_interval
    FROM $comments AS c
        INNER JOIN $slas AS slas
            ON c.issue_id = slas.issue_id AND c.created_at = slas.stopped_at

    UNION ALL

    SELECT
        c.support_comment_id    AS support_comment_id,
        c.issue_id              AS issue_id,
        slas.timer_id           AS timer_id,
        slas.fail_at            AS fail_at,
        slas.fail_threshold_ms  AS fail_threshold_ms,
        slas.paused_duration_ms AS paused_duration_ms,
        slas.spent_ms           AS spent_ms,
        slas.started_at         AS started_at,
        slas.stopped_at         AS stopped_at,
        slas.violation_status   AS violation_status,
        slas.warn_at            AS warn_at,
        slas.warn_threshold_ms  AS warn_threshold_ms,
        slas.clock_status       AS clock_status,
        slas.response_interval  AS response_interval
    FROM $comments AS c
        INNER JOIN $slas AS slas
            ON c.issue_id = slas.issue_id AND c.created_at_second_after = slas.stopped_at

    UNION ALL

    SELECT
        c.support_comment_id    AS support_comment_id,
        c.issue_id              AS issue_id,
        slas.timer_id           AS timer_id,
        slas.fail_at            AS fail_at,
        slas.fail_threshold_ms  AS fail_threshold_ms,
        slas.paused_duration_ms AS paused_duration_ms,
        slas.spent_ms           AS spent_ms,
        slas.started_at         AS started_at,
        slas.stopped_at         AS stopped_at,
        slas.violation_status   AS violation_status,
        slas.warn_at            AS warn_at,
        slas.warn_threshold_ms  AS warn_threshold_ms,
        slas.clock_status       AS clock_status,
        slas.response_interval  AS response_interval
    FROM $comments AS c
        INNER JOIN $slas AS slas
            ON c.issue_id = slas.issue_id AND c.created_at_second_before = slas.stopped_at
);

$all_comments = (
    SELECT
        c.support_comment_id        AS support_comment_id,
        c.created_at                AS created_at,
        c.direction                 AS direction,
        c.iam_user_id               AS iam_user_id,
        c.issue_id                  AS issue_id,
        slas.timer_id               AS timer_id,
        slas.fail_at                AS fail_at,
        slas.fail_threshold_ms      AS fail_threshold_ms,
        slas.paused_duration_ms     AS paused_duration_ms,
        slas.spent_ms               AS spent_ms,
        slas.started_at             AS started_at,
        slas.stopped_at             AS stopped_at,
        slas.violation_status       AS violation_status,
        slas.warn_at                AS warn_at,
        slas.warn_threshold_ms      AS warn_threshold_ms,
        slas.clock_status           AS clock_status,
        slas.response_interval      AS response_interval
    FROM $comments AS c
        LEFT JOIN $comment_slas AS slas
            ON c.support_comment_id = slas.support_comment_id
);

$staff_users = (
    SELECT
        u.st_user_id                        AS tracker_user_id,
        p.staff_user_id                     AS staff_user_id,
        p.department_name                   AS staff_department_name,
        GREATEST(pii_p.start_ts,p.start_ts) AS start_ts,
        LEAST(pii_p.end_ts,p.end_ts)        AS end_ts
    FROM $startrek_users_pii_table AS u
        INNER JOIN $staff_users_pii_table AS pii_p
            ON u.st_user_login = pii_p.staff_user_login
        INNER JOIN $staff_users_table AS p
            ON pii_p.staff_user_id = p.staff_user_id
    WHERE pii_p.start_ts <= p.end_ts and p.start_ts <= pii_p.end_ts
);

$comment_users = (
    SELECT
        st_comments.author                  AS author_id,
        st_comments.issue_id                AS issue_id,
        st_comments.created_at              AS created_at,
        issues.payment_tariff               AS payment_tariff,
        comments.support_comment_id         AS support_comment_id,
        comments.direction                  AS direction,
        comments.iam_user_id                AS iam_user_id,
        comments.timer_id                   AS timer_id,
        comments.fail_at                    AS fail_at,
        comments.fail_threshold_ms          AS fail_threshold_ms,
        comments.paused_duration_ms         AS paused_duration_ms,
        comments.spent_ms                   AS spent_ms,
        comments.started_at                 AS started_at,
        comments.stopped_at                 AS stopped_at,
        comments.violation_status           AS violation_status,
        comments.warn_at                    AS warn_at,
        comments.warn_threshold_ms          AS warn_threshold_ms,
        comments.clock_status               AS clock_status,
        comments.response_interval          AS response_interval,
        staff_users.staff_user_id           AS staff_user_id,
        staff_users.staff_department_name   AS staff_department_name
    FROM $startrek_comments_table AS st_comments
        LEFT JOIN $all_comments AS comments
            ON comments.issue_id = st_comments.issue_id
            AND comments.created_at = st_comments.created_at
        LEFT JOIN $issues_table AS issues
            ON comments.issue_id = issues.issue_id
        LEFT JOIN $staff_users AS staff_users
            ON st_comments.author = staff_users.tracker_user_id
        WHERE st_comments.created_at BETWEEN staff_users.start_ts AND staff_users.end_ts OR staff_users.tracker_user_id IS NULL
);



INSERT INTO $dst_table WITH TRUNCATE
    SELECT *
    FROM $comment_users
    ORDER BY issue_id, timer_id
;
