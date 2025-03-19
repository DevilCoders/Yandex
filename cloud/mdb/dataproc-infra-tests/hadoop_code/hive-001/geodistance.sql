ADD JAR s3a://dataproc-e2e/jobs/sources/java/dataproc-examples-1.0.jar;
CREATE TEMPORARY FUNCTION geodistance AS 'ru.yandex.cloud.dataproc.examples.GeoDistance';

SELECT name, population
FROM cities
WHERE geodistance(55.75222, 37.61556, latitude, longitude) < 200
ORDER BY population DESC
LIMIT 1 OFFSET 1;
