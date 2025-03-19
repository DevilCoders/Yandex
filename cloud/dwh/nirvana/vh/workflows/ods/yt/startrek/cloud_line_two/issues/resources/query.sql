PRAGMA Library("datetime.sql");
PRAGMA Library("helpers.sql");

IMPORT `datetime` SYMBOLS $get_datetime_ms;
IMPORT `helpers` SYMBOLS $lookup_string;

$src_table = {{ param["source_path"] -> quote() }};
$dst_table = {{ input1 -> table_quote() }};

$get_string = ($custom_fields, $field) -> ($lookup_string($custom_fields, $field, NULL));
$get_int = ($custom_fields, $field) -> (CAST($get_string($custom_fields, $field) AS Int32?));
$cast_dt = ($field) -> (DateTime::MakeDatetime($field));

$cleanse_tariff = ($payment_tariff) -> (
    CASE
        WHEN $payment_tariff IN ('business', 'premium')                             THEN $payment_tariff
        WHEN $payment_tariff IN ('Standard', 'standard', 'standart', 'standatd')    THEN 'standard'
        ELSE 'free'
    END
);

$extract_issue_data = (
    SELECT
        $get_datetime_ms(created)                       AS created_at, 
        $get_datetime_ms(updated)                       AS updated_at, 
        $get_datetime_ms(resolved)                      AS resolved_at,
        key                                             AS key,
        type                                            AS type_id,
        $get_string(`customFields`, 'pay')              AS payment_tariff,
        $get_string(`customFields`, 'billingId')        AS billing_account_id,
        $get_string(`customFields`, 'customerSegment')  AS tracker_customer_segment,
        $get_int(`customFields`, 'cloudFeedbackThree')  AS feedback_reaction_speed,
        $get_int(`customFields`, 'cloudFeedbackTwo')    AS feedback_response_completeness,
        $get_int(`customFields`, 'cloudFeedbackOne')    AS feedback_general,
        id                                              AS issue_id,
        author                                          AS author_id,
        `hashKey`                                       AS hash_key,
        `version`                                       AS version,
        `aliases`                                       AS aliases,
        `queue`                                         AS queue,
        `status`                                        AS status,
        `resolution`                                    AS resolution,
        `summary`                                       AS summary,
        `description`                                   AS description,
        `start`                                         AS start,
        `end`                                           AS end,
        `dueDate`                                       AS due_date,
        `resolver`                                      AS resolver,
        `modifier`                                      AS modifier,
        `assignee`                                      AS assignee,
        `priority`                                      AS priority,
        `affectedVersions`                              AS affected_versions,
        `fixVersions`                                   AS fix_versions,
        `components`                                    AS components,
        `tags`                                          AS tags,
        `sprint`                                        AS sprint,
        `customFields`                                  AS custom_fields,
        `followers`                                     AS followers,
        `access`                                        AS access,
        `unique`                                        AS unique,
        `followingGroups`                               AS following_groups,
        `followingMaillists`                            AS following_maillists,
        `parent`                                        AS parent,
        `epic`                                          AS epic,
        `originalEstimation`                            AS original_estimation,
        `estimation`                                    AS estimation,
        `spent`                                         AS spent,
        `storyPoints`                                   AS story_points,
        `ranks`                                         AS ranks,
        `ranking`                                       AS ranking,
        `goals`                                         AS goals,
        `votedBy`                                       AS voted_by,
        `favoritedBy`                                   AS favorited_by,
        `emailFrom`                                     AS email_from,
        `emailTo`                                       AS email_to,
        `emailCc`                                       AS email_cc,
        `emailCreatedBy`                                AS email_created_by,
        `sla`                                           AS sla,
        `orgId`                                         AS org_id
    FROM
        $src_table
);

$transform_issue_data = (
    SELECT
        $cast_dt(created_at)    AS created_at,
        $cast_dt(updated_at)    AS updated_at, 
        $cast_dt(resolved_at)   AS resolved_at,
        key,
        type_id,
        author_id,
        $cleanse_tariff(payment_tariff) AS payment_tariff,
        billing_account_id,
        tracker_customer_segment,
        feedback_reaction_speed,
        feedback_response_completeness,
        feedback_general,
        issue_id,
        hash_key,
        version,
        aliases,
        queue,
        status,
        resolution,
        summary,
        description,
        start,
        end,
        due_date,
        resolver,
        modifier,
        assignee,
        priority,
        affected_versions,
        fix_versions,
        components,
        tags,
        sprint,
        custom_fields,
        followers,
        access,
        unique,
        following_groups,
        following_maillists,
        parent,
        epic,
        original_estimation,
        estimation,
        spent,
        story_points,
        ranks,
        ranking,
        goals,
        voted_by,
        favorited_by,
        email_from,
        email_to,
        email_cc,
        email_created_by,
        sla,
        org_id
    FROM $extract_issue_data
);

INSERT INTO $dst_table WITH TRUNCATE
    SELECT *
    FROM $transform_issue_data
    ORDER BY issue_id;