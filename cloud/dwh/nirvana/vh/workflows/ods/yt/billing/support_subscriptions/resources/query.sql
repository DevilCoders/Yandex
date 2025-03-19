PRAGMA yt.UseNativeYtTypes;
PRAGMA Library("tables.sql");
PRAGMA Library("datetime.sql");

IMPORT `tables` SYMBOLS $select_transfer_manager_table;
IMPORT `datetime` SYMBOLS $get_msk_datetime;

$cluster = {{cluster->table_quote()}};

$src_folder = {{ param["source_folder_path"]->quote() }};
$dst_table = {{ input1->table_quote() }};

$get_support_level_name = ($code) -> {
    RETURN CASE
    WHEN $code = 0 THEN 'Basic'
    WHEN $code = 1 THEN 'Standard'
    WHEN $code = 2 THEN 'Business'
    WHEN $code = 3 THEN 'Premium'
    ELSE 'Unknown' END
};

$snapshot = SELECT * FROM $select_transfer_manager_table($src_folder, $cluster);

$result = (
  SELECT
    billing_account_id                        AS billing_account_id,
    id                                        AS subscription_id,
    priority_level                            AS priority_level_code,
    $get_support_level_name(priority_level)   AS priority_level_name,
    $get_msk_datetime(start_time)             AS start_time_msk,
    $get_msk_datetime(end_time)               AS end_time_msk
FROM $snapshot
);

INSERT INTO $dst_table WITH TRUNCATE
SELECT * FROM $result
ORDER BY `billing_account_id`, `start_time_msk` DESC;
