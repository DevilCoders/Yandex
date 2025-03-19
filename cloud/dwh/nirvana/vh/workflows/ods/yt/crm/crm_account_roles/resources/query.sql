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
        $toString(`account_id`)                                                       AS crm_account_id,
        $toString(`assigned_user_id`)                                                 AS assigned_user_id,
        $toString(`created_by`)                                                       AS created_by,
        $get_datetime($get_timestamp_from_days(CAST(`date_from` AS Uint64)))          AS date_from,
        $get_timestamp(`date_from`)                                                   AS date_from_ts,
        $from_utc_ts_to_msk_dt($get_timestamp(`date_from`))                           AS date_from_dttm_local,
        $get_datetime($get_timestamp_from_days(CAST(`date_to` AS Uint64)))            AS date_to,
        $get_timestamp(`date_to`)                                                     AS date_to_ts,
        $from_utc_ts_to_msk_dt($get_timestamp(`date_to`))                             AS date_to_dttm_local,
        $get_timestamp(`date_entered`)                                                AS date_entered_ts,
        $from_utc_ts_to_msk_dt($get_timestamp(`date_entered`))                        AS date_entered_dttm_local,
        $get_timestamp(`date_modified`)                                               AS date_modified_ts,
        $from_utc_ts_to_msk_dt($get_timestamp(`date_modified`))                       AS date_modified_dttm_local,
        cast(`deleted` AS bool)                                                       AS deleted,
        $toString($decode_utf8(`description`) )                                       AS crm_account_role_description,
        $toString(`id`                        )                                       AS crm_account_role_id,
        $toString(`last_confirmator_id`       )                                       AS last_confirmator_id,
        $toString(`modified_user_id`          )                                       AS modified_user_id,
        $toString(`name`                      )                                       AS crm_account_role_name,
        $toString(`requester_id`              )                                       AS requester_id,
        $toString(`role`                      )                                       AS crm_role_name,
        $toString(`segment_id`                )                                       AS segment_id,
        `share`                                                                       AS share,
        $toString(`status`                    )                                       AS status
    FROM $src_table
);

/* Save result in ODS */
INSERT INTO $dst_table WITH TRUNCATE
SELECT 
    crm_account_id                                                                 AS crm_account_id,
    assigned_user_id                                                               AS assigned_user_id,
    created_by                                                                     AS created_by,
    date_entered_ts                                                                AS date_entered_ts,
    date_entered_dttm_local                                                        AS date_entered_dttm_local,
    date_from                                                                      AS date_from,
    date_from_ts                                                                   AS date_from_ts,
    date_from_dttm_local                                                           AS date_from_dttm_local,
    date_modified_ts                                                               AS date_modified_ts,
    date_modified_dttm_local                                                       AS date_modified_dttm_local,
    date_to                                                                        AS date_to,
    date_to_ts                                                                     AS date_to_ts,
    date_to_dttm_local                                                             AS date_to_dttm_local,
    deleted                                                                        AS deleted,
    crm_account_role_description                                                   AS crm_account_role_description,
    crm_account_role_id                                                            AS crm_account_role_id,
    last_confirmator_id                                                            AS last_confirmator_id,
    modified_user_id                                                               AS modified_user_id,
    $get_md5(crm_account_role_name)                                                AS crm_account_role_name_hash,
    requester_id                                                                   AS requester_id,
    crm_role_name                                                                  AS crm_role_name,
    segment_id                                                                     AS segment_id,
    share                                                                          AS share,
    status                                                                         AS status
FROM $result
ORDER BY `crm_account_id`, `date_from`;


/* Save result in ODS PII*/
INSERT INTO $pii_dst_table WITH TRUNCATE
SELECT
    crm_account_role_id                                                             AS crm_account_role_id,
    crm_account_role_name                                                           AS crm_account_role_name
FROM $result;
