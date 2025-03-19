PRAGMA Library("datetime.sql");

IMPORT `datetime` SYMBOLS $from_utc_ts_to_msk_dt;

$src_table = {{ param["source_table_path"]->quote() }};
$dst_table = {{ input1->table_quote() }};

$toString = ($data) -> (CAST($data AS String));
$get_timestamp = ($ts) -> (CAST($ts AS TimeStamp));

$result = (
    SELECT
        $toString(`id`)                                           AS crm_billing_account_id,
        $toString(`account_id`)                                   AS crm_account_id,
        $toString(`name`)                                         AS billing_account_id,
        CAST(`deleted` AS bool)                                   AS deleted,
        $get_timestamp(`date_modified`)                           AS date_modified_ts,
        $from_utc_ts_to_msk_dt($get_timestamp(`date_modified`))   AS date_modified_dttm_local,
    FROM $src_table
);

/* Save result in ODS */
INSERT INTO $dst_table WITH TRUNCATE
SELECT * FROM $result
ORDER BY `crm_billing_account_id`;
