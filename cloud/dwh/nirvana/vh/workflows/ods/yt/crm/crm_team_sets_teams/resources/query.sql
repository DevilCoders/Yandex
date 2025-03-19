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
        $get_timestamp(`date_modified`)                                             AS date_modified_ts,
        $from_utc_ts_to_msk_dt($get_timestamp(`date_modified`))                     AS date_modified_dttm_local,
        CAST(`deleted` AS bool)                                                     AS deleted,
        $toString(`id`)                                                             AS id,
        $toString(`team_id`)                                                        AS crm_team_id,
        $toString(`team_set_id`)                                                    AS crm_team_set_id
    FROM $src_table
);

/* Save result in ODS */
INSERT INTO $dst_table WITH TRUNCATE
SELECT * FROM $result
ORDER BY `crm_team_id`, `crm_team_set_id`;
