use hahn;
PRAGMA yt.Pool = 'cloud_analytics_pool';
PRAGMA yt.TmpFolder = '//home/cloud_analytics/ml/tmp';
$script = @@#py
from yql.typing import *
import re


def foo(x:String) -> String:

    phone = str(x)
    return ''.join(c for c in phone if c.isdigit())
@@;

$udf = Python3::foo($script);

$tmp = (
SELECT
    `crm_leads_id` as `crm_lead_id`,
    `billing_account_id`,
FROM hahn.`home/cloud-dwh/data/prod/ods/crm/crm_leads_billing_accounts` as x
JOIN hahn.`home/cloud-dwh/data/prod/ods/crm/billing_accounts` as y
ON x.`crm_billing_accounts_id` == y.`crm_billing_account_id`
);

$result = (
SELECT
    `crm_account_name` as `Account_name`,
    `crm_user_name` as `Assigned_to`,
    '[\"' || `billing_account_id` || '\"]' as `Billing_account_id`,
    "cold calls" as `Lead_Source`,
    "DNA_Restored from recycled" as `Lead_Source_Description`,
    "" as `Timezone`,
    '[\"' || `lead_source` || '\",\"Срок реализации проекта более 1 месяца\"]' as `Tags`,
    $udf(COALESCE(CAST(`phone_mobile` as String), "")) as `Phone_1`,
    -- as `Email`,
    `first_name` as `First_name`, 
    `last_name` as `Last_name`,
    CAST(CurrentUtcDatetime() as Int64) as `Timestamp`
FROM `//home/cloud-dwh/data/prod/ods/crm/PII/crm_leads` as x
JOIN `//home/cloud-dwh/data/prod/ods/crm/crm_leads` as y
ON x.`crm_lead_id` == y.`crm_lead_id`
LEFT JOIN $tmp as z
ON x.`crm_lead_id` == z.`crm_lead_id`
LEFT JOIN (SELECT `crm_user_id`, `crm_user_name` FROM `//home/cloud-dwh/data/prod/ods/crm/crm_users`) as t
ON y.`assigned_user_id` == t.`crm_user_id`
WHERE `disqualified_reason` == 'prj_implementation_period_more_1m'
AND DateTime::ToDays(CurrentUtcTimestamp() - `date_modified_ts`) > 30
);


INSERT INTO `{output}`
SELECT *
FROM $result as x
LEFT ONLY JOIN (
    SELECT `Billing_account_id`, `Phone_1`
    FROM `{output}`
    WHERE `Tags` LIKE "%Срок реализации проекта более 1 месяца%"
) as y
ON x.`Phone_1` == y.`Phone_1`