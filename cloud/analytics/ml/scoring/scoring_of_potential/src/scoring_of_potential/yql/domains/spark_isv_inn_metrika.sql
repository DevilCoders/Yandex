USE hahn;
PRAGMA yt.Pool = 'cloud_analytics_pool';


$result_table = '//home/cloud_analytics/tmp/spark_isv_inn_metrika';

$spark_isv_inn = '//home/cloud_analytics/tmp/spark_isv_inn';
$metrika_sites = '//home/cloud_analytics/scoring_of_potential/clean_id_graph/metrika_inns';

INSERT INTO $result_table WITH TRUNCATE 
SELECT *
FROM $spark_isv_inn AS spark_isv_inn
LEFT JOIN $metrika_sites AS metrika_sites
    ON spark_isv_inn.inn  = CAST(metrika_sites.dst.value AS Int64)
    