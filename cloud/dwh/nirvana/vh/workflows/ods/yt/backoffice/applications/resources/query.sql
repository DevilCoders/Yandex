PRAGMA Library("datetime.sql");

IMPORT `datetime` SYMBOLS $parse_iso8601_to_datetime_msk, $from_utc_ts_to_msk_dt, $from_int64_to_ts;


$src_table        = {{ param["source_folder_path"] -> quote() }};
$dst_table_pii    = {{ param["PII_destination_path"] -> quote() }};
$dst_table_hashed = {{ input1 -> table_quote() }};

$lookup_str = ($json, $field) -> (
    Yson::LookupString(Yson::ParseJson($json, Yson::Options(false as Strict)), $field, Yson::Options(false as Strict))
);


$result = (
    SELECT
        id,
        `uuid`,
        status,
        $parse_iso8601_to_datetime_msk(created_at)                   AS created_at_msk,
        $from_utc_ts_to_msk_dt($from_int64_to_ts(created_at))        AS created_at_dttm_local,
        $from_int64_to_ts(created_at)                                AS created_at_ts,
        $parse_iso8601_to_datetime_msk(updated_at)                   AS updated_at_msk,
        event_id,
        participant_id,
        visited,
        language,
        visit_type,
        -- PII
        $lookup_str(flat_data, "name")                               AS participant_name,
        $lookup_str(flat_data, "last_name")                          AS participant_last_name,
        $lookup_str(flat_data, "work")                               AS participant_company_name,
        $lookup_str(flat_data, "industry")                           AS participant_company_industry,
        String::ToLower($lookup_str(flat_data, "position"))          AS participant_position,
        $lookup_str(flat_data, "phone")                              AS participant_phone,
        $lookup_str(flat_data, "email")                              AS participant_email,
        -- additional info
        $lookup_str(flat_data, "city")                               AS participant_city,
        $lookup_str(flat_data, "answer_choices_10334855")            AS participant_company_size,
        $lookup_str(flat_data, "website")                            AS participant_website,
        $lookup_str(flat_data, "user_status")                        AS participant_has_yacloud_account,
        $lookup_str(flat_data, "answer_choices_10222008")            AS participant_scenario,
        $lookup_str(flat_data, "answer_choices_10222012")            AS participant_need_manager_call,
        $lookup_str(flat_data, "answer_choices_10231648")            AS participant_infrastructure_type,
        $lookup_str(flat_data, "answer_choices_10222014")            AS participant_how_did_know_about_event,
        $lookup_str(flat_data, "utm_source")                         AS participant_utm_source,
        $lookup_str(flat_data, "utm_campaign")                       AS participant_utm_campaign,
        COALESCE(
            $lookup_str(flat_data, "answer_boolean_10222015"),
            $lookup_str(flat_data, "answer_boolean_10166162"),
            $lookup_str(flat_data, "agreementNewsletters")
        )                                                            AS participant_agree_to_communicate,
    FROM $src_table
);

$pii_version = (
    SELECT
        id,
        `uuid`,
       participant_name,
       participant_last_name,
       participant_company_name,
       participant_position,
       participant_phone,
       participant_email
    FROM $result
);

$hashed_version = (
    SELECT
        r.*,
        Digest::Md5Hex(participant_name)             AS participant_name_hash,
        Digest::Md5Hex(participant_last_name)        AS participant_last_name_hash,
        Digest::Md5Hex(participant_company_name)     AS participant_company_name_hash,
        Digest::Md5Hex(participant_position)         AS participant_position_hash,
        Digest::Md5Hex(participant_phone)            AS participant_phone_hash,
        Digest::Md5Hex(participant_email)            AS participant_email_hash
    WITHOUT
       participant_name,
       participant_last_name,
       participant_company_name,
       participant_position,
       participant_phone,
       participant_email
    FROM $result AS r
);


INSERT INTO $dst_table_pii WITH TRUNCATE
SELECT * FROM $pii_version
ORDER BY `id`;


INSERT INTO $dst_table_hashed WITH TRUNCATE
SELECT * FROM $hashed_version
ORDER BY `id`;
