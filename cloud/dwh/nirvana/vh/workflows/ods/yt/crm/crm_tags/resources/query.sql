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
        $toString(`assigned_user_id`)                                                               AS assigned_user_id,
        $toString(`created_by`)                                                                     AS created_by,
        $get_timestamp(`date_entered`)                                                              AS date_entered_ts,
        $from_utc_ts_to_msk_dt($get_timestamp(`date_entered`))                                      AS date_entered_dttm_local,
        $get_timestamp(`date_modified`)                                                             AS date_modified_ts,
        $from_utc_ts_to_msk_dt($get_timestamp(`date_modified`))                                     AS date_modified_dttm_local,
        CAST(`deleted` AS bool)                                                                     AS deleted,
        $toString(`description`)                                                                    AS crm_tag_description,
        $toString(`id`)                                                                             AS crm_tag_id,
        $toString(`modified_user_id`)                                                               AS modified_user_id,
        $toString(`name`)                                                                           AS crm_tag_name,
        $toString(`name_lower`)                                                                     AS crm_tag_name_lower,
        $toString(`source_id`)                                                                      AS crm_source_id,
        $toString(`source_meta`)                                                                    AS source_meta,
        $toString(`source_type`)                                                                    AS source_type
    FROM $src_table
);

/* Save result in ODS */
INSERT INTO $dst_table WITH TRUNCATE
SELECT * FROM $result
ORDER BY `crm_tag_id`;
