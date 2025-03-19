SELECT DISTINCT billing_account_id
FROM cloud_analytics.acquisition_cube
WHERE (is_var='var'
        OR segment='VAR'
        OR sales_name = 'victorbutenko'
        OR billing_account_id IN 
    (SELECT DISTINCT billing_account_id
    FROM cloud_analytics.crm_roles_daily
    WHERE user_name = 'victorbutenko'))
        AND billing_account_id NOT IN 
    (SELECT DISTINCT billing_account_id
    FROM cloud_analytics.acquisition_cube
    WHERE master_account_id != '')