#!/usr/bin/env python
import requests
import re

#get list of cron indexes
try:
    cron_config = open("/usr/local/bin/elasticsearch_index_rotate","r").read()
    pattern = re.compile('INDEX_PREFIX=[^\ ]*')
    cron_indexes = []
    for i in re.findall(pattern, cron_config):
        cron_indexes.append(i.split("=")[1])
except:
    print "2;fail while read cron rotation config"
    exit(1)

#get all indexes from elastic
curl = "http://elastic.tst.ape.yandex.net:9200/_cat/indices?h=index"
try:
    r = requests.get(curl)
except:
    print "2;problem with get _stats from http://elastic.ape.yandex.net:9200/_cat/indices?h=index"
    exit(1)

#remove delimiter and allow indexes
indexes = r.text.rstrip("\n")
indexes = indexes.replace(" ","").replace("\t","")
indexes = indexes.split("\n")
for value in indexes:
    if "video" in value or "kibana-int" in value:
        indexes.remove(value)

#check trash index
for i in indexes:
    #check end of stroka: example 2016.08.16-21 or 2016.08.16 or 2016-08-15 or 2016-08-18-09
    if not re.match(".*20[0-9][0-9]\.[0-9][0-9]\.[0-9][0-9]-[0-9][0-9]$", str(i)) and not re.match(".*20[0-9][0-9]-[0-9][0-9]-[0-9][0-9]$", str(i)) and not re.match(".*20[0-9][0-9]-[0-9][0-9]-[0-9][0-9]-[0-9][0-9]$", str(i)) and not re.match(".*20[0-9][0-9]\.[0-9][0-9]\.[0-9][0-9]$", str(i)):
        print "2;find trash index: " + str(i)
        exit(1)

#check rotation good indexes
for i in indexes:
    index_in_cron = 0
    for l in cron_indexes:
        if l in i:
            index_in_cron = 1
            break
    if index_in_cron == 0:
        print "2;find not rotation index: " + str(i)
        exit(1)

print "0;Ok"
