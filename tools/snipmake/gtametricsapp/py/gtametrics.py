#!/usr/bin/env python

import sys
import os
import subprocess
import logging
import MySQLdb
import time
import optparse
from xml.parsers import expat

import config
from db import DBWorker

def parse_args():
    parser = optparse.OptionParser()
    parser.add_option("-t", dest="serpType", metavar="{ru|tr}", default="ru", help = "serp type")
    return parser.parse_args()

class Application:
    def __init__(self, options, config):
        self.serpType = options.serpType
        self.cfg = config
        self.db = DBWorker(config)
        self.dataFolder = "data_%s" % self.serpType
        if not os.path.exists(self.dataFolder):
            os.makedirs(self.dataFolder)

    def run(self, args, raiseOnError=True):
        logger = logging.getLogger("gtametrics")
        logger.debug("execute [%s]" % args)
        retcode = subprocess.Popen(args, shell=True).wait()
        logger.debug("retcode: %s" % retcode)
        if retcode != 0 and raiseOnError:
            raise Exception("non zero exit code")
        return retcode

    def runMetricsApp(self, queriesFile, metricsFile):
        line = "%s -i %s > %s" % (self.cfg.metricsBinary, queriesFile, metricsFile)
        self.run(line)

    def recodeQueries(self, rawQueriesFile, queriesFile):
        with open(rawQueriesFile, "r") as reader:
            with open(queriesFile, "w") as writer:
                for line in reader:
                    query, region = line.split("\t")[:2]
                    print >>writer, "\t".join([query, region, self.serpType])

    def saveToDb(self, metricsFile, date):
        data = []
        def start(tag, attrs):
            if tag == "point" and len(attrs) == 3:
                data.append((attrs["system"], attrs["value"], attrs["size"]))

        with open(metricsFile, "r") as reader:
            parser = expat.ParserCreate()
            parser.StartElementHandler = start
            parser.ParseFile(reader)

        cursor = self.db.getCursor()
        cursor.executemany("delete from metrics where name=%s and date=%s and serp=%s", [(item[0], date, self.serpType) for item in data])
        cursor.executemany("insert into metrics values(%s, %s, %s, %s, %s)", [(name, date, self.serpType, value, size) for name, value, size in data])

    def calcMetrics(self, date):
        rawQueriesFile = os.path.join(self.dataFolder, "raw_queries.txt")
        queriesFile = os.path.join(self.dataFolder, "queries.txt")
        metricsFile = os.path.join(self.dataFolder, "metrics.txt")
        self.recodeQueries(rawQueriesFile, queriesFile)
        self.runMetricsApp(queriesFile, metricsFile)
        self.saveToDb(metricsFile, date)

def main():
    curTime = time.localtime()
    options, args = parse_args()
    app = Application(options, config)
    app.calcMetrics("{yyyy:04}_{mm:02}_{dd:02}".format(yyyy=curTime.tm_year, mm=curTime.tm_mon, dd=curTime.tm_mday))

if __name__ == "__main__":
    logger = logging.getLogger("gtametrics")
    logger.setLevel(logging.DEBUG)
    handler = logging.StreamHandler()
    handler.setFormatter(logging.Formatter("%(asctime)s - %(message)s"))
    logger.addHandler(handler)
    main()
