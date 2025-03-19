import csv
import json
from pyspark import SparkConf, SparkContext
import sys

from geonames import parse_record


sc = SparkContext(conf=SparkConf().setAppName('Geonames').setMaster('yarn'))
jobid = dict(sc._conf.getAll())['spark.yarn.tags'].replace('dataproc_job_', '')
cities_uri = sys.argv[1]
output_uri = sys.argv[2].replace('${JOB_ID}', jobid)

with open('config.json') as config_file:
    config = json.load(config_file)

with open('country-codes.csv.zip/country-codes.csv') as csv_file:
    csv_reader = csv.reader(csv_file, delimiter=',')
    line_count = 0
    columns = {}
    country_name_by_code = {}
    for row in csv_reader:
        if line_count == 0:
            for idx, val in enumerate(row):
                columns[val] = idx
        else:
            name_column_idx = columns.get(config['name_column'], 2)
            name = row[name_column_idx]
            name = sc._jvm.ru.yandex.cloud.dataproc.examples.Utils.translit(config['transliterator'], name)
            code2 = row[6]
            country_name_by_code[code2] = name
        line_count += 1

raw_data = sc.textFile(cities_uri)
parsed_data = raw_data.map(parse_record)
parsed_data = parsed_data.cache()

parsed_data.map(lambda x: (x.country_code, x)).mapValues(lambda x: x.population).reduceByKey(
    lambda x, y: x + y
).repartition(1).map(lambda x: (country_name_by_code.get(x[0], x[0]), x[1])).map(
    lambda x: ','.join(map(str, x))
).saveAsTextFile(
    output_uri
)
