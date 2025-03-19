use hahn;

$format = DateTime::Format("%Y-%m-%dT00:00:00");
$to_datetime = ($str) -> {RETURN DateTime::MakeDatetime(DateTime::ParseIso8601($str)) };
$month_ago = DateTime::MakeDatetime(DateTime::ShiftMonths(CurrentUtcDatetime(), -1)) ;
/* 
1. Пользователи, которые используют DL, но не используют другие сервисы облака. 
   И billing account ещё не создан. - Это же Демо-пользователи.
2. Пользователи, которые исп. бесплатный тариф DL и исп. другие сервисы облака. И billing account создан.*/

/* подбираем подходящие аккаунты в кубе*/
DEFINE subquery $cube_free() AS  (
    SELECT DISTINCT
        c.cloud_id AS cloud_id,
        c.user_settings_email AS user_settings_email,
        c.billing_account_id AS billing_account_id,
        c.ba_usage_status AS ba_usage_status,
        CASE WHEN c.billing_account_id =='' THEN 'demo-users'
            ELSE 'free-version'
        END AS task_part,
        CASE WHEN c.sales_name =='unmanaged' THEN 0
            ELSE 1
        END AS is_managed,
        
    FROM `//home/cloud_analytics/cubes/acquisition_cube/cube` AS c
    WHERE 
        ba_usage_status!='service'  /* не тестовый сервисный аккаунт */
        AND is_fraud = 0 /* не фрод */
    );
END DEFINE; 

/*табличка с актуальным состоянием подписки*/
DEFINE subquery $mail_info() AS (
    SELECT DISTINCT
        user_settings_email,
        mail_info,
        mail_promo,
        mail_feature,
        user_settings_language
    FROM `//home/cloud_analytics/import/iam/cloud_owners_history`
    WHERE (mail_info OR mail_promo OR mail_feature)
    );
END DEFINE;   
    
DEFINE subquery $data_lens() AS (
    SELECT 
        ba_cloud_folder.cloud_id
        FROM  `//home/cloud_analytics/dictionaries/ids/ba_cloud_folder_dict` AS ba_cloud_folder
    WHERE ba_cloud_folder.folder_id IN 
            (SELECT DISTINCT folder_id
             FROM RANGE (`//home/yandexbi/datalens-back/ext/production/requests`,
                    '2020-04-15T00:00:00', $format(CurrentUtcDateTime())   )
            )
    );
END DEFINE;     
    
/* 3. Выборка пользователей, которые потребляют MDB от 0 рублей за последние 30 дней и не используют DL - платные и триальщики. */

DEFINE subquery $no_data_lens() AS (
    SELECT 
        ba_cloud_folder.cloud_id
        FROM  `//home/cloud_analytics/dictionaries/ids/ba_cloud_folder_dict` AS ba_cloud_folder
    WHERE ba_cloud_folder.folder_id NOT IN 
            (SELECT DISTINCT folder_id
             FROM RANGE (`//home/yandexbi/datalens-back/ext/production/requests`,
                    '2020-04-15T00:00:00', $format(CurrentUtcDateTime())   )
            )
    );
END DEFINE;

DEFINE subquery $cube_mdb() AS (
    SELECT DISTINCT
        cloud_id,
        user_settings_email,
        billing_account_id,
        ba_usage_status,
        'mdb-nodl' as task_part,
        CASE WHEN sales_name =='unmanaged' THEN 0
        ELSE 1
    END AS is_managed
    FROM `//home/cloud_analytics/cubes/acquisition_cube/cube`
    WHERE event = 'day_use'
        AND service_name = 'mdb' -- потребляют MDB
        AND (real_consumption_vat >0 
             OR trial_consumption_vat >0 ) -- потребление >0
        AND $to_datetime(event_time) >= $month_ago -- за последний месяц
        AND is_fraud = 0 -- не фрод
    );
END DEFINE;

EXPORT $cube_mdb, $no_data_lens, $data_lens, $mail_info, $cube_free;