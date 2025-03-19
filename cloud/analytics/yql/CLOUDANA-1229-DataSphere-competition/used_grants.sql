use hahn;

$time_from_seconds = ($double) -> {RETURN DateTime::FromMilliseconds(CAST($double*1000 AS Uint64))};    

DEFINE subquery
$used_grants() AS (
    SELECT 
        used.billing_account_id AS billing_account_id,
        max($time_from_seconds(used.start_time)) AS start_time
    FROM `//home/cloud/billing/exported-billing-tables/monetary_grants_prod` AS used
    LEFT JOIN `//home/cloud/billing/exported-billing-tables/monetary_grant_offers_prod` AS grants ON used.source_id=grants.id
    WHERE (source = 'default'
        OR Yson::ConvertToString(grants.proposed_meta) LIKE "%CLOUDGRANTS-1098%")
    GROUP BY used.billing_account_id
    );
END DEFINE;
                    
EXPORT $used_grants;