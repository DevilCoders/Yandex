$issues_table = {{ param["issues_ods_table"] -> quote() }};
$issue_components_table = {{ param["issue_components_ods_table"] -> quote() }};
$components_table = {{ param["components_ods_table"] -> quote() }};
$dst_table = {{ input1 -> table_quote() }};

$get_date = ($field) -> (CAST($field AS Date));

$component_mart_projection = (
    SELECT
        issue_components_link.component_id  AS component_id,
        $get_date(issues.created_at)        AS issue_creation_date,
        issues.issue_id                     AS issue_id,
        issues.key                          AS issue_key,
        issues.payment_tariff               AS payment_tariff
    FROM $issue_components_table AS issue_components_link
        INNER JOIN $issues_table AS issues
            ON issue_components_link.issue_id = issues.issue_id
);


INSERT INTO $dst_table WITH TRUNCATE
    SELECT
        components.component_id     AS component_id,
        components.name             AS component_name,
        proj.issue_id               AS issue_id,
        proj.issue_key              AS issue_key,
        proj.payment_tariff         AS payment_tariff,
        proj.issue_creation_date    AS issue_creation_date
    FROM $component_mart_projection AS proj
        INNER JOIN $components_table AS components
            ON proj.component_id = components.component_id
    ORDER BY component_id, issue_id
;
