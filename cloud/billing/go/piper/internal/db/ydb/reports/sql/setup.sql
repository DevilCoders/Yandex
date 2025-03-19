CREATE TABLE `test/reports/realtime/resource_reports`
(
    billing_account_id Utf8,
    `date` Utf8,
    cloud_id Utf8,
    sku_id Utf8,
    folder_id Utf8,
    resource_id Utf8,
    labels_hash Uint64,
    pricing_quantity Decimal(22,9),
    cost Decimal(22,9),
    credit Decimal(22,9),
    cud_credit Decimal(22,9),
    monetary_grant_credit Decimal(22,9),
    volume_incentive_credit Decimal(22,9),
    free_credit Decimal(22,9),
    updated_at Uint64,
    PRIMARY KEY (billing_account_id, `date`, cloud_id, folder_id, sku_id, resource_id, labels_hash)
);
