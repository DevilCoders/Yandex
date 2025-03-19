USE hahn;


$isv_inn = '//home/cloud_analytics/tmp/spark_isv_inn';
$sprav_clustred = '//home/altay-dev/svyatokum/ALTAY-12347/results/clustered_ex';
$result_table = '//home/cloud_analytics/tmp/sprav_isv_with_sprav_data';

$sparv_isv = (
    SELECT *
    FROM $isv_inn AS isv_inn
    LEFT JOIN $sprav_clustred AS sprav_clustered
    ON isv_inn.guid = sprav_clustered.GUID
);



$sparv = (
    SELECT
        permalink, main_url
    FROM `//home/sprav/assay/common/company_pretty_format`
);



INSERT INTO $result_table WITH TRUNCATE 
SELECT *
FROM $sparv_isv AS sprav_isv
LEFT JOIN $sparv AS sprav
    ON sprav_isv.head = sprav.permalink 