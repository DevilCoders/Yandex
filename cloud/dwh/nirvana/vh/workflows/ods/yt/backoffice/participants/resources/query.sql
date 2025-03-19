PRAGMA Library("datetime.sql");

IMPORT `datetime` SYMBOLS $parse_iso8601_to_datetime_msk;


$src_table        = {{ param["source_folder_path"] -> quote() }};
$dst_table_pii    = {{ param["PII_destination_path"] -> quote() }};
$dst_table_hashed = {{ input1 -> table_quote() }};


$result = (
    SELECT
        id,
        uid                          AS puid,
        login,
        $parse_iso8601_to_datetime_msk(created_at) AS created_at_msk
    FROM $src_table
);
$pii_version = (
    SELECT *
    without
       created_at_msk
    FROM $result
);

$hashed_version = (
    SELECT
        r.*,
        Digest::Md5Hex(login) AS login_hash
    WITHOUT
         r.login
    FROM $result AS r
);


INSERT INTO $dst_table_pii WITH TRUNCATE
SELECT * FROM $pii_version
ORDER BY `id`;


INSERT INTO $dst_table_hashed WITH TRUNCATE
SELECT * FROM $hashed_version
ORDER BY `id`;
