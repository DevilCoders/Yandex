USE hahn;
PRAGMA Library("tables.sql");
IMPORT tables SYMBOLS $last_non_empty_table;

$result_table = '{result_table}';


$accounts = '//home/cloud_analytics/dwh/raw/crm/accounts';
$billingaccounts = '//home/cloud_analytics/dwh/raw/crm/billingaccounts';

INSERT INTO $result_table WITH TRUNCATE 
SELECT
DISTINCT
    ba.name AS billing_account_id,
    acc.name AS account_name,
    acc.inn AS inn
FROM $last_non_empty_table($accounts) AS acc
LEFT JOIN $last_non_empty_table($billingaccounts) AS ba
    ON acc.id = ba.account_id
WHERE  
    acc.deleted = False 
    AND ba.deleted = False
    AND ba.name IS NOT NULL
    AND acc.inn IS NOT NULL
    AND ba.name != ''
    AND acc.inn != '';




