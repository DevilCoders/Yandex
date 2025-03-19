$dm_yc_consumption = {{ param["dm_yc_consumption"]->quote() }};
$dst_table = {{input1->table_quote()}};

$result = (
    SELECT
        crm_segment,
        billing_record_msk_date,
        SUM(billing_record_total_rub_vat) AS billing_record_total_rub_vat
    FROM
        $dm_yc_consumption
    GROUP BY crm_segment,
        billing_record_msk_date
);

INSERT INTO $dst_table WITH TRUNCATE
SELECT * FROM $result;
