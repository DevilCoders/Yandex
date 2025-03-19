USE hahn;
PRAGMA AnsiInForEmptyOrNullableItemsCollections;

$result_table = '//home/cloud_analytics/antifraud/dashboard/time_to_block';

$toJson = ($metadata) -> {
    RETURN CAST(Yson::ConvertToString($metadata) as Json)
};

$block_ba_hist = (
    SELECT  
        billing_account_id,
        CAST(updated_at AS Datetime) AS block_time,
        JSON_VALUE($toJson(metadata), '$.block_reason')  as block_reason
    FROM  `//home/cloud/billing/exported-billing-tables/billing_accounts_history_prod` 
);

$curr_block_state = (
    SELECT  
        id AS billing_account_id,
        CAST(created_at AS Datetime) AS ba_created_time,
        name,
        person_type,
        JSON_VALUE($toJson(metadata), '$.block_reason')  AS block_reason,
        JSON_VALUE($toJson(metadata), '$.unblock_reason')  AS unblock_reason,
        JSON_VALUE($toJson(metadata), '$.verified') ?? 'false' AS is_verified
    FROM  `//home/cloud/billing/exported-billing-tables/billing_accounts_prod` 
);

$first_ba_block = (
    SELECT billing_account_id,
        MIN(block_time) AS block_time
    FROM $block_ba_hist
    WHERE  block_reason IN ('manual', 'mining')
    GROUP BY billing_account_id
);

$still_blocked = (
    SELECT *
    FROM $curr_block_state
    WHERE is_verified != 'true'
        AND unblock_reason != 'manual'
);

$last_cons_hour = (
    SELECT
        MAX(CAST(billing_record_end_time AS Datetime)) AS last_con_hour,
        billing_account_id
    FROM `//home/cloud/billing/analytics_cube/realtime/prod`
    WHERE
        billing_record_cost > 0
        AND CAST(billing_record_end_time AS Datetime) >= CurrentUtcDatetime() - INTERVAL('P30D')
        AND sku_tag_lazy = 0
    GROUP BY
        billing_account_id
);

$ba_total_cons = (
    SELECT
        SUM(billing_record_cost) AS consumption,
        billing_account_id
    FROM `//home/cloud/billing/analytics_cube/realtime/prod`
    WHERE
        billing_record_cost > 0
        AND sku_tag_lazy = 0
    GROUP BY
        billing_account_id
);


INSERT INTO $result_table WITH TRUNCATE
SELECT 
    first_ba_block.billing_account_id as billing_account_id,
    ba_created_time,
    name,
    person_type,
    block_time,
    last_con_hour,
    block_time - ba_created_time as time_to_block,
    IF(last_con_hour IS NULL, NULL, last_con_hour - block_time) as time_to_rm_block,
    last_con_hour - ba_created_time as total_time_to_block,
    consumption
FROM  $first_ba_block AS first_ba_block
INNER JOIN $still_blocked AS still_blocked
    ON first_ba_block.billing_account_id = still_blocked.billing_account_id
LEFT JOIN  $last_cons_hour AS last_cons_hour
    ON first_ba_block.billing_account_id = last_cons_hour.billing_account_id
LEFT JOIN  $ba_total_cons AS ba_total_cons
    ON first_ba_block.billing_account_id = ba_total_cons.billing_account_id
    

