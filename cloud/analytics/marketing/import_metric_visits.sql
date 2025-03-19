use hahn;

-- PRAGMA yt.Pool = 'cloud_analytics_pool';
--INSERT INTO `//home/cloud_analytics/import/metrika/2020_12_to_now` WITH TRUNCATE 

DEFINE action $imports($path) AS 
    INSERT INTO $path WITH TRUNCATE
    SELECT *
    FROM RANGE('//statbox/cooked_logs/visit-cooked-log/v1/1d', '2020-12-01' )
    WHERE CounterID in (50027884, 48570998, 51465824);
END DEFINE;

EXPORT $imports;