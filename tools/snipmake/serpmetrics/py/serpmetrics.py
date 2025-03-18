#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys
import re
import threading
import urllib
import urllib2
import json
from subprocess import check_output
from multiprocessing.dummy import Pool as ThreadPool

from sniplist import metrics_base, razladki_base
from xmlglue import *
from extmetrics import *


def checkSerpDate(filename, date):
    try:
        serpDate = check_output('zgrep -A 3 "<serp-set" %s | grep -o "<date>.*</date>"' % filename, shell=True)
    except:
        log("Exception: %s" % str(sys.exc_info()[1]))
        return False
    serpDate = serpDate[6:]

    if not serpDate.startswith(date):
        log("Invalid date: %s != %s" % (serpDate, date))
        return False
    return True

serpLock = threading.Lock()

def askAng(request):
    req = urllib2.Request(request)  # TODO: get token from some secure place and put it into header: urllib2.Request(request, headers={'Authorization': TOKEN})
    return urllib2.urlopen(req).read()

def downloadSerp(filename, date, stream):
    try:
        serpLock.acquire()
        evaluation = config.serp_type["evaluation"] if "evaluation" in config.serp_type else "WEB"
        request1 = "http://metrics.yandex-team.ru/services/api/serpset/list/%(country)s/%(evaluation)s/?from=%(date)sT00:00:00.000&to=%(date)sT23:59:59.000&cronSerpDownloadId=%(sysId)s" % {
            "country": config.serp_type["country"],
            "evaluation":evaluation,
            "sysId":stream["sysId"],
            "date":date
        }
        log("Ask 1: %s" % request1)
        serpSetIds = askAng(request1)
        sId = json.loads(serpSetIds)
        if not sId:
            raise Exception("Empty ids list")

        request2 = "http://metrics-calculation.qe.yandex-team.ru/api/qex/xml-serp-export?regional=%(country)s&evaluation=%(evaluation)s&serp-set=%(serpId)s" % {
            "country":config.serp_type["country"],
            "evaluation":evaluation,
            "serpId":sId[-1]
        }
        log("Ask 2: %s" % request2)
        serpData = askAng(request2)
        log("Recieve %i" % len(serpData))
        serp = open(filename, 'wb')
        serp.write(serpData)
        serp.close()
    except Exception as e:
        log("Serp fetch fail: %s [%s] %s" % (stream["name"], date, e))
    finally:
        serpLock.release()

def getFileLinesCout(filename):
    with open(filename,"r") as f:
        r = sum(1 for l in f)
        return r

def processWizards(serpFilename, wizFilename, tmpFilename):
    log("make wizards requests")
    run('zgrep -A 4 "<query" %s | paste -s | sed "s/<\/query>/\\n/g" | grep -oP "text>.*<region id=\\"\d+" | sed "s/text>//g" | sed "s/<\/.*id=\\"/\\\\t/g" > %s' % (serpFilename, wizFilename))
    l1 = getFileLinesCout(wizFilename)
    log("downloadWizards")
    run("python %s --no-shuffle --no-fake -s %s -r %s -o %s" % (config.wizards_py, config.wizards_url, wizFilename, tmpFilename) )
    l2 = getFileLinesCout(tmpFilename)
    if l2 < 0.5 * l1:
        log("Invalid wizards count: %i need %i, (%s %s)" % (l2, l1, serpFilename, wizFilename))
        os.remove(wizFilename)
    else:
        log("zip wizards")
        run("gzip -9c %s  > %s" % (tmpFilename, wizFilename))
    os.remove(tmpFilename)

def processCalc(serpFilename, wizFilename, resFilename):
    log("calcMetrics [Begin]")
    run("%s -a serp_metrics -d %s -p %s -i %s -w %s -r %s" % (  config.metrics, config.stopwords, config.porno_config,
                                                                serpFilename, wizFilename, resFilename))
    auxMetrics = {} # metric name -> (metric value, metric size)
    auxMetrics.update(calcFavIcons(serpFilename))

    patchPoint(resFilename, auxMetrics)
    log("calcMetrics [End]")

def uploadGraph(filename, plot):
    log("uploadGraph %s name: %s" % (filename, plot))
    run("curl -S -d @%s 'http://metrics.yandex-team.ru/hist/graph/upload?file-name=snippets&type=%s'" % (filename, plot))

def days_range(begin,dayStep):
    end = begin + datetime.timedelta(days=dayStep)
    if begin > end:
        begin,end = end,begin
    while begin <= end:
        yield begin
        begin += datetime.timedelta(days=1)

def makeGraph(graph, date, streams):
    log("make plot")
    w = XmlWriter(graph)
    w.startDocument()
    w.startElement("graph", {   "need-data-table"       : "true",
                                "need-relative"         : "true",
                                "hide-comments"         : "true",
                                "no-mist-points"        : "true",
                                "dinamic-vertical-scale": "true",
                                "show-prefs-on-graph"   : "true",
                                "config-on-graph"       : "true",
                                "title" : "%s (%s)" % (config.plot_name, date2str(date)),
                            })
    w.eol()
    w.startElement("systems")
    w.eol()

    for s in streams:
        w.startElement("system", {"name":s["file"], "title":s["file"]})
        w.endElement()
    w.endElement()

    w.startElement("options", {"type":"metric"})
    w.eol()
    for m in metrics_base:
        w.startElement("option", {"text":m[1], "value":m[0]})
        w.endElement()
    w.endElement()

    for m in metrics_base:
        w.startElement("day-group")
        w.eol()
        w.startElement("option", {"type":"metric", "value":m[0]})
        w.endElement()
        for root, dirs, files in os.walk(config.data_folder):
            for date_str in sorted(dirs):
                w.startElement("day", {"date":date_str})
                w.eol()
                for s in streams:
                    filename = getPointName(date_str, s["file"])
                    val = {}
                    if os.path.exists(filename):
                        try:
                            point = readPoint(filename)
                            if len(point) == 0:
                                raise ValueError, "Empty point file"
                            if point.has_key(m[0]):
                                metric_data = point[m[0]]
                                val = {"value": metric_data[0], "size": metric_data[1]}
                        except:
                            log("%s\nPoint file: %s Metric: %s" % (str(sys.exc_info()[1]),filename, m[0]))
                            removePath(getSerpName(date_str, s["file"]))
                            removePath(getWizName(date_str, s["file"]))
                            removePath(filename)
                    w.startElement("point", val)
                    w.endElement()
                w.endElement()
        w.endElement()

    w.endElement()
    w.endDocument()

def isExternal(ang):
    return ang == "External"

def downloadExternalPoint(date, point):
    line = "wget -nv -a %s -O %s '%s?action=get&date=%s&serp=%s'" % (config.log, point, config.external_source, date, config.serp_type["country"])
    log("downloadExternal -> %s\n%s" % (point, line))
    run(line)

def createSerpPoint(date, point, stream):
    filename = stream["file"]
    serp = getSerpName(date, filename)
    if not os.path.exists(serp):
        downloadSerp(serp, date, stream)
    if not os.path.exists(serp):
        log("Missing serp: %s" % serp)
        return
    if not checkSerpDate(serp, date):
        os.remove(serp)
        return
    wiz = getWizName(date, filename)
    if not os.path.exists(wiz):
        processWizards(serp, wiz, getTempName(date, filename))
    if os.path.exists(wiz):
        processCalc(serp, wiz, point)
    else:
        log("Missing wiz: %s" % wiz)

def send2razladki(date, point, stream):
    if not os.path.exists(point):
        return
    if stream["file"] not in config.razladki_streams:
        return
    razladki_stream = config.razladki_streams[stream["file"]]
    data = readPoint(point)
    query = {
        "ts": date2timestamp(str2date(date)),
    }
    prefix = razladki_stream + "|" + config.razladki_metrics_pref + "-"
    for metric in razladki_base:
        if metric in data:
            value = data[metric]
            key = prefix + razladki_base[metric]
            query[key] = float(value[0])
    try:
        urllib2.urlopen(config.razladki_url, urllib.urlencode(query))
    except urllib2.HTTPError as e:
        if e.code == 409:
            log("Alredy have data on razladki at timestamp %s" % query["ts"])
        else:
            raise

def doJob(args):
    date, today, stream = args
    point = getPointName(date, stream["file"])
    if not os.path.exists(point):
        log("Missing point: %s" % point)
        if isExternal(stream["name"]):
            if date != date2str(today):
                downloadExternalPoint(date, point)
        else:
            createSerpPoint(date, point, stream)
            send2razladki(date, point, stream)

def processDates():
    try:
        today = datetime.date.today()
        log("\n\nDate: %s" % date2str(today))
        period = -config.period
        streams = config.streams
        jobs = []
        for d in days_range(today, period):
            date = date2str(d)
            createPath(os.path.join(config.data_folder, date))
            for stream in streams:
                point = getPointName(date, stream["file"])
                if not os.path.exists(point):
                    jobs.append((date, today, stream))

        if config.threads == 1:
            map(doJob, jobs)
        else:
            pool = ThreadPool(config.threads)
            pool.map_async(doJob, jobs, chunksize=1).get(24*60*60)   # pool.map(..) doesn't reraise exceptions, but pool.map_async(..).get(timeout) does
        graph = getGraphName(today, streams)
        makeGraph(graph, today, streams)
        uploadGraph(graph, config.plot_type)
        run("gzip -9f %s" % graph)
    except:
        log("Exception: %s" % (str(sys.exc_info())))
        raise

def processSerp(filename):
    wiz = filename.replace(".xml.gz", ".wiz.gz")
    res = filename.replace(".xml.gz", ".point")
    if not os.path.exists(wiz):
        processWizards(filename, wiz, res)
    processCalc(filename, wiz, res)

if __name__ == "__main__":
    if config.options.serp:
        processSerp(config.options.serp)
    else:
        processDates()
