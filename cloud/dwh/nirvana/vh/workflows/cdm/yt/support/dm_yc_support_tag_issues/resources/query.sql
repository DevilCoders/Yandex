$issues_table = {{ param["issues_ods_table"] -> quote() }};
$issue_tags_table = {{ param["issue_tags_ods_table"] -> quote() }};
$dst_table = {{ input1 -> table_quote() }};

$get_date = ($field) -> (CAST($field AS Date));

$tag_mart_projection = (
    SELECT
        issue_tags.tag                  AS tag,
        $get_date(issues.created_at)    AS issue_creation_date,
        issues.issue_id                 AS issue_id,
        issues.key                      AS issue_key,
        issues.payment_tariff           AS payment_tariff
    FROM $issue_tags_table AS issue_tags
        INNER JOIN $issues_table AS issues
            ON issue_tags.issue_id = issues.issue_id
);


INSERT INTO $dst_table WITH TRUNCATE
    SELECT *
    FROM $tag_mart_projection as tags
    ORDER BY tag, issue_id
;
