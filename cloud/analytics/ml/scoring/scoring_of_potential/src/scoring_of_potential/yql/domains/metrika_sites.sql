USE hahn;
PRAGMA yt.Pool = 'cloud_analytics_pool';

$result_table = '//home/cloud_analytics/scoring_of_potential/clean_id_graph/metrika_inns';
$passport_inns = (
    SELECT *
    FROM `//home/cloud_analytics/scoring_of_potential/clean_id_graph/passport_inns`
    WHERE max_level<3
);

$metrika_counters = (
    SELECT owner, site_path AS site
    FROM `//home/metrika/export/counters`
    WHERE status='Active'
);

INSERT INTO  $result_table WITH TRUNCATE 
SELECT DISTINCT * 
FROM $passport_inns AS passport_inns
INNER JOIN $metrika_counters AS metrika_counters
    ON metrika_counters.owner = CAST(passport_inns.src.value AS Int64)


