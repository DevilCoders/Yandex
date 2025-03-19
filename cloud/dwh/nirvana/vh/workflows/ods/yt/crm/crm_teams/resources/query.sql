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
        $toString(`associated_user_id`)                                                 AS associated_user_id,
        $toString(`created_by`)                                                         AS created_by,
        $get_timestamp(`date_entered`)                                                  AS date_entered_ts,
        $from_utc_ts_to_msk_dt($get_timestamp(`date_entered`))                          AS date_entered_dttm_local,
        $get_timestamp(`date_modified`)                                                 AS date_modified_ts,
        $from_utc_ts_to_msk_dt($get_timestamp(`date_modified`))                         AS date_modified_dttm_local,
        CAST(`deleted` AS bool)                                                         AS deleted,
        $toString($decode_utf8(`description`))                                          AS crm_team_description,
        $toString(`id`)                                                                 AS crm_team_id,
        $toString(`modified_user_id`)                                                   AS modified_user_id,
        $toString(`name`)                                                               AS crm_team_name,
        $toString(`name_2`)                                                             AS crm_team_name_2,
        CAST(`private` AS bool)                                                         AS private,
        $toString(`segment`)                                                            AS segment
    FROM $src_table
);

/* Save result in ODS */
INSERT INTO $dst_table WITH TRUNCATE
SELECT 
    associated_user_id                                                                  AS associated_user_id,
    created_by                                                                          AS created_by,
    date_entered_ts                                                                     AS date_entered_ts,
    date_entered_dttm_local                                                             AS date_entered_dttm_local,
    date_modified_ts                                                                    AS date_modified_ts,
    date_modified_dttm_local                                                            AS date_modified_dttm_local,
    deleted                                                                             AS deleted,
    $get_md5(crm_team_description)                                                      AS crm_team_description_hash,
    crm_team_id                                                                         AS crm_team_id,
    modified_user_id                                                                    AS modified_user_id,
    $get_md5(crm_team_name)                                                             AS crm_team_name_hash,
    $get_md5(crm_team_name_2)                                                           AS crm_team_name_2_hash,
    private                                                                             AS private,
    segment                                                                             AS segment 
FROM $result
ORDER BY crm_team_id;

/* Save result in ODS PII*/
INSERT INTO $pii_dst_table WITH TRUNCATE
SELECT 
    crm_team_id                                     AS crm_team_id,
    crm_team_description                            AS crm_team_description,
    crm_team_name                                   AS crm_team_name,
    crm_team_name_2                                 as crm_team_name_2
FROM $result
ORDER BY crm_team_id;
