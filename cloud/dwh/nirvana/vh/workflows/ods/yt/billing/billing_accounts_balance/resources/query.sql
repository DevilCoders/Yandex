PRAGMA yt.UseNativeYtTypes;
PRAGMA Library("tables.sql");
PRAGMA Library("numbers.sql");
PRAGMA Library("datetime.sql");

IMPORT `tables` SYMBOLS $select_transfer_manager_table;
IMPORT `numbers` SYMBOLS $to_decimal_35_15;
IMPORT `datetime` SYMBOLS $get_datetime;

$cluster = {{ cluster->table_quote() }};

$src_folder = {{ param["source_folder_path"] -> quote() }};
$dst_table = {{ input1 -> table_quote() }};

$snapshot = SELECT * FROM $select_transfer_manager_table($src_folder, $cluster);


$result = (
  SELECT
    `id`                           AS billing_account_id,

     $to_decimal_35_15(`balance`)  AS balance,
     $to_decimal_35_15(`debt`)     AS debt,
     $to_decimal_35_15(`receipts`) AS receipts,

     $get_datetime(`balance_dt`)   AS balance_modified_at,
     $get_datetime(`debt_dt`)      AS debt_modified_at,
     $get_datetime(`receipts_dt`)  AS receipts_modified_at,
     $get_datetime(`modified_at`)  AS modified_at

  FROM $snapshot
);

INSERT INTO $dst_table WITH TRUNCATE
SELECT *
FROM $result
ORDER BY `billing_account_id`;
