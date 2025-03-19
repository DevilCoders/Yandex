PRAGMA Library("datetime.sql");

IMPORT `datetime` SYMBOLS $get_datetime, $get_timestamp_from_days;

$src_table = {{ param["source_table_path"]->quote() }};
$dst_table = {{ input1->table_quote() }};

$toString = ($data) -> (CAST($data AS String));
$decode_utf8 = ($data) -> (nvl(cast(String::Base64StrictDecode($data) as Utf8),$data));

$get_timestamp = ($ts) -> (CAST($ts AS TimeStamp));
$from_utc_ts_to_msk_dt = ($ts) -> (DateTime::MakeDatetime(DateTime::Update(AddTimeZone($ts, "Europe/Moscow"), "GMT"  as Timezone)));

$result = (
    SELECT
        $toString(`acl_team_set_id`)                                                        AS acl_crm_team_set_id,
        `amount`                                                                            AS amount,
        `amount_usdollar`                                                                   AS amount_usdollar,
        $toString(`assigned_user_id`)                                                       AS assigned_user_id,
        `ba_base_rate`                                                                      AS ba_base_rate,
        $toString(`ba_currency_id`)                                                         AS ba_currency_id,
        `base_rate`                                                                         AS base_rate,
        `baseline_trend`                                                                    AS baseline_trend,
        `best_case`                                                                         AS best_case,
        $toString(`block_reason`)                                                           AS block_reason,
        $toString(`campaign_id`)                                                            AS crm_campaign_id,
        `closed_revenue_line_items`                                                         AS closed_revenue_line_items,
        $toString(`commit_stage`)                                                           AS commit_stage,
        $toString(`consumption_type`)                                                       AS consumption_type,
        $toString(`created_by`)                                                             AS created_by,
        $toString(`currency_id`)                                                            AS crm_currency_id,
        `date_closed`                                                                       AS date_closed,
        $toString(`date_closed_by`)                                                         AS date_closed_by,
        `date_closed_month`                                                                 AS date_closed_month,
        `date_closed_timestamp`                                                             AS date_closed_timestamp,--?
        $get_timestamp(`date_entered`)                                                      AS date_entered_ts,
        $from_utc_ts_to_msk_dt($get_timestamp(`date_entered`))                              AS date_entered_dttm_local,
        $get_timestamp(`date_modified`)                                                     AS date_modified_ts,
        $from_utc_ts_to_msk_dt($get_timestamp(`date_modified`))                             AS date_modified_dttm_local,
        CAST(`deleted` AS bool)                                                             AS deleted,
        $decode_utf8(`description`)                                                         AS crm_opportunity_description,
        `discount`                                                                          AS discount,
        $toString(`display_status`)                                                         AS display_status,
        $toString(`dri_workflow_template_id`)                                               AS dri_workflow_template_id,
        `first_month_consumption`                                                           AS first_month_consumption,
        `first_trial_consumption_date`                                                      AS first_trial_consumption_date,
        $toString(`forecast`)                                                               AS forecast,
        `full_capacity_date`                                                                AS full_capacity_date,
        $toString(`id`)                                                                     AS crm_opportunity_id,
        `included_revenue_line_items`                                                       AS included_revenue_line_items,
        `initial_capacity`                                                                  AS initial_capacity,
        $toString(`lead_source`)                                                            AS lead_source,
        $toString(`lead_source_description`)                                                AS lead_source_description,
        `likely_daily`                                                                      AS likely_daily,
        $toString(`likely_formula`)                                                         AS likely_formula,
        `linked_total_calls`                                                                AS linked_total_calls,
        `linked_total_notes`                                                                AS linked_total_notes,
        `linked_total_quotes`                                                               AS linked_total_quotes,
        `linked_total_tasks`                                                                AS linked_total_tasks,
        $toString(`lost_reason`)                                                            AS lost_reason,
        $toString($decode_utf8(`lost_reason_description`))                                  AS lost_reason_description,
        $toString(`modified_user_id`)                                                       AS modified_user_id,
        $toString(`name`)                                                                   AS crm_opportunity_name,
        $toString(`next_step`)                                                              AS next_step,
        CAST(`non_recurring` AS bool)                                                       AS non_recurring,
        CAST(`old_format_dimensions` AS bool)                                               AS old_format_dimensions,
        $toString(`opportunity_type`)                                                       AS opportunity_type,
        `oppty_id`                                                                          AS oppty_id,
        `paid_consumption`                                                                  AS paid_consumption,
        $toString(`partner_id`)                                                             AS partner_id,
        $toString(`partners_value`)                                                         AS partners_value,
        $toString(`person_type`)                                                            AS person_type,
        `probability`                                                                       AS probability,
        $toString(`probability_enum`)                                                       AS probability_enum,
        `probability_new`                                                                   AS probability_new,
        $toString(`revenue_ordinated`)                                                      AS revenue_ordinated,
        $toString(`sales_stage`)                                                            AS sales_stage,
        $toString(`sales_status`)                                                           AS sales_status,
        $toString(`scenario`)                                                               AS scenario,
        $toString(`segment_ba`)                                                             AS segment_ba,
        $toString(`service_type`)                                                           AS service_type,
        `services_total`                                                                    AS services_total,
        `services_total_usdollar`                                                           AS services_total_usdollar,
        $toString(`taxrate_id`)                                                             AS tax_rate_id,
        `taxrate_value`                                                                     AS tax_rate_value,
        $toString(`team_id`)                                                                AS crm_team_id,
        $toString(`team_set_id`)                                                            AS crm_team_set_id,
        `total_revenue_line_items`                                                          AS total_revenue_line_items,
        $toString(`tracker_number`)                                                         AS tracker_number,
        `trial_consumption`                                                                 AS trial_consumption,
        `worst_case`                                                                        AS worst_case
    FROM $src_table
);

/* Save result in ODS */
INSERT INTO $dst_table WITH TRUNCATE
SELECT * FROM $result
ORDER BY `crm_opportunity_id`;
