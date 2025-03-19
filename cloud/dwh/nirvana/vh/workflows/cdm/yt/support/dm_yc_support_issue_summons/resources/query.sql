PRAGMA Library("helpers.sql");

IMPORT `helpers` SYMBOLS $lookup_string;


$issue_events_table = {{ param["issue_events_ods_table"] -> quote() }};
$staff_pii_persons_table = {{ param["staff_pii_persons_ods_table"] -> quote() }};
$dst_table = {{ input1 -> table_quote() }};


$get_string = ($container, $field) -> ($lookup_string($container, $field, NULL));


$summonee_updates = (
    SELECT
        e.issue_id  AS issue_id,
        e.dttm      AS dttm,
        Yson::ConvertToList(`new_value`) AS value_list
    FROM $issue_events_table AS e
    WHERE field = 'pendingReplyFrom' 
);

$flatten_summonee_updates = (
    SELECT
        su.issue_id                         AS issue_id,
        su.dttm                             AS dttm,
        $get_string(su.value_list, 'id')    AS summonee_login
    FROM $summonee_updates AS su
    FLATTEN LIST BY value_list
);


$summonee_ids = (
    SELECT
        persons.staff_user_id   AS staff_user_id,
        su.issue_id             AS issue_id,
        su.dttm                 AS dttm
    FROM $flatten_summonee_updates AS su
        INNER JOIN $staff_pii_persons_table as persons
            ON su.summonee_login = persons.staff_user_login
);


INSERT INTO $dst_table WITH TRUNCATE
SELECT
    si.issue_id         AS issue_id,
    si.staff_user_id    AS staff_user_id,
    min(dttm)           AS first_summon_dttm,
FROM $summonee_ids AS si
GROUP BY si.issue_id, si.staff_user_id
ORDER BY issue_id, staff_user_id
;
