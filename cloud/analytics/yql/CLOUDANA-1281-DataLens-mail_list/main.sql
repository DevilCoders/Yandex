use hahn;

$to_date = DateTime::Format("%Y-%m-%d");

/* Активность в даталензе последние 2 недели */
IMPORT actions_in_dl SYMBOLS $week_2_actions, $dl_action;

IMPORT subtables SYMBOLS  $cube_mdb, $no_data_lens, $data_lens, $mail_info, $cube_free;

/* 
1. Пользователи, которые используют DL, но не используют другие сервисы облака. 
   И billing account ещё не создан. - Это же Демо-пользователи.
2. Пользователи, которые исп. бесплатный тариф DL и исп. другие сервисы облака. И billing account создан.*/

DEFINE subquery $dl_users() AS (
    SELECT DISTINCT
        cube_free.user_settings_email           AS user_settings_email,
        cube_free.billing_account_id            AS billing_account_id,
        cube_free.ba_usage_status               AS ba_usage_status,
        cube_free.is_managed                    AS is_managed,
        CASE WHEN week_2_actions.weeks_2_actions ==1  THEN 1
             ELSE 0
        END AS weeks_2_actions,
        CASE WHEN dl_action.dl_action_after_ba ==1  THEN 1
            ELSE 0
        END AS dl_action_after_ba,
        mail_info.mail_info                     AS mail_info,
        mail_info.mail_promo                    AS mail_promo,
        mail_info.mail_feature                  AS mail_feature,
        mail_info.user_settings_language        AS user_settings_language,
         cube_free.task_part                     AS task_part,
        CASE 
            WHEN Math::Mod(CAST(Digest::SipHash(0,0,cube_free.user_settings_email)AS Int64), 100) <50 THEN 'control'
            ELSE 'test'
        END                                     AS test_group,
        $to_date(CurrentUtcTimestamp())                            AS expirement_date
    FROM $cube_free() AS cube_free
    INNER JOIN $data_lens() AS no_data_lens ON no_data_lens.cloud_id = cube_free.cloud_id
    INNER JOIN $mail_info() AS mail_info ON cube_free.user_settings_email = mail_info.user_settings_email
    LEFT JOIN $week_2_actions() AS week_2_actions ON week_2_actions.billing_account_id = cube_free.billing_account_id
    LEFT JOIN $dl_action() AS dl_action ON dl_action.billing_account_id = cube_free.billing_account_id
    );
END DEFINE;

/* 3. Выборка пользователей, которые потребляют MDB от 0 рублей за последние 30 дней и не используют DL - платные и триальщики. */

DEFINE subquery $mdb_no_dl_users() AS (
    SELECT DISTINCT
        cube_mdb.user_settings_email     AS user_settings_email,
        cube_mdb.billing_account_id      AS billing_account_id,
        cube_mdb.ba_usage_status         AS ba_usage_status,
        cube_mdb.is_managed              AS is_managed,
        0 AS weeks_2_actions,
        0 AS dl_action_after_ba,
        mail_info.mail_info              AS mail_info,
        mail_info.mail_promo             AS mail_promo,
        mail_info.mail_feature           AS mail_feature,
        mail_info.user_settings_language AS user_settings_language,
        cube_mdb.task_part               AS task_part,
        CASE 
            WHEN Math::Mod(CAST(Digest::SipHash(0, 0, cube_mdb.user_settings_email) AS Int64), 100) < 50 THEN 'control'
            ELSE 'test'
        END                              AS test_group,
        $to_date(CurrentUtcTimestamp())                     AS expirement_date
    FROM $cube_mdb() AS cube_mdb
    INNER JOIN $no_data_lens() AS no_data_lens ON no_data_lens.cloud_id = cube_mdb.cloud_id
    INNER JOIN $mail_info() AS mail_info ON cube_mdb.user_settings_email = mail_info.user_settings_email
    );
END DEFINE;

EXPORT $dl_users, $mdb_no_dl_users;