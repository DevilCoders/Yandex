$dm_yc_consumption = {{ param["dm_yc_consumption"]->quote() }};
$dst_table = {{input1->table_quote()}};

$result = (
    SELECT
        sku_service_name,
        billing_record_msk_date,
        count(distinct billing_account_id) AS unique_billing_account_id
    FROM
        $dm_yc_consumption
    GROUP BY sku_service_name,
        billing_record_msk_date
);

INSERT INTO $dst_table WITH TRUNCATE
SELECT * FROM $result;
