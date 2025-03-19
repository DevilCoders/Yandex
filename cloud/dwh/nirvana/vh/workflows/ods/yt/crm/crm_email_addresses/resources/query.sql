PRAGMA Library("datetime.sql");
PRAGMA Library("helpers.sql");

IMPORT `datetime` SYMBOLS $get_datetime, $get_timestamp_from_days;
IMPORT `helpers` SYMBOLS $get_md5;

$src_table = {{ param["source_table_path"]->quote() }};
$dst_table = {{ input1->table_quote() }};
$pii_dst_table = {{ param["pii_destination_path"] -> quote() }};

$toString = ($data) -> (CAST($data AS String));
$decode_utf8 = ($data) -> (nvl(cast(String::Base64StrictDecode($data) as Utf8),$data));

$get_timestamp = ($ts) -> (CAST($ts AS TimeStamp));
$from_utc_ts_to_msk_dt = ($ts) -> (DateTime::MakeDatetime(DateTime::Update(AddTimeZone($ts, "Europe/Moscow"), "GMT"  as Timezone)));

$result = (
    SELECT
        $get_timestamp(`date_created`)                                              AS date_created_ts,
        $from_utc_ts_to_msk_dt($get_timestamp(`date_created`))                      AS date_created_dttm_local,
        $get_timestamp(`date_modified`)                                             AS date_modified_ts,
        $from_utc_ts_to_msk_dt($get_timestamp(`date_modified`))                     AS date_modified_dttm_local,
        CAST(`deleted` AS bool)                                                     AS deleted,
        $toString(`email_address`)                                                  AS email_address,
        $toString(`email_address_caps`)                                             AS email_address_caps,
        $toString(`id`)                                                             AS crm_email_id,
        CAST(`invalid_email` AS bool)                                               AS invalid_email,
        CAST(`opt_out` AS bool)                                                     AS opt_out
    FROM $src_table
);

/* Save result in ODS */
INSERT INTO $dst_table WITH TRUNCATE
SELECT
    crm_email_id                    AS crm_email_id,
    date_created_ts                 AS date_created_ts,
    date_created_dttm_local         AS date_created_dttm_local,
    date_modified_ts                AS date_modified_ts,
    date_modified_dttm_local        AS date_modified_dttm_local,
    deleted                         AS deleted,
    $get_md5(email_address)         AS email_address_hash,
    $get_md5(email_address_caps)    AS email_address_caps_hash,
    invalid_email                   AS invalid_email,
    opt_out                         AS opt_out
FROM $result;

/* Save result in ODS PII*/
INSERT INTO $pii_dst_table WITH TRUNCATE
SELECT 
    crm_email_id                    AS crm_email_id,
    email_address                   AS email_address,
    email_address_caps              AS email_address_caps
FROM $result;
