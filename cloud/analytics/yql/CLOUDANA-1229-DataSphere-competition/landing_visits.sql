use hahn;

$format = DateTime::Format("%Y-%m-%dT%H:%M:%S");
$start_hour = $format(DateTime::MakeDatetime(DateTime::StartOfDay(CurrentUtcDatetime())) );
$end_hour = $format(DateTime::MakeDatetime(DateTime::StartOf(CurrentUtcDatetime(), Interval("PT1H"))));
 
DEFINE subquery
$all_valid_visits() AS 
    SELECT 
         MAX_BY(CAST(UserID AS string), (VisitVersion, -Sign)) AS yandexuid,
         MAX_BY(CAST(PassportUserID AS string), (VisitVersion, -Sign)) AS puid,
         MAX_BY(CAST(UTCStartTime as Datetime), (VisitVersion, -Sign)) AS StartDateTime,
         MAX_BY(UTMCampaign, (VisitVersion, -Sign)) AS UTMCampaign,
         MAX_BY(UTMSource, (VisitVersion, -Sign)) AS UTMSource ,
         MAX_BY(UTMMedium, (VisitVersion, -Sign)) AS UTMMedium
    FROM RANGE (`//home/logfeller/logs/visit-v2-private-log/1d`,
                '2020-12-01', CAST(CurrentUtcDate() AS String))
    WHERE 
        CounterID=69786625
    GROUP BY  VisitID
        HAVING MAX_BY(Sign, (VisitVersion, -Sign)) = 1
    UNION ALL 
    SELECT 
        MAX_BY(CAST(UserID AS string), (VisitVersion, -Sign)) AS yandexuid,
        MAX_BY(CAST(PassportUserID AS string), (VisitVersion, -Sign)) AS puid,
        MAX_BY(CAST(UTCStartTime as Datetime), (VisitVersion, -Sign)) AS StartDateTime,
        MAX_BY(UTMCampaign, (VisitVersion, -Sign)) AS UTMCampaign,
        MAX_BY(UTMSource, (VisitVersion, -Sign)) AS UTMSource ,
        MAX_BY(UTMMedium, (VisitVersion, -Sign)) AS UTMMedium
    FROM RANGE (`//home/logfeller/logs/visit-v2-private-log/1h`, $start_hour, $end_hour)
    WHERE 
        CounterID=69786625
    GROUP BY  VisitID
        HAVING MAX_BY(Sign, (VisitVersion, -Sign)) = 1
END DEFINE;
           
DEFINE subquery         
$first_landing_visits() AS (
    SELECT 
        utm_visits.yandexuid AS yandexuid,
        utm_visits.puid AS puid, 
        utm_visits.StartDateTime AS StartDateTime,
        utm_visits.UTMCampaign AS UTMCampaign,
        utm_visits.UTMSource AS UTMSource,
        utm_visits.UTMMedium AS UTMMedium
    FROM (
            SELECT 
                yandexuid,
                MIN(StartDateTime) AS StartDateTime
            FROM $all_valid_visits()
            GROUP BY 
                yandexuid
        ) AS first_visit
    INNER JOIN $all_valid_visits() AS utm_visits ON 
        utm_visits.yandexuid = first_visit.yandexuid
        AND  utm_visits.StartDateTime = first_visit.StartDateTime
   );
END DEFINE;


EXPORT $first_landing_visits, $all_valid_visits;