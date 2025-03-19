USE hahn;

DEFINE SUBQUERY $crm_leads_history($lead_source) AS
    $leads = '//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_leads';
    $billingaccounts = '//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_billingaccounts';
    $leads_billing_accounts = '//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_leads_billing_accounts';
    $users = '//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_users';
    $email_addr_bean_rel = '//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_email_addr_bean_rel';
    $email_addresses = '//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_email_addresses';
    SELECT 
        l.id AS lead_id,
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
        b.name AS billing_account_id,
        u.user_name AS user_name,
        e.email_address AS email
    FROM 
        (
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
                date_modified
            FROM 
                $leads AS l
            WHERE
                l.deleted = 0
                AND ((l.lead_source = $lead_source) OR ($lead_source = 'All') )
                -- AND l.lead_source_description like '%30k%'   
        ) AS l
        LEFT JOIN $leads_billing_accounts AS l_ba on l.id = l_ba.leads_id
        LEFT JOIN $billingaccounts AS b on l_ba.billingaccounts_id = b.id
        LEFT JOIN $users AS u on l.assigned_user_id = u.id
        LEFT JOIN $email_addr_bean_rel AS e_b on l.id = e_b.bean_id
        LEFT JOIN $email_addresses AS e on e_b.email_address_id = e.id;
END DEFINE;


EXPORT $crm_leads_history;

