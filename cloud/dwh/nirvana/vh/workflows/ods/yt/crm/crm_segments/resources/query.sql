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
        $toString(`account_id`)                                                       AS crm_account_id,
        $toString(`acl_team_set_id`)                                                  AS acl_crm_team_set_id,
        $toString(`assigned_user_id`)                                                 AS assigned_user_id,
        $toString(`created_by`)                                                       AS created_by,
        $get_datetime(`date_from`)                                                    AS date_from,
        $get_datetime(`date_from`)                                                    AS date_from_ts,
        $from_utc_ts_to_msk_dt($get_timestamp(`date_from`))                           AS date_from_dttm_local,
        $get_datetime(`date_to`)                                                      AS date_to,
        $get_datetime(`date_to`)                                                      AS date_to_ts,
        $from_utc_ts_to_msk_dt($get_timestamp(`date_to`))                             AS date_to_dttm_local,
        $get_timestamp(`date_entered`)                                                AS date_entered_ts,
        $from_utc_ts_to_msk_dt($get_timestamp(`date_entered`))                        AS date_entered_dttm_local,
        $get_timestamp(`date_modified`)                                               AS date_modified_ts,
        $from_utc_ts_to_msk_dt($get_timestamp(`date_modified`))                       AS date_modified_dttm_local,
        CAST(`default_segment` AS bool)                                               AS default_segment,
        CAST(`deleted` AS bool)                                                       AS deleted,
        $toString(`description`)                                                      AS crm_segment_description,
        $toString(`id`)                                                               AS crm_segment_id,
        $toString(`modified_user_id`)                                                 AS modified_user_id,
        $toString(`name`)                                                             AS crm_segment_name,
        $toString(`segment_enum`)                                                     AS segment_enum,
        CAST(`segment_manually` AS bool)                                              AS segment_manually,
        $toString(`team_id`)                                                          AS crm_team_id,
        $toString(`team_set_id`)                                                      AS crm_team_set_id
    FROM $src_table
);

/* Save result in ODS */
INSERT INTO $dst_table WITH TRUNCATE
SELECT * FROM $result
ORDER BY `crm_account_id`, `date_from`;
