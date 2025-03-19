PRAGMA AnsiInForEmptyOrNullableItemsCollections;
PRAGMA Library('tables.sql');


IMPORT `tables` SYMBOLS $select_last_not_empty_table;

$cluster = {{cluster->table_quote()}};


$leads = {{param["leads"]->quote()}};
$billingaccounts = {{param["billingaccounts"]->quote()}};
$leads_billing_accounts = {{param["leads_billing_accounts"]->quote()}};
$users  = {{param["users"]->quote()}};
$opportunities = {{param["opportunities"]->quote()}};
$calls = {{param["calls"]->quote()}};
$accounts = {{param["accounts"]->quote()}};
$accounts_opportunities = {{param["accounts_opportunities"]->quote()}};

$destination_path = {{input1->table_quote()}};

$calls_data = (
        SELECT
                calls.id AS call_id,
                calls.name AS call_name,
                calls.date_start AS call_date_start,
                calls.date_end AS call_date_end,
                users.user_name AS call_user_name,
                calls.from_phone AS call_from_phone,
                calls.to_phone AS call_to_phone,
                calls.history_duration AS call_history_duration,
                calls.history_taked AS call_history_taked,
                calls.date_click_to_call AS call_date_click_to_call,
                calls.duration_hours AS call_duration_hours,
                calls.duration_minutes AS call_duration_minutes,
                calls.status AS call_status,
                calls.direction AS call_direction,
                calls.parent_id AS call_parent_id,
                calls.parent_type AS call_parent_type
        FROM $select_last_not_empty_table($calls, $cluster) AS calls
        LEFT JOIN $select_last_not_empty_table($users, $cluster) AS users
                ON users.id = calls.assigned_user_id
        WHERE  calls.deleted = False
);

$leads_data = (
        SELECT
                leads.id AS lead_id,
                leads.first_name AS lead_first_name,
                leads.last_name AS lead_last_name,
                leads.account_name AS lead_account_name,
                leads.lead_source AS lead_source,
                leads.lead_source_description AS lead_source_description,
                leads.status AS lead_status,
                billingaccounts.name AS lead_ba_id
        FROM $select_last_not_empty_table($leads, $cluster) AS leads
        LEFT JOIN $select_last_not_empty_table($leads_billing_accounts, $cluster) AS leads_billing_accounts
                ON leads.id = leads_billing_accounts.leads_id
        LEFT JOIN $select_last_not_empty_table($billingaccounts, $cluster) AS billingaccounts
                ON leads_billing_accounts.billingaccounts_id = billingaccounts.id
        WHERE  leads.deleted = False and leads_billing_accounts.deleted = False
);

$accounts_data = (
        SELECT
                accounts.id AS acc_id,
                accounts.name AS acc_name,
                accounts.segment AS acc_segment,
                billingaccounts.name AS acc_ba_id
        FROM $select_last_not_empty_table($accounts, $cluster) AS accounts
        LEFT JOIN $select_last_not_empty_table($billingaccounts, $cluster) AS billingaccounts
                ON billingaccounts.account_id = accounts.id
        WHERE  accounts.deleted = False
);

$oppty_data = (
        SELECT
                opportunities.id AS opp_id,
                opportunities.name AS opp_name,
                opportunities.amount_usdollar AS opp_likely,
                opportunities.probability_enum AS opp_probability,
                opportunities.sales_stage AS opp_sales_stage,
                opportunities.date_closed AS opp_expected_close_date,
                accounts.name AS opp_acc_name,
                billingaccounts.name AS opp_acc_ba_id
        FROM $select_last_not_empty_table($opportunities, $cluster) AS opportunities
        LEFT JOIN $select_last_not_empty_table($accounts_opportunities, $cluster) AS accounts_opportunities
                ON opportunities.id = accounts_opportunities.opportunity_id
        LEFT JOIN $select_last_not_empty_table($accounts, $cluster) AS accounts
                ON accounts.id = accounts_opportunities.account_id
        LEFT JOIN $select_last_not_empty_table($billingaccounts, $cluster) AS billingaccounts
                ON billingaccounts.account_id = accounts.id
        WHERE  opportunities.deleted = False
);


$result = (
        SELECT *
        FROM $calls_data AS calls_data
        LEFT JOIN $leads_data AS leads_data
                ON calls_data.call_parent_id = leads_data.lead_id
        LEFT JOIN $accounts_data AS accounts_data
                ON calls_data.call_parent_id = accounts_data.acc_id
        LEFT JOIN $oppty_data AS oppty_data
                ON calls_data.call_parent_id = oppty_data.opp_id
);



INSERT INTO $destination_path WITH TRUNCATE
SELECT
  *
FROM $result
ORDER BY `call_id`, `call_date_start`, `call_date_end`;
