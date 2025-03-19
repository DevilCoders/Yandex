PRAGMA yt.UseNativeYtTypes;
PRAGMA Library("tables.sql");

IMPORT `tables` SYMBOLS $select_transfer_manager_table;

$cluster = {{ cluster -> table_quote() }};

$src_folder = {{ param["source_folder_path"] -> quote() }};
$dst_table = {{ input1 -> table_quote() }};

$snapshot = SELECT * FROM $select_transfer_manager_table($src_folder, $cluster);

$parsed = (
    SELECT
        billing_account_id,
        Yson::YPathString(person_data, '/' || type || '/name')       AS name,
        Yson::YPathString(person_data, '/' || type || '/first_name') AS first_name,
        Yson::YPathString(person_data, '/' || type || '/last_name')  AS last_name,
        person_data,
        type,
    FROM (SELECT s.*, UNWRAP(Yson::LookupString(person_data, 'type')) AS type FROM $snapshot AS s)
);

$result = (SELECT
    billing_account_id,
    coalesce(String::ReplaceAll(name, '\"', "\'"), first_name || ' ' || last_name) AS name,
    CASE
        WHEN String::EndsWithIgnoreCase(type, 'company') THEN 'company'
        WHEN String::EndsWithIgnoreCase(type, 'individual') THEN 'individual'
        ELSE 'other'
    END                                                                            AS type,
    person_data                                                                    AS person_data,
    type                                                                           AS original_type
FROM $parsed
);

INSERT INTO $dst_table WITH TRUNCATE
SELECT * FROM $result
ORDER BY billing_account_id
