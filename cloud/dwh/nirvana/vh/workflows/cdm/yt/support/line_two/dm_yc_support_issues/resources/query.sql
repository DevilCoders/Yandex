PRAGMA Library('datetime.sql');
PRAGMA Library("helpers.sql");

IMPORT `datetime` SYMBOLS $format_msk_date_by_timestamp;
IMPORT `helpers` SYMBOLS $lookup_string;

$issues_table = {{ param["issues_ods_table"] -> quote() }};
$issue_types_table = {{ param["issue_types_ods_table"] -> quote() }};
$issue_events_table = {{ param["issue_events_ods_table"] -> quote() }};
$issue_slas_table = {{ param["comment_slas_cdm_table"] -> quote() }};
$st_users_table = {{ param["st_users_ods_table"] -> quote() }};
$crm_tags_table = {{ param["ba_crm_tags_stg_table"] -> quote() }};
$staff_department_employees_table = {{ param["staff_department_employees_cdm_table"] -> quote() }};
$dst_table = {{ input1 -> table_quote() }};

$COMPONENT_QUOTA_ID = '5dd52152a2b79e001d6d4ea8';
$TRACKER_QUEUE_CLOUDLINETWO = '615c16e902cdad686c7a362b';
$SUPPORT_DEPARTMENT_ID = 195856; --Customer experience service Yandex.Cloud
$SUPPORT_SECOND_LINE_TEAM_ID = 10700; --Leading support engineers group Yandex.Cloud

$cast_to_ms = ($field) -> (DateTime::ToMilliseconds($field));
$get_string = ($field, $lookup_key) -> ($lookup_string($field, $lookup_key, NULL));


$support_staff_employees = (
    SELECT
        employees.staff_user_id                                 AS staff_user_id,
        IF(employees.is_support_summon = 0, True, False)        AS is_external_summon,
        IF(employees.is_second_line_summon > 0, True, False)    AS is_second_line_summon,
        IF(employees.is_support_summon > 0, True, False)        AS is_support_summon
    FROM (
        SELECT
            employees.staff_user_id                                                 AS staff_user_id,
            SUM(IF(employees.department_id = $SUPPORT_SECOND_LINE_TEAM_ID, 1, 0))   AS is_second_line_summon,
            SUM(IF(employees.department_id = $SUPPORT_DEPARTMENT_ID, 1, 0))         AS is_support_summon
        FROM 
            $staff_department_employees_table AS employees
        GROUP BY employees.staff_user_id
    ) AS employees  
);

$comment_slas_stats = (
    SELECT
        slas.issue_id                               AS issue_id,
        SUM(slas.spent_ms)                          AS spent_total_ms,
        MIN(slas.stopped_at)                        AS first_timer_stopped_at,
        SUM(IF(slas.direction = 'outgoing', 1, 0))  AS outgoing_message_count,
        SUM(IF(slas.direction = 'incoming', 1, 0))  AS incoming_message_count,
        SUM(IF(employees.is_support_summon, 1, 0))  AS support_message_count
    FROM $issue_slas_table AS slas
        LEFT JOIN $support_staff_employees AS employees
            ON slas.staff_user_id = employees.staff_user_id
    GROUP BY slas.issue_id
);

$ba_crm_tags = (
    SELECT
        issues.issue_id             AS issue_id,
        crm_tags.segment            AS segment, 
        crm_tags.segment_current    AS segment_current
    FROM $issues_table AS issues
        INNER JOIN $crm_tags_table AS crm_tags
            ON issues.billing_account_id = crm_tags.billing_account_id 
    WHERE $format_msk_date_by_timestamp(issues.created_at) = crm_tags.`date`
);

$issue_status_first_changes = (
    SELECT
        issue_id    AS issue_id,
        MIN(dttm)   AS dttm
    FROM $issue_events_table
    WHERE field = 'status' AND old_value IS NOT NULL
    GROUP BY issue_id
);

$issue_transitions = (
    SELECT
        issue_id    AS issue_id,
        MAX(dttm)   AS dttm
    FROM $issue_events_table
    WHERE 
        field = 'queue' 
        AND new_value IS NOT NULL
        AND $get_string(`new_value`, 'id') = $TRACKER_QUEUE_CLOUDLINETWO
    GROUP BY issue_id
);

INSERT INTO $dst_table WITH TRUNCATE
    SELECT
        issues.created_at                                                           AS created_at,
        issues.resolved_at                                                          AS resolved_at,
        issues.key                                                                  AS issue_key,
        issues.issue_id                                                             AS issue_id,
        issue_types.key                                                             AS issue_type,
        issues.payment_tariff                                                       AS payment_tariff,
        issues.billing_account_id                                                   AS billing_account_id,
        issues.feedback_reaction_speed                                              AS feedback_reaction_speed,
        issues.feedback_response_completeness                                       AS feedback_response_completeness,
        issues.feedback_general                                                     AS feedback_general,
        issues.author_id                                                            AS author_id,
        usr.st_user_login_hash                                                      AS author_login_hash,
        ListHas(Yson::ConvertToStringList(components), $COMPONENT_QUOTA_ID)         AS components_quotas,
        
        slas.spent_total_ms                                                         AS comment_sla_spent_total_ms,
        slas.first_timer_stopped_at                                                 AS comment_sla_first_timer_stopped_at,
        $cast_to_ms(slas.first_timer_stopped_at - issues.created_at)                AS comment_sla_first_timer_stop_lag_ms,
        NVL(slas.outgoing_message_count, 0)                                         AS comments_outgoing_cnt,
        NVL(slas.incoming_message_count, 0)                                         AS comments_incoming_cnt,
        NVL(slas.support_message_count, 0)                                          AS comments_from_support_cnt,

        crm_tags.segment                                                            AS crm_segment,
        crm_tags.segment_current                                                    AS segment_current,
        
        issue_status_first_changes.dttm                                                                                      AS first_status_change_at,
        IF(issue_status_first_changes.dttm IS NOT NULL, $cast_to_ms(issue_status_first_changes.dttm - issues.created_at), 0) AS status_first_change_time_ms,

        issue_transitions.dttm                                                                                                                  AS moved_to_linetwo_queue_at,
        IF(issue_transitions.dttm IS NOT NULL AND issues.resolved_at IS NOT NULL, $cast_to_ms(issues.resolved_at - issue_transitions.dttm), 0)  AS linetwo_spent_ms,

    FROM $issues_table AS issues
        INNER JOIN $issue_types_table AS issue_types
            ON issues.type_id = issue_types.id
        LEFT JOIN $st_users_table AS usr
            ON issues.author_id = usr.st_user_id
        LEFT JOIN $comment_slas_stats AS slas
            ON slas.issue_id = issues.issue_id
        LEFT JOIN $ba_crm_tags AS crm_tags
            ON issues.issue_id = crm_tags.issue_id
        LEFT JOIN $issue_status_first_changes AS issue_status_first_changes
            ON issues.issue_id = issue_status_first_changes.issue_id
        LEFT JOIN $issue_transitions AS issue_transitions
            ON issues.issue_id = issue_transitions.issue_id
;
