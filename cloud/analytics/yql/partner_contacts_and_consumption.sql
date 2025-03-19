USE hahn;
DEFINE ACTION $partner_datamart_script() AS
    $output_table_path = '//home/cloud_analytics/data_swamp/projects/partner_program/partner_info';
    $current_dt = CurrentUtcDate();
    $quarter_begin = ($date) -> {
        RETURN DateTime::MakeDate(DateTime::StartOfQuarter($date))
    };
    $quarter_end = ($date) -> {
        RETURN DateTime::MakeDate(DateTime::ShiftQuarters(DateTime::StartOfQuarter($date), 1)) - DateTime::IntervalFromDays(1)
    };
    --календарь с периодами начала и конца кварталов
    $calendar =
        SELECT
            'current_quarter' AS name_of_period,
            $quarter_begin($current_dt) AS quarter_begin,
            $quarter_end($current_dt) AS quarter_end UNION ALL SELECT
            'current_quarter-1' AS name_of_period,
            $quarter_begin(DateTime::ShiftQuarters($current_dt, - 1)) AS quarter_begin,
            $quarter_end(DateTime::ShiftQuarters($current_dt, - 1)) AS quarter_end UNION ALL SELECT
            'current_quarter-2' AS name_of_period,
            $quarter_begin(DateTime::ShiftQuarters($current_dt, - 2)) AS quarter_begin,
            $quarter_end(DateTime::ShiftQuarters($current_dt, - 2)) AS quarter_end UNION ALL SELECT
            'current_quarter-3' AS name_of_period,
            $quarter_begin(DateTime::ShiftQuarters($current_dt, - 3)) AS quarter_begin,
            $quarter_end(DateTime::ShiftQuarters($current_dt, - 3)) AS quarter_end UNION ALL SELECT
            'current_quarter-4' AS name_of_period,
            $quarter_begin(DateTime::ShiftQuarters($current_dt, - 4)) AS quarter_begin,
            $quarter_end(DateTime::ShiftQuarters($current_dt, - 4)) AS quarter_end;
    --собираем детальное потребление за требуемые периоды
    $detail_consumption =
        SELECT
            base.billing_account_id AS billing_account_id,
            base.converted_date AS date_qualified,
            base.master_account_name AS master_account_name,
            base.partner_manager AS partner_manager,
            base.master_account_id AS master_account_id,
            base.is_partner AS is_partner,
            base.paid_vat - base.var_reward_vat AS consumption,
            clnd.name_of_period AS name_of_period
            FROM `//home/cloud_analytics/dashboards/partner_cdm`
                AS base
            CROSS JOIN $calendar
                AS clnd
            WHERE base.event_time BETWEEN clnd.quarter_begin AND clnd.quarter_end;
    --агрегируем потребление
    $agg_consumption =
        SELECT
            master_account_id,
            date_qualified,
            master_account_name,
            partner_manager,
            sum(consumption) AS summary_consumption,
            count(DISTINCT billing_account_id) AS count_billing_account_id,
            sum(CASE WHEN is_partner == 1 AND name_of_period == 'current_quarter' THEN consumption ELSE 0 END) AS billing_consumption_curr_quater,
            sum(CASE WHEN is_partner == 1 AND name_of_period == 'current_quarter-1' THEN consumption ELSE 0 END) AS billing_consumption_curr_quater_1,
            sum(CASE WHEN is_partner == 1 AND name_of_period == 'current_quarter-2' THEN consumption ELSE 0 END) AS billing_consumption_curr_quater_2,
            sum(CASE WHEN is_partner == 1 AND name_of_period == 'current_quarter-3' THEN consumption ELSE 0 END) AS billing_consumption_curr_quater_3,
            sum(CASE WHEN is_partner == 1 AND name_of_period == 'current_quarter-4' THEN consumption ELSE 0 END) AS billing_consumption_curr_quater_4,
            sum(CASE WHEN is_partner == 0 AND name_of_period == 'current_quarter' THEN consumption ELSE 0 END) AS subacc_billing_consumption_curr_quater,
            sum(CASE WHEN is_partner == 0 AND name_of_period == 'current_quarter-1' THEN consumption ELSE 0 END) AS subacc_billing_consumption_curr_quater_1,
            sum(CASE WHEN is_partner == 0 AND name_of_period == 'current_quarter-2' THEN consumption ELSE 0 END) AS subacc_billing_consumption_curr_quater_2,
            sum(CASE WHEN is_partner == 0 AND name_of_period == 'current_quarter-3' THEN consumption ELSE 0 END) AS subacc_billing_consumption_curr_quater_3,
            sum(CASE WHEN is_partner == 0 AND name_of_period == 'current_quarter-4' THEN consumption ELSE 0 END) AS subacc_billing_consumption_curr_quater_4,
            count(DISTINCT CASE WHEN name_of_period == 'current_quarter' THEN billing_account_id ELSE master_account_id END) - 1 AS subaccount_num_curr_quater,
            count(DISTINCT CASE WHEN name_of_period == 'current_quarter-1' THEN billing_account_id ELSE master_account_id END) - 1 AS subaccount_num_curr_quater_1,
            count(DISTINCT CASE WHEN name_of_period == 'current_quarter-2' THEN billing_account_id ELSE master_account_id END) - 1 AS subaccount_num_curr_quater_2,
            count(DISTINCT CASE WHEN name_of_period == 'current_quarter-3' THEN billing_account_id ELSE master_account_id END) - 1 AS subaccount_num_curr_quater_3,
            count(DISTINCT CASE WHEN name_of_period == 'current_quarter-4' THEN billing_account_id ELSE master_account_id END) - 1 AS subaccount_num_curr_quater_4,
            FROM $detail_consumption
            WHERE master_account_id IS NOT NULL
            GROUP BY
                master_account_id,
                date_qualified,
                master_account_name,
                partner_manager
                ;
    --собираем базу мастер-аккуантов для дальнейшей фильтрации
    $base_master_account_id =
        SELECT
            master_account_id
            FROM $agg_consumption;
    --связка аккаунт биллинг-аккаунт CRM
    $billing_to_crm_account =
        SELECT
            billing_account_id,
            crm_account_id
            FROM `//home/cloud-dwh/data/prod/ods/crm/billing_accounts`
            WHERE NOT deleted AND billing_account_id IN $base_master_account_id;
    $desc =
        SELECT
            base.billing_account_id AS billing_account_id,
            descr.crm_account_description AS description
            FROM $billing_to_crm_account
                AS base
            LEFT JOIN `//home/cloud-dwh/data/prod/ods/crm/PII/crm_accounts`
                AS descr
            USING (crm_account_id);
    --связка аккаунт CRM-контакт CRM
    $crm_account_to_contact =
        SELECT
            crm_account_id,
            crm_contact_id,
            primary_account,
            date_modified_ts
            FROM `//home/cloud-dwh/data/prod/ods/crm/crm_accounts_contacts`
            WHERE NOT deleted AND crm_account_id IN (
                SELECT
                    crm_account_id
                    FROM $billing_to_crm_account
            );
    --лиды CRM
    $crm_lead =
        SELECT
            crm_contact_id,
            crm_lead_description,
            crm_lead_id,
            crm_lead_status,
            inn,
            lead_source,
            converted,
            website,
            date_entered_dttm_local,
            lead_source_description
            FROM `//home/cloud-dwh/data/prod/ods/crm/crm_leads`
            WHERE NOT deleted AND crm_contact_id IS NOT NULL AND crm_contact_id IN (
                SELECT
                    crm_contact_id
                    FROM $crm_account_to_contact
            );
    --контакты с лидов
    $lead_contacts =
        SELECT
            crm_account_name,
            crm_lead_id,
            first_name,
            last_name,
            phone_mobile,
            title
            FROM `//home/cloud-dwh/data/prod/ods/crm/PII/crm_leads`
            WHERE crm_lead_id IN (
                SELECT
                    crm_lead_id
                    FROM $crm_lead
            );
    --контакты с CRM 
    $crm_lead_contact_info =
        SELECT
            base.billing_account_id AS billing_account_id,
            crm_lead.crm_contact_id AS crm_contact_id,
            crm_lead.crm_lead_description AS crm_lead_description,
            crm_lead.crm_lead_status AS crm_lead_status,
            crm_lead.inn AS inn,
            crm_lead.lead_source AS lead_source,
            crm_lead.converted AS converted,
            crm_lead.website AS website,
            lead_cont.crm_account_name AS crm_account_name,
            lead_cont.first_name AS first_name,
            lead_cont.last_name AS last_name,
            lead_cont.phone_mobile AS phone_mobile,
            lead_cont.title AS title,
            crm_lead.date_entered_dttm_local AS date_entered_dttm_local,
            ROW_NUMBER() OVER w AS row_num
            FROM $billing_to_crm_account
                AS base
            LEFT JOIN $crm_account_to_contact
                AS a2c
            ON a2c.crm_account_id == base.crm_account_id
            LEFT JOIN $crm_lead
                AS crm_lead
            ON crm_lead.crm_contact_id == a2c.crm_contact_id
            LEFT JOIN $lead_contacts
                AS lead_cont
            ON crm_lead.crm_lead_id == lead_cont.crm_lead_id
            WINDOW
                w AS (
                    PARTITION BY
                        base.billing_account_id
                    ORDER BY
                        CASE WHEN crm_lead.lead_source == 'var' THEN 0 ELSE 1 END,
                        crm_lead.date_entered_dttm_local DESC
                );
    --самая приоритетная запись контактов
    $crm_lead_contact_info =
        SELECT
            *
            FROM $crm_lead_contact_info
            WHERE row_num == 1;
    --
    $crm_contact =
        SELECT
            crm_contact_id,
            first_name,
            last_name,
            phone_mobile,
            phone_work,
            telegram_account
            FROM `//home/cloud-dwh/data/prod/ods/crm/PII/crm_contacts`
            WHERE crm_contact_id IN (
                SELECT
                    crm_contact_id
                    FROM $crm_account_to_contact
            );
    --
    $crm_contact_with_billing_acc =
        SELECT
            base.billing_account_id AS billing_account_id,
            contact.first_name AS first_name,
            contact.last_name AS last_name,
            coalesce(contact.phone_mobile, contact.phone_work) AS phone_mobile,
            a2c.primary_account AS primary_account,
            a2c.date_modified_ts AS date_modified_ts
            FROM $billing_to_crm_account
                AS base
            LEFT JOIN $crm_account_to_contact
                AS a2c
            ON base.crm_account_id == a2c.crm_account_id
            LEFT JOIN $crm_contact
                AS contact
            ON a2c.crm_contact_id == contact.crm_contact_id;
    --
    $crm_contact_agg =
        SELECT
            billing_account_id,
            first_name,
            last_name,
            phone_mobile,
            ROW_NUMBER() OVER w AS row_num
            FROM $crm_contact_with_billing_acc
            WINDOW
                w AS (
                    PARTITION BY
                        billing_account_id
                    ORDER BY
                        primary_account DESC,
                        date_modified_ts DESC
                );
    --
    $crm_contact_agg =
        SELECT
            *
            FROM $crm_contact_agg
            WHERE row_num == 1;
    --
    $union_of_crm_contacts =
        SELECT
            coalesce(lead_info.billing_account_id, contact_info.billing_account_id) AS billing_account_id,
            lead_info.converted AS lead_converted,
            lead_info.crm_account_name AS lead_crm_account_name,
            lead_info.crm_contact_id AS lead_crm_contact_id,
            lead_info.crm_lead_description AS lead_crm_lead_description,
            lead_info.crm_lead_status AS lead_crm_lead_status,
            lead_info.first_name AS lead_first_name,
            lead_info.inn AS lead_inn,
            lead_info.last_name AS lead_last_name,
            lead_info.lead_source AS lead_lead_source,
            lead_info.phone_mobile AS lead_phone_mobile,
            lead_info.row_num AS lead_row_num,
            lead_info.title AS lead_title,
            lead_info.website AS lead_website,
            contact_info.first_name AS first_name,
            contact_info.last_name AS last_name,
            contact_info.phone_mobile AS phone_mobile,
            contact_info.row_num AS row_num
            FROM $crm_lead_contact_info
                AS lead_info
            FULL JOIN $crm_contact_agg
                AS contact_info
            USING (billing_account_id);
    --
    $first_last_names =
        SELECT
            billing_account_id,
            first_name,
            last_name,
            ROW_NUMBER() OVER w AS row_num
            FROM $crm_lead_contact_info
            WINDOW
                w AS (
                    PARTITION BY
                        billing_account_id
                    ORDER BY
                        CASE WHEN first_name IS NOT NULL THEN 1 ELSE 0 END DESC,
                        CASE WHEN last_name IS NOT NULL THEN 1 ELSE 0 END DESC,
                        date_entered_dttm_local DESC
                );
    $first_last_names =
        SELECT
            *
            FROM $first_last_names
            WHERE row_num == 1;
    --контакты бэкфиса/биллинга
    $billing_contacts =
        SELECT
            billing_account_id,
            Yson::ConvertToString(person_data.company.email) AS backoffice_email,
            person_data.company.inn AS backoffice_inn,
            Yson::ConvertToString(person_data.company.name) AS backoffice_name,
            Yson::ConvertToString(person_data.company.phone) AS backoffice_phone
            FROM `//home/cloud-dwh/data/prod/ods/billing/person_data/person_data`;
    --роли
    $roles =
        SELECT
            billing_account_id,
            account_owner_current,
            architect_current,
            bus_dev_current,
            partner_manager_current,
            sales_current
            FROM `//home/cloud-dwh/data/prod/cdm/dm_ba_crm_tags`
            WHERE date = CAST($current_dt AS String);
    --вычисляем creation_dt
    $creation_date =
        SELECT
            billing_account_id,
            min(event_dt) AS creation_dt,
            FROM `//home/cloud-dwh/data/prod/cdm/dm_ba_history`
            WHERE `action` == 'create' AND billing_account_id IN $base_master_account_id
            GROUP BY
                billing_account_id;
    --вычисляем количество субаккаунтов (всех и активных)
    $account_subaccount_and_status =
        SELECT
            billing_account_id,
            master_account_id,
            CASE WHEN state NOT IN ('deleted', 'suspended', 'inactive') THEN TRUE ELSE FALSE END AS is_active
            FROM `//home/cloud-dwh/data/prod/ods/billing/billing_accounts`
            WHERE master_account_id IS NOT NULL AND master_account_id IN $base_master_account_id;
    --агругируем
    $account_subaccount_and_status_agg =
        SELECT
            master_account_id,
            count(DISTINCT billing_account_id) - max(CASE WHEN master_account_id == billing_account_id THEN 1 ELSE 0 END) AS all_sub_accounts_number,
            count(DISTINCT CASE WHEN is_active THEN billing_account_id ELSE master_account_id END) - max(CASE WHEN master_account_id == billing_account_id THEN 1 ELSE 0 END) AS all_active_sub_accounts_number
            FROM $account_subaccount_and_status
            GROUP BY
                master_account_id;
    --вычисляем date_entered
    $entered_date =
        SELECT
            crm_account_id,
            max(date_entered_dttm_local) AS date_entered_dttm_local
            FROM `//home/cloud-dwh/data/prod/ods/crm/crm_leads`
            WHERE lead_source == 'var'
            GROUP BY
                crm_account_id;
    --вычисляем date_entered
    $entered_date =
        SELECT
            base.billing_account_id AS billing_account_id,
            CAST(en_dt.date_entered_dttm_local AS date) AS date_entered
            FROM $billing_to_crm_account
                AS base
            LEFT JOIN $entered_date
                AS en_dt
            USING (crm_account_id);
    --staff 
    $staff =
        SELECT
            firstname_ru,
            lastname_ru,
            staff_user_login,
            FROM `//home/cloud-dwh/data/prod/ods/staff/PII/persons`;
    $email_id =
        SELECT
            base.crm_billing_account_id AS crm_billing_account_id,
            base.billing_account_id AS billing_account_id,
            base.crm_account_id AS crm_account_id,
            relation_crm_billing_account_id.date_modified_ts AS date_modified_ts,
            relation_crm_billing_account_id.primary_address AS primary_address,
            relation_crm_billing_account_id.crm_email_id AS crm_email_id
            FROM hahn.`home/cloud-dwh/data/prod/ods/crm/billing_accounts`
                AS base
            INNER JOIN hahn.`home/cloud-dwh/data/prod/ods/crm/crm_email_address_bean_relations`
                AS relation_crm_billing_account_id
            ON relation_crm_billing_account_id.bean_id == base.crm_billing_account_id
            WHERE relation_crm_billing_account_id.bean_module == 'BillingAccounts' AND NOT relation_crm_billing_account_id.deleted UNION ALL SELECT
            base.crm_billing_account_id AS crm_billing_account_id,
            base.billing_account_id AS billing_account_id,
            base.crm_account_id AS crm_account_id,
            relation_crm_account_id.crm_email_id AS crm_email_id,
            relation_crm_account_id.date_modified_ts AS date_modified_ts,
            relation_crm_account_id.primary_address AS primary_address,
            FROM `//home/cloud-dwh/data/prod/ods/crm/billing_accounts`
                AS base
            INNER JOIN `//home/cloud-dwh/data/prod/ods/crm/crm_email_address_bean_relations`
                AS relation_crm_account_id
            ON relation_crm_account_id.bean_id == base.crm_account_id
            WHERE relation_crm_account_id.bean_module == 'Accounts' AND NOT relation_crm_account_id.deleted;
    --email
    $emails =
        SELECT
            base.billing_account_id AS billing_account_id,
            base.date_modified_ts AS date_modified_ts,
            base.primary_address AS primary_address,
            email.email_address AS email_address
            FROM $email_id
                AS base
            INNER JOIN `//home/cloud-dwh/data/prod/ods/crm/PII/crm_email_addresses`
                AS email
            ON base.crm_email_id == email.crm_email_id
            WHERE coalesce(email.email_address, '') != '';
    $last_or_primary_email =
        SELECT
            billing_account_id,
            email_address,
            ROW_NUMBER() OVER w AS row_num
            FROM $emails
                AS base
            WINDOW
                w AS (
                    PARTITION BY
                        billing_account_id
                    ORDER BY
                        primary_address DESC,
                        date_modified_ts DESC
                );
    $final_email =
        SELECT
            billing_account_id,
            email_address,
            FROM $last_or_primary_email
            WHERE row_num == 1;
    --собираем 
    $final_pre =
        SELECT
            coalesce(base.billing_consumption_curr_quater, 0) AS billing_consumption_curr_quater,
            coalesce(base.billing_consumption_curr_quater_1, 0) AS billing_consumption_curr_quater_1,
            coalesce(base.billing_consumption_curr_quater_2, 0) AS billing_consumption_curr_quater_2,
            coalesce(base.billing_consumption_curr_quater_3, 0) AS billing_consumption_curr_quater_3,
            coalesce(base.billing_consumption_curr_quater_4, 0) AS billing_consumption_curr_quater_4,
            base.count_billing_account_id AS count_billing_account_id,
            base.date_qualified AS date_qualified,
            base.master_account_id AS master_account_id,
            coalesce(base.master_account_name, crm_contacts.lead_crm_account_name) AS account_name,
            coalesce(base.subacc_billing_consumption_curr_quater, 0) AS subacc_billing_consumption_curr_quater,
            coalesce(base.subacc_billing_consumption_curr_quater_1, 0) AS subacc_billing_consumption_curr_quater_1,
            coalesce(base.subacc_billing_consumption_curr_quater_2, 0) AS subacc_billing_consumption_curr_quater_2,
            coalesce(base.subacc_billing_consumption_curr_quater_3, 0) AS subacc_billing_consumption_curr_quater_3,
            coalesce(base.subacc_billing_consumption_curr_quater_4, 0) AS subacc_billing_consumption_curr_quater_4,
            coalesce(base.subaccount_num_curr_quater, 0) AS subaccount_num_curr_quater,
            coalesce(base.subaccount_num_curr_quater_1, 0) AS subaccount_num_curr_quater_1,
            coalesce(base.subaccount_num_curr_quater_2, 0) AS subaccount_num_curr_quater_2,
            coalesce(base.subaccount_num_curr_quater_3, 0) AS subaccount_num_curr_quater_3,
            coalesce(base.subaccount_num_curr_quater_4, 0) AS subaccount_num_curr_quater_4,
            coalesce(base.summary_consumption, 0) AS summary_consumption,
            coalesce(crm_contacts.first_name, crm_contacts.lead_first_name) AS first_name,
            coalesce(crm_contacts.last_name, crm_contacts.lead_last_name) AS last_name,
            email.email_address,
            descr.description AS description,
            coalesce(crm_contacts.phone_mobile, crm_contacts.lead_phone_mobile) AS phone_mobile,
            crm_contacts.lead_title AS lead_title,
            crm_contacts.lead_website AS lead_website,
            billing_contacts.backoffice_email AS backoffice_email,
            coalesce(CASE WHEN Yson::IsString(billing_contacts.backoffice_inn) THEN Yson::ConvertToString(billing_contacts.backoffice_inn) WHEN Yson::IsDouble(billing_contacts.backoffice_inn) THEN CAST(Yson::ConvertToDouble(billing_contacts.backoffice_inn) AS string) ELSE NULL END, crm_contacts.lead_inn) AS inn,
            billing_contacts.backoffice_inn AS backoffice_inn,
            crm_contacts.lead_inn AS lead_inn,
            billing_contacts.backoffice_name AS backoffice_name,
            billing_contacts.backoffice_phone AS backoffice_phone,
            roles.account_owner_current AS account_owner_current,
            roles.architect_current AS architect_current,
            roles.bus_dev_current AS bus_dev_current,
            coalesce(roles.partner_manager_current, base.partner_manager) AS partner_manager_current,
            roles.sales_current AS sales_current,
            c_dt.creation_dt AS creation_dt,
            coalesce(acc_subacc.all_active_sub_accounts_number, 0) AS all_active_sub_accounts_number,
            coalesce(acc_subacc.all_sub_accounts_number, 0) AS all_sub_accounts_number,
            en_dt.date_entered AS date_entered,
            base.date_qualified - en_dt.date_entered AS num_of_days_from_entered_to_qualified
            FROM $agg_consumption
                AS base
            LEFT JOIN $union_of_crm_contacts
                AS crm_contacts
            ON base.master_account_id == crm_contacts.billing_account_id
            LEFT JOIN $billing_contacts
                AS billing_contacts
            ON base.master_account_id == billing_contacts.billing_account_id
            LEFT JOIN $roles
                AS roles
            ON base.master_account_id == roles.billing_account_id
            LEFT JOIN $creation_date
                AS c_dt
            ON base.master_account_id == c_dt.billing_account_id
            LEFT JOIN $account_subaccount_and_status_agg
                AS acc_subacc
            ON base.master_account_id == acc_subacc.master_account_id
            LEFT JOIN $entered_date
                AS en_dt
            ON base.master_account_id == en_dt.billing_account_id
            LEFT JOIN $first_last_names
                AS first_last_name
            ON base.master_account_id == first_last_name.billing_account_id
            LEFT JOIN $final_email
                AS email
            ON base.master_account_id == email.billing_account_id
            LEFT JOIN $desc
                AS descr
            ON base.master_account_id == descr.billing_account_id;
    $final =
        SELECT
            t.*,
            partner_manager_current.firstname_ru || ', ' || partner_manager_current.firstname_ru AS partner_manager,
            sales_current.firstname_ru || ', ' || sales_current.firstname_ru AS sales,
            account_owner_current.firstname_ru || ', ' || account_owner_current.firstname_ru AS account_owner,
            architect_current.firstname_ru || ', ' || architect_current.firstname_ru AS architect,
            bus_dev_current.firstname_ru || ', ' || bus_dev_current.firstname_ru AS bus_dev
            FROM $final_pre
                AS t
            LEFT JOIN $staff
                AS partner_manager_current
            ON t.partner_manager_current == partner_manager_current.staff_user_login
            LEFT JOIN $staff
                AS sales_current
            ON t.sales_current == sales_current.staff_user_login
            LEFT JOIN $staff
                AS account_owner_current
            ON t.account_owner_current == account_owner_current.staff_user_login
            LEFT JOIN $staff
                AS architect_current
            ON t.architect_current == architect_current.staff_user_login
            LEFT JOIN $staff
                AS bus_dev_current
            ON t.bus_dev_current == bus_dev_current.staff_user_login;
    INSERT INTO $output_table_path
        WITH TRUNCATE
    SELECT
        *
        FROM $final
END DEFINE;
EXPORT $partner_datamart_script;