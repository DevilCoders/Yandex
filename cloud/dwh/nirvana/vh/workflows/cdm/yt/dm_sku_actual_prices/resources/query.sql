-- to preserve columns order
PRAGMA OrderedColumns;

$now = CurrentUtcDateTime();

$destination_path = {{ input1 -> table_quote() }};
$publisher_accounts = {{ param["publisher_accounts"] -> quote() }};
$sku_pricing_versions = {{ param["sku_pricing_versions"] -> quote() }};
$services = {{ param["services"] -> quote() }};
$skus = {{ param["skus"] -> quote() }};
$to_double = ($value) -> (CAST($value as Double));


$sku_current_pricing_version = (
    SELECT
        sku_id,
        start_time,
        end_time,
        rate_id,
        start_pricing_quantity,
        end_pricing_quantity,
        pricing_unit,
        unit_price
    FROM $sku_pricing_versions
    WHERE start_time <= $now AND $now <= end_time
);

$dm_sku_actual_pricing = (
    SELECT
        skus.sku_id                                             AS sku_id,
        skus.balance_product_id                                 AS balance_product_id,
        skus.publisher_account_id                               AS publisher_account_id,
        publisher_accounts.name                                 AS publisher_account_name,
        skus.service_id                                         AS service_id,
        services.name                                           AS service_name,
        services.description                                    AS service_descripiton,
        skus.translation_en                                     AS translation_en,
        skus.translation_ru                                     AS translation_ru,
        sku_pricing.start_time                                  AS start_time,
        sku_pricing.end_time                                    AS end_time,
        sku_pricing.rate_id                                     AS rate_id,
        $to_double(sku_pricing.start_pricing_quantity)          AS start_pricing_quantity,
        $to_double(sku_pricing.end_pricing_quantity)            AS end_pricing_quantity,
        sku_pricing.pricing_unit                                AS pricing_unit,
        $to_double(sku_pricing.unit_price)                      AS unit_price
        FROM (SELECT * FROM $skus WHERE deprecated == false) AS skus
        LEFT JOIN $publisher_accounts AS publisher_accounts
          ON skus.publisher_account_id = publisher_accounts.publisher_account_id
        LEFT JOIN $services AS services
          ON skus.service_id = services.service_id
        LEFT JOIN $sku_current_pricing_version AS sku_pricing
          ON skus.sku_id = sku_pricing.sku_id
);

INSERT INTO $destination_path WITH TRUNCATE
SELECT
*
FROM $dm_sku_actual_pricing
ORDER BY
    service_id,
    sku_id,
    rate_id,
    start_time
