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
        $toString(`bean_id`)                                                        AS bean_id,
        $toString(`bean_module`)                                                    AS bean_module,
        $get_timestamp(`date_created`)                                              AS date_created_ts,
        $from_utc_ts_to_msk_dt($get_timestamp(`date_created`))                      AS date_created_dttm_local,
        $get_timestamp(`date_modified`)                                             AS date_modified_ts,
        $from_utc_ts_to_msk_dt($get_timestamp(`date_modified`))                     AS date_modified_dttm_local,
        CAST(`deleted` AS bool)                                                     AS deleted,
        $toString(`email_address_id`)                                               AS crm_email_id,
        $toString(`id`)                                                             AS id,
        CAST(`primary_address` AS bool)                                             AS primary_address,
        CAST(`reply_to_address` AS bool)                                            AS reply_to_address
    FROM $src_table
);

/* Save result in ODS */
INSERT INTO $dst_table WITH TRUNCATE
SELECT * FROM $result
ORDER BY `id`;
