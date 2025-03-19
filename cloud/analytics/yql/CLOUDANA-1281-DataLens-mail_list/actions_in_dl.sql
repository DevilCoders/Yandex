use hahn;

$format = DateTime::Format("%Y-%m-%d");
$week_2_ago = DateTime::FromSeconds(CAST(DateTime::ToSeconds(CurrentUtcTimestamp())-60*60*24*14 AS Uint32));

DEFINE subquery $folder_ba() AS (
    SELECT 
        folder_id,
          cloud_id,
          billing_account_id
    FROM(
        SELECT 
          folder_id,
          cloud_id,
          billing_account_id,
          ts,
          ROW_NUMBER() OVER w1 AS rn
        FROM `//home/cloud_analytics/dictionaries/ids/ba_cloud_folder_dict`
        WHERE 
            cloud_id IS not NULL
            AND billing_account_id IS not NULL
        WINDOW w1 AS (PARTITION BY folder_id, cloud_id ORDER BY ts DESC) 
      )
    WHERE rn = 1);
END DEFINE;

DEFINE subquery $week_2_actions() AS (
    SELECT DISTINCT 
        folder_ba.billing_account_id AS billing_account_id,
        1 AS weeks_2_actions
    FROM RANGE('//home/yandexbi/datalens-back/ext/production/requests', 
                $format($week_2_ago), $format(CurrentUtcTimestamp())) AS t 
    INNER JOIN $folder_ba() AS folder_ba ON folder_ba.folder_id = t.folder_id
    WHERE t.folder_id IS NOT NULL
    );
END DEFINE;   

DEFINE subquery $dl_action() AS (
    SELECT DISTINCT 
        folder_ba.billing_account_id AS billing_account_id,
        1 AS dl_action_after_ba
    FROM RANGE('//home/yandexbi/datalens-back/ext/production/requests', 
               '2020-10-15T00:00:00', $format(CurrentUtcTimestamp())) AS t 
    INNER JOIN $folder_ba() AS folder_ba ON folder_ba.folder_id = t.folder_id
    INNER JOIN `//home/cloud_analytics/cubes/acquisition_cube/cube` AS c ON c.billing_account_id=folder_ba.billing_account_id
    WHERE 
        t.folder_id IS NOT NULL
    AND c.event='ba_created'
    AND c.event_time<t.event_time
    );
END DEFINE; 

EXPORT $week_2_actions, $dl_action;