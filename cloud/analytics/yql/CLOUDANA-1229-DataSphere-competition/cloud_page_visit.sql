use hahn;

$time_from_seconds = ($double) -> {RETURN DateTime::FromMilliseconds(CAST($double*1000 AS Uint64))};    

DEFINE subquery $all_visits_site() AS (
    SELECT 
        yandexuid,
        $time_from_seconds(ts) AS ts
    FROM `//home/cloud_analytics/import/console_logs/events` AS events
    WHERE 
        events.event_type="pageview");
END DEFINE;
                    
DEFINE subquery $first_utm_visit_site() AS (
    SELECT 
        yandexuid,
        MIN($time_from_seconds(ts)) AS ts
    FROM `//home/cloud_analytics/import/console_logs/events` AS events
    WHERE 
        events.event_type="pageview"
      AND events.url like "%=ds2020%"
    GROUP BY 
       yandexuid
    );
END DEFINE;


DEFINE SUBQUERY 
$visit_ds_page() AS
    SELECT 
        yandexuid,
        $time_from_seconds(ts) AS ts
    FROM `//home/cloud_analytics/import/console_logs/events` AS events
    WHERE 
        events.event_type="pageview"
        AND url like "https://console.cloud.yandex.ru/folders/%"
        AND url like  "%datasphere%"
        AND url not like  "%create-project%"
   ;
END DEFINE;

DEFINE SUBQUERY 
$visit_create_project_page() AS
    SELECT 
        yandexuid,
        $time_from_seconds(ts) AS ts
    FROM `//home/cloud_analytics/import/console_logs/events` AS events
    WHERE 
        events.event_type="pageview"
        AND url like "https://console.cloud.yandex.ru/folders/%"
        AND url like  "%datasphere%"
        AND url like  "%create-project%"
   ;
END DEFINE;

EXPORT $all_visits_site, $first_utm_visit_site, $visit_ds_page, $visit_create_project_page;