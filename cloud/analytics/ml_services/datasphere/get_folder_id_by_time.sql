USE hahn;

$yson_string = ($value,$key) -> (Yson::LookupString(Yson::FromStringDict($key),$value));

DEFINE subquery $active_folder_id() AS (
    SELECT
    $yson_string("folder_id",`_rest`) as folder_id
    ,cast(cast($yson_string("time",`_rest`) as TimeStamp) as DateTime) AS time
FROM range (`home/logfeller/logs/yc-ai-prod-logs-users-ml-platform/1d`
            ,cast((CurrentUtcDate() - Interval("P2D")) as String) --curday-2
            ,cast((CurrentUtcDate() - Interval("P1D")) as String) --curday-1
           )
WHERE `type` = 'action'
    AND $yson_string("action",`_rest`) = 'stop'
    );
END DEFINE; 

EXPORT $active_folder_id;