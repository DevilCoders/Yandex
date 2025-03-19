Use hahn;
PRAGMA yt.Pool = 'cloud_analytics_pool';
PRAGMA OrderedColumns;
PRAGMA yt.TmpFolder = '//home/cloud_analytics/ml/ml_model_features/tmp';
PRAGMA AnsiInForEmptyOrNullableItemsCollections;

DECLARE $dates AS List<String>;

DEFINE ACTION $get_data_for_one_date($date) as
    $output = "//home/cloud_analytics/ml/ml_model_features/by_puid/metrika/visits/" || $date;
    $pattern = $date || "-%";
    $raw_data_path = "//home/cloud_analytics/ml/ml_model_features/raw/metrika/counter_51465824/visits/" || $date;
    
    $dwh_metrika = (SELECT
        SUBSTRING(CAST(CAST(`event_start_dt_utc` as DateTime) as String), 0, 10) as `date`,
        `visit_id`,
        DateTime::ToDays(CAST(`event_start_dt_utc` as DateTime) - CAST(TryMember(TableRow(), "FirstVisit", NULL) as DateTime)) as `days_since_first_visit`,
        TryMember(TableRow(), "browser_country", NULL) as `browser_country`,
        TryMember(TableRow(), "browser_language", NULL) as `browser_language`,
        TryMember(TableRow(), "domain_zone", NULL) as `domain_zone`,
        COALESCE(CAST(TryMember(TableRow(), "is_mobile", NULL) as Int32), 0) as `is_mobile`,
        DateTime::ToDays(CAST(`event_start_dt_utc` as DateTime) - CAST(TryMember(TableRow(), "last_visit", NULL) as DateTime)) as `days_since_last_visit`,
        TryMember(TableRow(), "os_family", NULL) as `os_family`,
        TryMember(TableRow(), "region_id", NULL) as `region_id`
    FROM LIKE(`//home/cloud-dwh/data/prod/ods/metrika/visit_log`, $pattern));

    INSERT INTO $output WITH TRUNCATE
    SELECT 
        x.`puid` as `puid`,
        y.`date` as `date`,
        MAX(`any_goal_reaches_num`) as `any_goal_reaches_num`,
        MAX(`audience_docs`) as `audience_docs`,
        MAX(`audience_prices`) as `audience_prices`,
        MAX(`audience_support`) as`audience_support`,
        SUM(`duration`) as `visit_duration_sum`,
        AVG(`duration`) as `visit_duration_mean`,
        MEDIAN(`duration`) as `visit_duration_median`,
        MAX(`duration`) as `visit_duration_max`,
        MIN(`duration`) as `visit_duration_min`,
        MAX(CAST((140396323 in `goals_id_list`) as Int32)) as `btn_сreated_project_datasphere`,
        MAX(CAST((128708506 in `goals_id_list`) as Int32)) as `btn_сreated_cluster_postgresql`,
        MAX(CAST((128708527 in `goals_id_list`) as Int32)) as `btn_сreated_cluster_mysql`,
        MAX(CAST((128708596 in `goals_id_list`) as Int32)) as `btn_сreated_cluster_clickhouse`,
        MAX(CAST((128708614 in `goals_id_list`) as Int32)) as `btn_сreated_cluster_mongodb`,
        MAX(CAST((128708743 in `goals_id_list`) as Int32)) as `btn_сreated_cluster_redis`,
        MAX(CAST((128709025 in `goals_id_list`) as Int32)) as `btn_сreated_data_proc`,
        MAX(CAST((128709046 in `goals_id_list`) as Int32)) as `btn_сreated_cluster_ydb`,
        MAX(CAST((226434222 in `goals_id_list`) as Int32)) as `created_support_ticket`,
        MAX(`is_mobile`) as `is_mobile`,
        MAX(`is_download`) as `is_download`,
        MAX(`is_logged_in`) as `is_logged_in`,
        MAX(`is_robot_internal`) as `is_robot_internal`,
        MAX(`is_turbo_page`) as `is_turbo_page`,
        MAX(`is_web_view`) as `is_web_view`,
        SUM(`loaded_docs`) as `num_loaded_docs`,
        SUM(`submit_forms`) as `num_submit_forms`,
        SUM(`visits_direct`) as `num_visits_direct`,
        SUM(`visits_external_links`) as `num_visits_external_links`,
        SUM(`visits_from_ads`) as `num_visits_from_ads`,
        SUM(`visits_from_letters`) as `num_visits_from_letters`,
        SUM(`visits_from_messangers`) as `num_visits_from_messangers`,
        SUM(`visits_from_recommenders`) as `num_visits_from_recommenders`,
        SUM(`visits_from_saved`) as `num_visits_from_saved`,
        SUM(`visits_from_search`) as `num_visits_from_search`,
        SUM(`visits_from_site_links`) as `num_visits_from_site_links`,
        SUM(`visits_from_social_media`) as `num_visits_from_social_media`,
        SUM(`visits_iternal_links`) as `num_visits_iternal_links`,
        MAX(`days_since_first_visit`) as `num_days_since_first_visit`,
        MODE(`browser_country`)[0].Value as `browser_country`,
        MODE(`domain_zone`)[0].Value as `domain_zone`,
        MIN(`days_since_last_visit`) as `days_since_last_visit`,
        SUM(CAST((`os_family` ==  "Linux") as Int32)) as `visits_from_linux`,
        SUM(CAST((`os_family` ==  "Android") as Int32)) as `visits_from_android`,
        SUM(CAST((`os_family` ==  "iOS") as Int32)) as `visits_from_ios`,
        SUM(CAST((`os_family` ==  "Windows") as Int32)) as `visits_from_windows`,
        SUM(CAST((`os_family` ==  "MacOS") as Int32)) as `visits_from_macos`,
        COUNT(x.`visit_id`) as `visits_day_count`
    FROM $raw_data_path as x
    JOIN $dwh_metrika as y
    ON x.`visit_id` == y.`visit_id`
    GROUP BY x.`puid`, y.`date`
END DEFINE;

EVALUATE FOR $date IN $dates
    DO $get_data_for_one_date($date)