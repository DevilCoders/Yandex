#!/usr/bin/python
# -*- coding: utf-8 -*-

from os import write, listdir, path, chmod, getcwd, remove
from math import sqrt
import stat
import tempfile
import random
from django.utils import encoding
from django.db import transaction, connection
from snipmetricsview.log import *
from snipmetricsview.tasks import *
from snipmetricsview.models import *
from snipmetricsview.mathutil import *

from codecs import decode, encode
import sys

def prepareDB():
    requiredTypes = {"calc": u"Подсчет метрик", "dump" : u"Сбор сниппетов", "uploadSnippets" : u"Загрузка сниппетов из файла", "uploadSerp" : u"Загрузка серпа из файла"}
    foundTypes = set([])
    taskTypes = TaskType.objects.all()
    for taskType in taskTypes:
        foundTypes.add(taskType.id)
    for requiredType in requiredTypes.iterkeys():
        if requiredType not in foundTypes:
            newType = TaskType(id = requiredType, name = requiredTypes[requiredType])
            newType.save()

class MetricsException(Exception):
    def __init__(self, message):
        self.message = message

    def __str__(self):
        return str(self.message)

def getRandomFileName(suffixStr = ""):
    tempf, path = tempfile.mkstemp(prefix ='sm_',suffix="_"+suffixStr, dir = get_option_value("tempDir"))
    return path, tempf

def generateShortUrl():
    import string
    len = random.randint(5,10)
    return ''.join(random.choice(list(string.letters.upper()) + ['0','1','2','3','4','5','6','7','8','9'] + list(string.letters) + ['_']) for i in xrange(len))

def flush_uploaded_to_temp(uploaded_file):
    path, tempf = getRandomFileName(str(uploaded_file))
    for chunk in uploaded_file.chunks():
        write(tempf, chunk)
    return path

def fillFilesList(path, lst):
    del lst[:]
    for fileName in listdir(path):
        if fileName.find("__init__") >= 0 or fileName.find(".svn") >= 0:
            continue
        lst.append((path + fileName,fileName))

def fillMetricsTree(tree, selected = set([])):
    del tree[:]
    categories = MetricCategory.objects.all()
    for category in categories:
        cat_metrics = Metric.objects.filter(categories__id = category.id)
        if cat_metrics.count() > 0:
            cat_subtree = [(u"cat" + str(category.id), encoding.smart_unicode(category.name), ("cat" + str(category.id)) in selected)]
            for metric in cat_metrics:
                cat_subtree.append((u"met" + str(metric.id), encoding.smart_unicode(metric.name), ("met" + str(metric.id)) in selected))
            tree.append(cat_subtree)

def fillMetricEditForm(form, metric):
    form.fields["name"].initial = metric.name
    form.fields["shortName"].initial = metric.shortName
    form.fields["description"].initial = metric.description
    form.fields["calculatorExePath"].initial = metric.calculator.exePath
    form.fields["categories"].initial = metric.categories.all()
    form.fields["commandLineParams"].initial = metric.calculator.commandLineParams

def fillSnipDumpsList(lst, withMetricsOnly = False):
    del lst[:]
    for dump in SnippetsDump.objects.filter(isArchive = False).select_related().all():
        if withMetricsOnly:
            cursor = connection.cursor()
            cursor.execute("SELECT (1) AS `a` FROM `snipmetricsview_metricvalue` INNER JOIN `snipmetricsview_snippet` ON (`snipmetricsview_metricvalue`.`snippet_id` = `snipmetricsview_snippet`.`id`) WHERE `snipmetricsview_snippet`.`snippetsDump_id` = %s  LIMIT 1", [dump.id])
            if not cursor.fetchone():
            #if not MetricValue.objects.filter(snippet__snippetsDump = dump.id).exists(): # This works as SQL query above, but the function exists since Django 1.2 only
                continue
        lst.append((dump.id, encoding.smart_unicode(dump)))

def fillQueryWordsCountList(lst):
    del lst[:]
    for i in range(1,7):
        lst.append((str(i), str(i)))
    lst.append((">6", ">6"))

def fillQueryCharacteristicList(lst):
    del lst[:]
    for characteristic in QueryCharacteristic.objects.all():
        lst.append((characteristic.name, characteristic))

def saveUploadedFile(fileName, uploadedFile):
    out = open(fileName, "w")
    for chunk in uploadedFile.chunks():
        out.write(chunk)
    out.close()

def add_serp_from_file(name, fileFormat, serpFile):
    try:
        dataFile = flush_uploaded_to_temp(serpFile)
        date = datetime.now()
        if serpFile != None:
            uploadSerpTaskType = TaskType.objects.get(id="uploadSerp")
            task = TaskProgress(taskType = uploadSerpTaskType, progress = 0, details = name + " - " + date.strftime("%H:%M:%S - %d.%m.%Y"), modificationDate = date)
            task.save()
            startUploadSerpTask(task.id, name, dataFile, fileFormat)
    except:
        writeToLog("dumps_dump.html", [])
        transaction.rollback()
        raise

def addQueriesListFromFile(name, fileName):
    try:
        date = datetime.now()
        if fileName != None:
            uploadQueriesTaskType = TaskType.objects.get(id="uploadQueries")
            task = TaskProgress(taskType = uploadQueriesTaskType, progress = 0, details = name + " - " + date.strftime("%H:%M:%S - %d.%m.%Y"), modificationDate = date)
            task.save()
            startUploadQueriesTask(task.id, name, fileName)
    except:
        writeToLog("dumps_dump.html", [])
        transaction.rollback()
        raise

def add_engine(name, desc, url, searchByUrlFilter, cgi, wizardUrl, documentCacheUrlTemplate):
    engine = SnippetsEngine()
    modify_engine(engine, name, desc, url, searchByUrlFilter, cgi, wizardUrl, documentCacheUrlTemplate)

def edit_engine(id, name, desc, url, searchByUrlFilter, cgi, wizardUrl, documentCacheUrlTemplate):
    engine = SnippetsEngine.objects.get(id = id)
    modify_engine(engine, name, desc, url, searchByUrlFilter, cgi, wizardUrl, documentCacheUrlTemplate)

def modify_engine(engine_obj, name, description, url, searchByUrlFilter, cgiParams, wizardUrl, documentCacheUrlTemplate):
   engine_obj.name = name
   engine_obj.url = url
   engine_obj.searchByUrlFilter = searchByUrlFilter
   engine_obj.description = description
   engine_obj.wizardUrl = wizardUrl
   engine_obj.cgiParams = cgiParams
   engine_obj.documentCacheUrlTemplate = documentCacheUrlTemplate
   engine_obj.save()

def add_metric(name, shortName, description, categories, calculatorPath, calculatorFile, commandLineParams, editMetricId = -1):
    if calculatorFile != None:
        if path.exists(get_option_value("metricCalculatorPath") + calculatorFile.name):
            raise MetricsException("A file with the given name already exists")
        saveUploadedFile(get_option_value("metricCalculatorPath") + calculatorFile.name, calculatorFile)
        chmod(get_option_value("metricCalculatorPath") + calculatorFile.name, stat.S_IEXEC)
        calcs = MetricCalculator.objects.filter(exePath = get_option_value("metricCalculatorPath") + calculatorFile.name).filter(commandLineParams = commandLineParams)
        if calcs.count() > 0:
            calc = calcs[0]
        else:
            calc = MetricCalculator(exePath = get_option_value("metricCalculatorPath") + calculatorFile.name, commandLineParams = commandLineParams)
        calc.save()
        if editMetricId >= 0:
            metric = Metric.objects.get(id = editMetricId)
            metric.name = name
            metric.shortName = shortName
            metric.description = description
        else:
            metric = Metric(name = name, shortName = shortName)
        metric.calculator = calc
    else:
        if editMetricId >= 0:
            metric = Metric.objects.get(id = editMetricId)
            metric.name = name
            metric.shortName = shortName
            metric.description = description
            calcs = MetricCalculator.objects.filter(exePath = calculatorPath).filter(commandLineParams = commandLineParams)
            if calcs.count() > 0:
                metric.calculator = calcs[0]
            else:
                calc = MetricCalculator(exePath = calculatorPath, commandLineParams = commandLineParams)
                calc.save()
                metric.calculator = calc
        else:
            metric = Metric(name = name, shortName = shortName, description = description)
            metric.calculator = MetricCalculator.objects.filter(exePath = calculatorPath)[0]
    metric.save()
    metric.categories = []
    for metric_category in [MetricCategory.objects.filter(name = cat_name) for cat_name in categories]:
        metric.categories.add(metric_category[0])
    metric.save()


def addSnippetsDump(newSerpName, engineUrl, cgiParams, wizardUrl, fileType, dumpFile, startCalc = False):
    writeToLog("[util.py] addSnippetsDump")
    try:
        date = datetime.now()
        dumpParseTaskType = TaskType.objects.get(id="uploadSnippets")
        task = TaskProgress(taskType = dumpParseTaskType, progress = 0, details = newSerpName + " - " + date.strftime("%H:%M:%S - %d.%m.%Y"), modificationDate = date)
        task.save()
        dataFile = flush_uploaded_to_temp(dumpFile)
        startUploadSnippetsTask(task.id, newSerpName, engineUrl, cgiParams, wizardUrl, fileType, dataFile)
        if startCalc:
            metrics = Metric.objects.all().only("id").values()
            metrics = [m['id'] for m in metrics]
            calc_metrics(metrics, -1, newSerpName, False, task.id)
    except:
        writeToLog("upload_snips.html", [])
        transaction.rollback()
        raise

def startCalcDump(dumpId):
    p = Popen("python " + get_option_value("scriptsHomeDir")  + "start_daemon.py -calc_dump %d" % dumpId, shell = True, close_fds = True, cwd = getAbsCwd())

def addSnippets(newSerpName, engineUrl, cgiParams, wizardUrl, fileType, dumpFile):
    writeToLog("[util.py] addSnippets")
    try:
        dump = SnipDump(name = encode(newSerpName, "utf-8"), date = datetime.now(), fileType = fileType, fileName = dumpFile)
        dump.save()
        if not dumpFile.name.endswith(".gz"):
            writeToLog("gzip file: %s" % dump.fileName)
            Popen("gzip -9 %s" % dump.fileName.path, shell = True).wait()
            dump.fileName.name = dump.fileName.name + ".gz"
            dump.save()

        writeToLog("Run task calcDump:  dump_id: %d serpName: %s type: %i" % (dump.id, dump.name, fileType))
        startCalcDump(dump.id)
    except:
        writeToLog("snips_add.html", str(exc_info()))
        transaction.rollback()
        raise


def dumpSnippets(queryListId, newSerpName, engineUrl, searchByUrlFilter, cgiParams, wizardUrl, documentCachePath, startCalc = False):
    try:
        date = datetime.now()
        dumpTaskType = TaskType.objects.get(id="dump")
        task = TaskProgress(taskType = dumpTaskType, progress = 0, details = newSerpName, modificationDate = date)
        task.save()
        startDumpTask(task.id, queryListId, newSerpName, engineUrl, searchByUrlFilter, cgiParams, wizardUrl, documentCachePath)
        if startCalc:
            metrics = Metric.objects.all().only("id").values()
            metrics = [str(m['id']) for m in metrics]
            calc_metrics(metrics, -1, newSerpName, False, task.id)
    except:
        writeToLog("snips_dump.html", [])
        transaction.rollback()
        raise

def calc_metrics(metrics, snipDumpId, snipDumpName, removePrevValues, waitForTaskId = -1):
    try:
        calcTaskType = TaskType.objects.get(id = "calc")
        task = TaskProgress(taskType = calcTaskType, progress = 0.0, details = snipDumpName, modificationDate = datetime.now())
        task.save()
        startCalcTask(task.id, snipDumpId, metrics, removePrevValues, waitForTaskId)
    except:
        writeToLog("calc_metrics.html",[])
        transaction.rollback()
        raise
    return True

# A better solution for confidence intervals but works too slow
def GetBootstrapConfidenceIntervalForMean(vals, confidenceProb = 0.95, N = 1000):
    bootstrapSamples = {"mean" : []}
    for i in range(0, N):
        mean = 0
        for n in range(0, len(vals)):
            mean += vals[random.randint(0, len(vals) - 1)]
        mean /= float(len(vals))
        bootstrapSamples["mean"].append(mean)
    samplesHist = 0
    confidenceInterval = [0, 0]
    minimum = {}
    maximum = {}
    mean = {}
    std = {}
    conf = {}
    samplesHist = {}

    GetMetricsStatistics(bootstrapSamples, 1000, minimum, maximum, mean, std, samplesHist, conf, False)
    binIndex = 0
    cummulProb = 0
    for binVal in samplesHist["mean"][1]:
        cummulProb += binVal / len(bootstrapSamples)
        if ( cummulProb  > (1-confidenceProb)/2.0 ):
            confidenceInterval[0] = samplesHist["mean"][0][binIndex]
            break
        binIndex += 1
    binIndex = len(samplesHist["mean"][1]) - 1
    cummulProb = 0
    for binVal in reversed(samplesHist["mean"][1]):
        cummulProb += binVal / len(bootstrapSamples)
        if ( cummulProb  > (1-confidenceProb)/2.0 ):
            confidenceInterval[1] = samplesHist["mean"][0][binIndex]
            break
        binIndex -= 1
    return confidenceInterval

# Rough assumption: means have normal distribution
def GetTDistributionConfidenceIntervalForMean(mean, std, valsCount, confidenceProb = 0.95):
    quantile = GetTDistributionQuantile(confidenceProb)
    return [mean - quantile*std/sqrt(valsCount), mean + quantile*std/sqrt(valsCount)]

def GetMetricsStatistics(metrics, bins, minimum, maximum, mean, median, std, hist, meanConfidenceInterval, calcConfidenceForMean = True):
    infinity = 1E400
    for metricName in metrics.iterkeys():
        minimum[metricName] = infinity
        maximum[metricName] = -infinity
        median[metricName] = 0
        mean[metricName] = 0
        std[metricName] = 0
        metricValues = sorted(metrics[metricName])
        if len(metricValues) == 0:
            continue
        minimum[metricName] = metricValues[0]
        maximum[metricName] = metricValues[-1]
        median[metricName] = metricValues[len(metricValues) / 2]
        for value in metricValues:
            mean[metricName] += value
            std[metricName] += value**2
        mean[metricName] /= float(len(metrics[metricName]))
        std[metricName] /= float(len(metrics[metricName]))
        std[metricName] -= mean[metricName]**2
        std[metricName] *= (float(len(metrics[metricName]))/ float(len(metrics[metricName]) - 1)) if len(metrics[metricName]) > 1 else float(len(metrics[metricName]))
        std[metricName] = sqrt(std[metricName])
        if calcConfidenceForMean:
            meanConfidenceInterval[metricName] = GetTDistributionConfidenceIntervalForMean(mean[metricName], std[metricName], len(metrics[metricName]), 0.95)

    for metricName in metrics.iterkeys():
        delta = 1.00001*(maximum[metricName] - minimum[metricName]) / float(bins)
        delta = delta if delta != 0 else 1
        histX = [minimum[metricName]+i*delta+delta/2.0 for i in range(1, bins+1)]
        hist[metricName] = (histX, [0]*bins)
        for value in metrics[metricName]:
            hist[metricName][1][int((value - minimum[metricName]) / delta)] += 1;

def cancelTask(taskId):
    try:
        taskObj = TaskProgress.objects.get(id = taskId)
        taskObj.delete()
    except TaskProgress.DoesNotExist:
        pass

def removeFileIfExists(filePath):
    if os.path.exists(filePath):
        remove(filePath)
