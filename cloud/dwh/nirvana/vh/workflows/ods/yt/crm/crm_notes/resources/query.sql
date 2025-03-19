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
        $toString(`acl_team_set_id`)                                                AS acl_crm_team_set_id,
        $toString(`assigned_user_id`)                                               AS assigned_user_id,
        $toString(`contact_id`)                                                     AS crm_contact_id,
        $toString(`created_by`)                                                     AS created_by,
        $get_timestamp(`date_entered`)                                              AS date_entered_ts,
        $from_utc_ts_to_msk_dt($get_timestamp(`date_entered`))                      AS date_entered_dttm_local,
        $get_timestamp(`date_modified`)                                             AS date_modified_ts,
        $from_utc_ts_to_msk_dt($get_timestamp(`date_modified`))                     AS date_modified_dttm_local,
        CAST(`deleted` AS bool)                                                     AS deleted,
        $toString($decode_utf8(`description`))                                      AS crm_note_description,
        $toString(`email_id`)                                                       AS crm_email_id,
        $toString(`email_type`)                                                     AS email_type,
        $toString(`id`)                                                             AS crm_note_id,
        $toString(`modified_user_id`)                                               AS modified_user_id,
        $toString(`name`)                                                           AS crm_note_name,
        $toString(`parent_id`)                                                      AS parent_id,
        $toString(`parent_type`)                                                    AS parent_type,
        $toString(`team_id`)                                                        AS crm_team_id,
        $toString(`team_set_id`)                                                    AS crm_team_set_id,
        $toString(`upload_id`)                                                      AS upload_id
    FROM $src_table
);

/* Save result in ODS */
INSERT INTO $dst_table WITH TRUNCATE
SELECT 
    acl_crm_team_set_id                                                        AS acl_crm_team_set_id,
    assigned_user_id                                                           AS assigned_user_id,
    crm_contact_id                                                             AS crm_contact_id,
    created_by                                                                 AS created_by,
    date_entered_ts                                                            AS date_entered_ts,
    date_entered_dttm_local                                                    AS date_entered_dttm_local,
    date_modified_ts                                                           AS date_modified_ts,
    date_modified_dttm_local                                                   AS date_modified_dttm_local,
    deleted                                                                    AS deleted,
    $get_md5(crm_note_description)                                             AS crm_note_description_hash,
    crm_email_id                                                               AS crm_email_id,
    email_type                                                                 AS email_type,
    crm_note_id                                                                AS crm_note_id,
    modified_user_id                                                           AS modified_user_id,
    crm_note_name                                                              AS crm_note_name,
    parent_id                                                                  AS parent_id,
    parent_type                                                                AS parent_type,
    crm_team_id                                                                AS crm_team_id,
    crm_team_set_id                                                            AS crm_team_set_id,
    upload_id                                                                  AS upload_id
FROM $result
ORDER BY `crm_note_id`;


/* Save result in ODS PII*/
INSERT INTO $pii_dst_table WITH TRUNCATE
SELECT 
    crm_note_id                                                                 AS crm_note_id,
    crm_note_description                                                        AS crm_note_description
FROM $result
ORDER BY `crm_note_id`;

