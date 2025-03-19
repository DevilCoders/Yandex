PRAGMA AnsiInForEmptyOrNullableItemsCollections;
PRAGMA Library('tables.sql');


IMPORT `tables` SYMBOLS $select_last_not_empty_table;

$cluster = {{cluster->table_quote()}};


$leads = {{param["leads"]->quote()}};
$billingaccounts = {{param["billingaccounts"]->quote()}};
$leads_billing_accounts = {{param["leads_billing_accounts"]->quote()}};
$users  = {{param["users"]->quote()}};
$dimensions_bean_rel = {{param["dimensions_bean_rel"]->quote()}};
$dimensions = {{param["dimensions"]->quote()}};
$dimensionscategories= {{param["dimensionscategories"]->quote()}};
$email_addr_bean_rel= {{param["email_addr_bean_rel"]->quote()}};
$email_addresses = {{param["email_addresses"]->quote()}};
$tag_bean_rel = {{param["tag_bean_rel"]->quote()}};
$tags = {{param["tags"]->quote()}};
$opportunities = {{param["opportunities"]->quote()}};


$destination_path = {{input1->table_quote()}};


$current_leads = (
  SELECT
      id,
      first_name,
      last_name,
      account_name,
      title,
      assigned_user_id,
      phone_mobile,
      description,
      timezone,
      status,
      lead_source,
      lead_source_description,
      lead_priority,
      date_entered,
      date_modified,
      tracker_number,
      send_to_isv_team,
      promocode,
      promocode_sum,
      website,
      opportunity_id
  FROM $select_last_not_empty_table($leads, $cluster) AS leads_all
  WHERE leads_all.deleted = False
);

$result = (
    SELECT
          leads.id AS lead_id,
          leads.first_name AS first_name,
          leads.last_name AS last_name,
          leads.account_name AS account_name,
          leads.title AS title,
          leads.phone_mobile AS phone_mobile,
          leads.description AS description,
          leads.timezone AS timezone,
          leads.status AS status,
          leads.lead_source AS lead_source,
          leads.lead_source_description AS lead_source_description,
          IF(leads.lead_source like '%mkt%', leads.lead_source, 'other') AS lead_source_marketing,
          leads.lead_priority AS lead_priority,
          leads.date_entered AS date_entered,
          leads.date_modified AS date_modified,
          leads.tracker_number AS tracker_number,
          leads.send_to_isv_team AS send_to_isv_team,
          leads.promocode AS promocode,
          leads.promocode_sum AS promocode_sum,
          billingaccounts.name AS billing_account_id,
          users.user_name AS user_name,
          dimensions.name AS dimension_name,
          dimensionscategories.name AS dimension_ctg_name,
          tags.name AS tag_name,
          email_addresses.email_address AS email,
          leads.website AS website,
          oppty.id AS oppty_id,
          oppty.name AS oppty_name,
          oppty.date_closed AS oppty_date_closed,
          oppty.amount AS oppty_amount,
          oppty.sales_stage AS oppty_sales_stage,
          oppty.probability AS oppty_probability
      FROM $current_leads AS leads
      LEFT JOIN $select_last_not_empty_table($leads_billing_accounts, $cluster) AS leads_billing_accounts
              ON leads.id = leads_billing_accounts.leads_id
      LEFT JOIN $select_last_not_empty_table($billingaccounts, $cluster) AS billingaccounts
              ON leads_billing_accounts.billingaccounts_id = billingaccounts.id
      LEFT JOIN $select_last_not_empty_table($users, $cluster) AS users
              ON leads.assigned_user_id = users.id
      LEFT JOIN $select_last_not_empty_table($dimensions_bean_rel, $cluster) AS dimensions_bean_rel
              ON leads.id = dimensions_bean_rel.bean_id
      LEFT JOIN $select_last_not_empty_table($dimensions, $cluster) AS dimensions
              ON dimensions_bean_rel.dimension_id = dimensions.id
      LEFT JOIN $select_last_not_empty_table($dimensionscategories, $cluster) AS dimensionscategories
              ON dimensionscategories.id = dimensions.category_id
      LEFT JOIN $select_last_not_empty_table($email_addr_bean_rel, $cluster) AS email_addr_bean_rel
              ON leads.id = email_addr_bean_rel.bean_id
      LEFT JOIN $select_last_not_empty_table($email_addresses, $cluster) AS email_addresses
              ON email_addr_bean_rel.email_address_id = email_addresses.id
      LEFT JOIN (SELECT * FROM $select_last_not_empty_table($tag_bean_rel, $cluster) WHERE deleted = False) AS tag_bean_rel
              ON leads.id = tag_bean_rel.bean_id
      LEFT JOIN (SELECT * FROM $select_last_not_empty_table($tags, $cluster) WHERE deleted = False) AS tags
              ON tag_bean_rel.tag_id = tags.id
      LEFT JOIN (SELECT * FROM $select_last_not_empty_table($opportunities, $cluster) WHERE deleted = False) AS oppty
              ON leads.opportunity_id = oppty.id
);

INSERT INTO $destination_path WITH TRUNCATE
SELECT
  *
FROM $result
ORDER BY `billing_account_id`, `date_modified`;
