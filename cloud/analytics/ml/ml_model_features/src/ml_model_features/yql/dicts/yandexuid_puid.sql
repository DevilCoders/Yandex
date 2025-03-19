USE hahn;
PRAGMA yt.TmpFolder = '//home/cloud_analytics/ml/ml_model_features/tmp';
PRAGMA yt.QueryCacheTtl = '1h';

$yandexuid_puid = 
    SELECT 
        Cast(`id` AS UInt64) AS `yandexuid`,
        Cast(`target_id` AS UInt64) AS `puid`
    FROM `//home/crypta/production/state/graph/v2/matching/by_id/yandexuid/direct/puid`
;

$cloud_related_puids =
    SELECT DISTINCT `puid`
    FROM `//home/cloud_analytics/ml/ml_model_features/dict/baid_puid`
;

INSERT INTO `//home/cloud_analytics/ml/ml_model_features/dict/yandexuid_puid` WITH TRUNCATE 
    SELECT 
        yp.`yandexuid` AS `yandexuid`,
        yp.`puid` AS `puid`,
    FROM $yandexuid_puid AS yp
    INNER JOIN $cloud_related_puids AS crp ON yp.`puid` = crp.`puid`
    ORDER BY `yandexuid`, `puid`
;