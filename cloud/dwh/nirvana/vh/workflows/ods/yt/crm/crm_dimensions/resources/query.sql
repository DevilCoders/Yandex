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
        $toString(`assigned_user_id`)                                                 AS assigned_user_id,
        $toString(`category_id`)                                                      AS crm_category_id,
        $toString(`color`)                                                            AS color,
        $toString(`created_by`)                                                       AS created_by,
        $get_timestamp(`date_entered`)                                                AS date_entered_ts,
        $from_utc_ts_to_msk_dt($get_timestamp(`date_entered`))                        AS date_entered_dttm_local,
        $get_timestamp(`date_modified`)                                               AS date_modified_ts,
        $from_utc_ts_to_msk_dt($get_timestamp(`date_modified`))                       AS date_modified_dttm_local,
        CAST(`deleted` AS bool)                                                       AS deleted,
        $toString(`description`)                                                      AS crm_dimension_description,
        CAST(`disable_use_alone` AS bool)                                             AS disable_use_alone,
        $toString(`id`)                                                               AS crm_dimension_id,
        `level`                                                                       AS level,
        $toString(`modified_user_id`)                                                 AS modified_user_id,
        $toString(`name`)                                                             AS crm_dimension_name,
        $toString(`parent_id`)                                                        AS parent_id,
        $toString(`root_id`)                                                          AS root_id
    FROM $src_table
);

/* Save result in ODS */
INSERT INTO $dst_table WITH TRUNCATE
SELECT * FROM $result
ORDER BY `crm_dimension_id`;
