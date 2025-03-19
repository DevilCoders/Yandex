PRAGMA Library('datetime.sql');
IMPORT `datetime` SYMBOLS $format_msk_date_by_timestamp;
IMPORT `datetime` SYMBOLS $format_msk_datetime_by_timestamp;

$billing_accounts_hist_table = {{ param["billing_accounts_hist_ods_table"] -> quote() }};
$crm_tags_table = {{ param["ba_crm_tags_stg_table"] -> quote() }};
$dst_table = {{ input1 -> table_quote() }};

$get_date = ($field) -> (CAST($field AS Date));
$parse_dttm = DateTime::Parse("%Y-%m-%d %H:%M:%S");
$get_datetime = ($field) -> (DateTime::MakeDatetime($field));

$cast_billing_account_type = ($field) -> (
    CASE
        WHEN String::Contains($field, 'company')    THEN 'company' 
        WHEN String::Contains($field, 'individual') THEN 'individual'
        WHEN String::Contains($field, 'internal')   THEN 'internal'
        ELSE 'unknown'
    END
);

$cast_office = ($field) -> (
    CASE
        WHEN $field = 'switzerland_nonresident_company' THEN 'switzerland'
        WHEN $field IN ('kazakhstan_company', 'kazakhstan_individual') THEN 'kazakhstan'
        WHEN $field IS NULL OR $field IN ('company', 'individual', 'internal') THEN 'russia'
        ELSE 'unknown'
    END
);

$billing_accounts_table = (
    SELECT
        IF(change_id = 1, 'create', 'billing_account_update') AS action_type,
        updated_at,
        $get_datetime($parse_dttm($format_msk_datetime_by_timestamp(updated_at))) AS updated_at_msk,
        billing_account_id,
        person_type,
        is_var,
        is_subaccount,
        is_suspended_by_antifraud,
        is_isv,
        country_code
    FROM (
        SELECT
            ht.*,
            ROW_NUMBER() OVER (PARTITION BY billing_account_id ORDER BY updated_at ASC) AS change_id
        FROM $billing_accounts_hist_table AS ht
    ) AS t
);

$ba_segments = (
    SELECT
        ba.billing_account_id   AS billing_account_id,
        crm_tags.segment        AS segment,
        ba.updated_at           AS updated_at
    FROM $billing_accounts_table AS ba
        LEFT JOIN $crm_tags_table as crm_tags
            ON ba.billing_account_id = crm_tags.billing_account_id
    WHERE crm_tags.`date` = $format_msk_date_by_timestamp(ba.updated_at)
);


INSERT INTO $dst_table WITH TRUNCATE
    SELECT
        ba.action_type                          AS `action`,
        ba.billing_account_id                   AS `billing_account_id`,
        $cast_billing_account_type(person_type) AS `billing_account_type`,
        country_code                            AS `country_code`,
        segments.segment                        AS `crm_segment`,
        $get_date(updated_at_msk)               AS `event_dt`,
        updated_at_msk                          AS `event_dttm`,
        is_isv                                  AS `is_isv`,
        is_subaccount                           AS `is_subaccount`,
        is_suspended_by_antifraud               AS `is_suspended_by_antifraud`,
        is_var                                  AS `is_var`,
        person_type                             AS `person_type`,
        $cast_office(person_type)               AS `yandex_office`
    FROM $billing_accounts_table AS ba
        LEFT JOIN $ba_segments as segments
            ON ba.billing_account_id = segments.billing_account_id AND ba.updated_at = segments.updated_at
;



