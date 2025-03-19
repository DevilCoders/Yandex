USE hahn;

$res_table = '//home/cloud_analytics/import/crm/business_accounts/data';
$leads = '//home/cloud-dwh/data/prod/ods/crm/crm_leads';
$leads_pii = '//home/cloud-dwh/data/prod/ods/crm/PII/crm_leads';
$leads_billing_accounts = '//home/cloud-dwh/data/prod/ods/crm/crm_leads_billing_accounts';
$accounts = '//home/cloud-dwh/data/prod/ods/crm/crm_accounts';
$accounts_pii = '//home/cloud-dwh/data/prod/ods/crm/PII/crm_accounts';
$billingaccounts = '//home/cloud-dwh/data/prod/ods/crm/billing_accounts';
$dimensions_bean_rel = '//home/cloud-dwh/data/prod/ods/crm/dimension_bean_relations';
$dimensions = '//home/cloud-dwh/data/prod/ods/crm/crm_dimensions';
$email_addr_bean_rel = '//home/cloud-dwh/data/prod/ods/crm/crm_email_address_bean_relations';
$email_addresses = '//home/cloud-dwh/data/prod/ods/crm/PII/crm_email_addresses';

$account_business = (
    SELECT
            b.billing_account_id AS billing_account_id,
            e.email_address AS email,
            acc_pii.phone_office AS phone
    FROM $accounts AS acc
        LEFT JOIN $accounts_pii AS acc_pii ON acc.crm_account_id = acc_pii.crm_account_id
        LEFT JOIN $billingaccounts AS b ON acc.crm_account_id = b.crm_account_id
        LEFT JOIN $dimensions_bean_rel AS d_b ON acc.crm_account_id = d_b.crm_bean_id
        LEFT JOIN $dimensions AS d ON d_b.crm_dimension_id = d.crm_dimension_id
        LEFT JOIN $email_addr_bean_rel AS e_b ON acc.crm_account_id = e_b.bean_id
        LEFT JOIN $email_addresses AS e ON e_b.crm_email_id = e.crm_email_id
    WHERE acc.deleted = False
        AND d.crm_dimension_name == 'Company (Telesales)'
        AND b.billing_account_id != ''
        AND b.billing_account_id IS NOT NULL
);

$leads_business = (
    SELECT
        l_pii.phone_mobile AS phone,
        b.billing_account_id AS billing_account_id,
        e.email_address AS email
    FROM $leads AS l
        LEFT JOIN $leads_pii AS l_pii ON l.crm_lead_id = l_pii.crm_lead_id
        LEFT JOIN $leads_billing_accounts AS l_ba ON l.crm_lead_id = l_ba.crm_leads_id
        LEFT JOIN $billingaccounts AS b ON l_ba.crm_billing_accounts_id = b.crm_billing_account_id
        LEFT JOIN $dimensions_bean_rel AS d_b ON l.crm_account_id = d_b.crm_bean_id
        LEFT JOIN $dimensions AS d ON d_b.crm_dimension_id = d.crm_dimension_id
        LEFT JOIN $email_addr_bean_rel AS e_b ON l.crm_account_id = e_b.bean_id
        LEFT JOIN $email_addresses AS e ON e_b.crm_email_id = e.crm_email_id
    WHERE l.deleted = False
        AND d.crm_dimension_name == 'Company (Telesales)'
        AND b.billing_account_id != ''
        AND b.billing_account_id IS NOT NULL
);

INSERT INTO $res_table WITH TRUNCATE
    SELECT
        billing_account_id,
        SOME(raw_phone) AS phone,
        SOME(raw_email) AS email,
        AGGREGATE_LIST(raw_phone) AS all_phones,
        AGGREGATE_LIST(raw_email) AS all_email
    FROM (
        SELECT
            billing_account_id,
            phone AS raw_phone,
            email AS raw_email,
        FROM $account_business

        UNION ALL

        SELECT
            billing_account_id,
            phone,
            email,
        FROM $leads_business
    )
    WHERE billing_account_id != ''
        AND billing_account_id IS NOT NULL
    GROUP BY billing_account_id
;
