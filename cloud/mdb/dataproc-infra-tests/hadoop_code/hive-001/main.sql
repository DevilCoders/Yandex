CREATE EXTERNAL TABLE IF NOT EXISTS cities
(
    geonameid int,
    name string,
    asciiname string,
    alternatenames string,
    latitude float,
    longitude float,
    feature_class string,
    feature_code string,
    country_code string,
    cc2 string,
    a1 string,
    a2 string,
    a3 string,
    a4 string,
    population int,
    elevation int,
    dem int,
    timezone string,
    modification_date date
)
ROW FORMAT DELIMITED
FIELDS TERMINATED BY '\t'
LINES TERMINATED BY '\n'
STORED AS TEXTFILE
LOCATION '${CITIES_URI}';


SELECT sum(population) FROM cities WHERE country_code='${COUNTRY_CODE}';
