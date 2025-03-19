PRAGMA Library("datetime.sql");

IMPORT `datetime` SYMBOLS $parse_iso8601_to_datetime_msk;


$cluster = {{cluster->table_quote()}};
$src_table = {{ param["source_folder_path"] -> quote() }};
$dst_table_hashed = {{ input1 -> table_quote() }};
$dst_table_pii    = {{ param["PII_destination_path"] -> quote() }};


$result = (
    SELECT
        id,
        name_ru,
        name_en,
        description_ru,
        description_en,
        short_description_ru,
        short_description_en,
        $parse_iso8601_to_datetime_msk(`date`)   AS date_msk,
        $parse_iso8601_to_datetime_msk(created_at) AS created_at_msk,
        $parse_iso8601_to_datetime_msk(updated_at) AS updated_at_msk,
        image,
        record,
        `stream`,
        place_id,
        is_canceled,
        url,
        is_published,
        start_registration_time,
        registration_status,
        registration_form_hashed_id,
        is_online,
        webinar_url,
        custom_registration_form_fields,
        need_request_email,
        is_private,
        will_broadcast,
        contact_person,
        contact_person_phone
    FROM $src_table
);

$pii_version = (
    SELECT
        id,
        contact_person,
        contact_person_phone
    FROM $result
);

$hashed_version = (
    SELECT
        r.*,
        Digest::Md5Hex(r.contact_person)       AS contact_person_hash,
        Digest::Md5Hex(r.contact_person_phone) AS contact_person_phone_hash
    WITHOUT
         r.contact_person,
         r.contact_person_phone
    FROM $result as r
);

INSERT INTO $dst_table_pii WITH TRUNCATE
SELECT * FROM $pii_version
ORDER BY `id`;


INSERT INTO $dst_table_hashed WITH TRUNCATE
SELECT * FROM $hashed_version
ORDER BY `id`;
