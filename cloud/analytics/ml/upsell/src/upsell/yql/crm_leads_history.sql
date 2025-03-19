USE hahn;

$result_table = '//home/cloud_analytics/import/crm/leads/upsell';
$leads_wo_pii = '//home/cloud-dwh/data/prod/ods/crm/crm_leads';
$leads_pii = '//home/cloud-dwh/data/prod/ods/crm/PII/crm_leads';
$leads_billing_accounts = '//home/cloud-dwh/data/prod/ods/crm/crm_leads_billing_accounts';
$billingaccounts = '//home/cloud-dwh/data/prod/ods/crm/billing_accounts';
$users = '//home/cloud-dwh/data/prod/ods/crm/crm_users';
$email_addr_bean_rel = '//home/cloud-dwh/data/prod/ods/crm/crm_email_address_bean_relations';
$email_addresses = '//home/cloud-dwh/data/prod/ods/crm/PII/crm_email_addresses';

DEFINE SUBQUERY $crm_leads_history($lead_source) AS
    SELECT
        l.lead_id AS lead_id,
        l.first_name AS first_name,
        l.last_name AS last_name,
        l.account_name AS client_name,
        l.title AS title,
        l.phone_mobile AS phone,
        l.description AS description,
        l.timezone AS timezone,
        l.status AS status,
        l.lead_source_description AS lead_source,
        l.lead_priority AS lead_priority,
        l.date_entered AS date_entered,
        l.date_modified AS date_modified,
        b.billing_account_id AS billing_account_id,
        u.crm_user_name AS user_name,
        e.email_address AS email
    FROM
        (
            SELECT
                lw.crm_lead_id AS lead_id,
                first_name,
                last_name,
                crm_account_name AS account_name,
                title,
                assigned_user_id,
                phone_mobile,
                crm_lead_description AS description,
                timezone,
                crm_lead_status AS status,
                lead_source,
                lead_source_description,
                lead_priority,
                date_entered_ts AS date_entered,
                date_modified_ts AS date_modified
            FROM $leads_wo_pii AS lw
            LEFT JOIN $leads_pii AS lp ON lw.crm_lead_id = lp.crm_lead_id
            WHERE lw.deleted = False
                AND ((lw.lead_source = $lead_source) OR ($lead_source = 'All') )
        ) AS l
        LEFT JOIN $leads_billing_accounts AS l_ba on l.lead_id = l_ba.crm_leads_id
        LEFT JOIN $billingaccounts AS b on l_ba.crm_billing_accounts_id = b.crm_billing_account_id
        LEFT JOIN $users AS u on l.assigned_user_id = u.crm_user_id
        LEFT JOIN $email_addr_bean_rel AS e_b on l.lead_id = e_b.bean_id
        LEFT JOIN $email_addresses AS e on e_b.crm_email_id = e.crm_email_id;
END DEFINE
;

INSERT INTO $result_table WITH TRUNCATE
    SELECT *
    FROM $crm_leads_history('upsell')
;
