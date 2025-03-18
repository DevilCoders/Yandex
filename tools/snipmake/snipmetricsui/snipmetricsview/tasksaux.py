#!/usr/bin/python
# -*- coding: utf-8 -*-
import os
from subprocess import Popen, PIPE
from cStringIO import StringIO 
from snipmetricsview.models import TaskType
from snipmetricsview.log import writeToLog
from snipmetricsview.utils import * 
from django.template.loader import render_to_string
from django.core.files.base import ContentFile

def run(args):
    with open(get_option_value("logDir") + "error_log.txt","a") as log:
        Popen(args, shell=True, stderr=log).wait()

def getFileLinesCout(filename):
    with open(filename,"r") as f:
        r = sum(1 for l in f)
        return r

def createTask(id_name, details_text):
    date = datetime.now()
    baseTask = TaskType.objects.get(id=id_name)
    task = TaskProgress(taskType = baseTask, progress = 0, details = details_text + " - " + date.strftime("%H:%M:%S - %d.%m.%Y"), modificationDate = date)
    task.save()
    return task

def noExt(path):
    return os.path.join(os.path.dirname(path), os.path.basename(path).split('.')[0])

def wizName(path):
    return noExt(path) + ".wiz.gz"

def runCalc(poolName, calcArgs, errors, taskObj = None, unzip = True):
    line=' '
    metric_values = {}
    try:
        snips = open(poolName, "rb")
        fsize = os.path.getsize(poolName)
        writeToLog("Pool file: %s size: %s" % (poolName, fsize))
        # TODO: a dirty hack for "nonblocking_reading" from final pipe and controlling reading/calculating progress
        args = calcArgs
        if unzip and poolName.endswith(".gz"):
           args = "gunzip -c | " + args

        writeToLog("Calc args: " + args)
        error_log = open(get_option_value("logDir") + "error_log.txt", "a")
        calculator = Popen(args, shell = True, close_fds = True, cwd = "./" ,stdin = snips,  stdout = PIPE, stderr = error_log)

        prev = 0
        taskObj.update(progress = 0, details = taskObj.details + " ..calculating")
        while line:
            line = calculator.stdout.readline()
            if line:
                (snipId, query, url, metrics) = line.split("\t")
                for metric in metrics.split(" "):
                    metric = metric.strip()
                    if not metric:
                        continue
                    metric = metric.split(":")
                    if len(metric) != 2:
                        print >> errors, "Error parsing metric: [%s]" % metric
                        continue
                    val = float(metric[1])
                    if metric_values.has_key(metric[0]):
                        metric_values[metric[0]].append(val)
                    else:
                        metric_values[metric[0]] = [val]
                persent = 100.0 * snips.tell() / fsize
                if taskObj and persent - prev > 1.0:
                    #print >> error_log, "%s" % (persent)
                    taskObj.update(progress = persent)
                    prev = persent
    except:
        print >> errors, "Line: %s\n%s" % (line, str(exc_info())) 
    return metric_values

def saveDumpResults(dump, values):
    Min = {}
    Max = {}
    Mean = {}
    Median = {}
    Std = {}
    Histograms = {}
    MeanConfidenceInterval = {}

    bins = 20
    GetMetricsStatistics(values, bins, Min, Max, Mean, Median, Std, Histograms, MeanConfidenceInterval)
    stat = []
    for metric in Metric.objects.all():
        k = metric.shortName
        if values.has_key(k):
            stat.append({'name':metric.name.encode("utf-8"), 'min':Min[k], 'max':Max[k], 'mean':Mean[k], 'median':Median[k], 'std':Std[k], 'histogram':zip(Histograms[k][0], Histograms[k][1]), 'interval':MeanConfidenceInterval[k], 'width':(Max[k] - Min[k]) / bins})
            #writeToLog("[%i] %s" % (len(stat), stat[len(stat)-1]))

    context = {'dump':dump, 'stat':stat}
    report  = render_to_string('statistics.html', context)

    if dump.fileRes:
        writeToLog("[delete] previous result: %s" % dump.fileRes.name.encode("utf-8"))
        dump.fileRes.delete()

    dump.fileRes.save(dump.name+"_result.html", ContentFile(report.encode("utf-8")), save=True)
    writeToLog("[save] saveDumpResults dump: %s -> %s" % (dump.name.encode("utf-8"), dump.fileRes.name.encode("utf-8")))

def dumpWizards(dump, task, errors):
    wizFilename = wizName(dump.fileName.path)
    if os.path.exists(wizFilename):
        writeToLog("Dump: %s wizards file already exists: %s" % (dump.name, wizFilename))
        return wizFilename
    
    writeToLog("Make wizards requests: %s" % dump.fileName.path)
    info = task.details
    task.update(details = info + " ..parse serp for queries")
    tmpFilename = wizFilename + ".tmp"
    args = "zgrep -o 'query text=\".* reg-id=\".[0-9]*' %s | cut -d\\\" -f2,4 | tr \"\\\"\" \"\\t\" > %s" % (dump.fileName.path, wizFilename)
    writeToLog(args)
    run(args)
    l1 = getFileLinesCout(wizFilename)
    if l1 == 0: 
        print >> errors, "Empty wizards file: %s" % (wizFilename)
        return
    task.update(details=info + " ..dump wizards")
   
    queries = open(wizFilename, "rb")
    tmp = open(tmpFilename, "wb")
    args = "python %s --no-shuffle -s xmlsearch.hamster.yandex.ru -o '' -r ''" % get_option_value("wizardsScript")
    error_log = open(get_option_value("logDir") + "error_log.txt", "a")
    wiz = Popen(args, shell = True, close_fds = True, cwd = "./", stdin = queries, stdout = PIPE, stderr = error_log)
    index = 0
    line = ' '
    while line:
        line = wiz.stdout.readline()
        if line:
            tmp.write(line)
            index += 1
            if index % 100 == 0:
                task.update(progress = 99.0 * index / l1)
    task.update(details=info)
    tmp.close()
    if index < 0.99 * l1:
        print >> errors, "Invalid wizards count: %i need %i, (%s %s)" % (index, l1, dump.fileName.path, wizFilename)
        os.remove(wizFilename)
        wizFilename = None
    else:
        run("gzip -9ck %s  > %s" % (tmpFilename, wizFilename))
    os.remove(tmpFilename)
    return wizFilename 

def calcDump(dumpId):
    errors = StringIO()
    try:
        dump = SnipDump.objects.get(id=dumpId)
        writeToLog("[begin] calcDump dumpId: %d name: %s" % (dumpId, dump.name.encode("utf-8")))
        task = createTask("calc", dump.name)
        args = get_option_value("metricCalculatorPath") + "snipmetricsapp -a dump_metrics -d " + get_option_value("scriptsHomeDir")+"../arcadia_tests_data/wizard/language/stopword.lst -p " + get_option_value("scriptsHomeDir") + "../utility/porno_config.dat -o print"
        if dump.fileType == 5:
            args += " -m xml"
        if dump.fileType == 2:
            wizPath = dumpWizards(dump, task, errors)
            if wizPath:
                args += " -m serp -w " + wizPath

        values = runCalc(dump.fileName.path, args, errors, task, dump.fileType == 5)
        saveDumpResults(dump, values)
        task.update(progress = 100, errors = errors.getvalue() ,res_id = dumpId)
        writeToLog("[end] calcDump dumpId: %d name: %s\Errors: %s" % (dumpId, dump.name.encode("utf-8"), errors.getvalue()))
    except:
        writeToLog("Calc dump [%i]:%s\n%s\n" % (dumpId, errors.getvalue(), str(exc_info())) )
