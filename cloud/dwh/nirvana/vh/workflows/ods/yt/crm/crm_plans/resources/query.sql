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
        $toString(`account_id`)                                                     AS crm_account_id,
        $toString(`acl_team_set_id`)                                                AS acl_crm_team_set_id,
        $toString(`assigned_user_id`)                                               AS assigned_user_id,
        `base_rate`                                                                 AS base_rate,
        $toString(`created_by`)                                                     AS created_by,
        $toString(`currency_id`)                                                    AS crm_currency_id,
        $get_timestamp(`date_entered`)                                              AS date_entered_ts,
        $from_utc_ts_to_msk_dt($get_timestamp(`date_entered`))                      AS date_entered_dttm_local,
        $get_timestamp(`date_modified`)                                             AS date_modified_ts,
        $from_utc_ts_to_msk_dt($get_timestamp(`date_modified`))                     AS date_modified_dttm_local,
        CAST(`deleted` AS bool)                                                     AS deleted,
        $toString(`description`)                                                    AS crm_plan_description,
        $toString(`id`)                                                             AS crm_plan_id,
        CAST(`is_target` AS bool)                                                   AS is_target,
        $toString(`modified_user_id`)                                               AS modified_user_id,
        $toString(`name`)                                                           AS crm_plan_name,
        $toString(`plan_type`)                                                      AS plan_type,
        $toString(`segment`)                                                        AS segment,
        $toString(`selectedTimePeriod`)                                             AS selected_time_period,
        $toString(`status`)                                                         AS status,
        $toString(`team_id`)                                                        AS crm_team_id,
        $toString(`team_set_id`)                                                    AS crm_team_set_id,
        `value_currency`                                                            AS value_currency,
        CAST(`value_logical` AS bool)                                               AS value_logical,
        $toString(`value_numeric`)                                                  AS value_numeric,
        $toString(`value_type`)                                                     AS value_type,
        `weight`                                                                    AS weight
    FROM $src_table
);

/* Save result in ODS */
INSERT INTO $dst_table WITH TRUNCATE
SELECT * FROM $result
ORDER BY `crm_plan_id`;
