#!/usr/bin/python
# -*- coding: utf-8 -*-

from __init__ import _restorePackageStructure
_restorePackageStructure()

import os
os.environ['DJANGO_SETTINGS_MODULE'] = 'settings'

import tempfile
from snipmetricsview.models import *
from django.db import transaction, connection
import _mysql_exceptions
from time import sleep
from sys import argv, exc_info, path, stderr
from datetime import datetime
from options import get_option_value
from snips_parser import SnippetsParser
from daemon import Daemon
from tempfile import mkstemp, NamedTemporaryFile
from subprocess import Popen, PIPE
from codecs import encode, decode
import codecs
from snipmetricsview.log import writeToLog
from tasksaux import calcDump


# Filter wrongly parsed snippets
def filterParsedSnip(query, url, title, snipText):
    res = snipText.find("class='g") != -1 #Fix for problems with google serp parsing, video and so other snippets parsing results ain't good
    res = res and len(title.strip()) == 0 # Fix for 'dangerous sites' parsing in Yandex site.
    return res

def getRichRequestTree(query, wizardHost):
    wizPath = get_option_value("wizardsScript")
    tempf, queryFile = tempfile.mkstemp()
    os.close(tempf)
    tempf = open(queryFile, "w")
    print >> tempf, query.encode("utf-8") + "\t" + str(get_option_value("defaultQueryRegion"))
    tempf.close()
    tempf, outputFile = tempfile.mkstemp()
    os.close(tempf)
    import subprocess
    subprocess.call("python " + wizPath + ' -s ' + wizardHost + ' -r ' +  queryFile + ' -o ' + outputFile + ' --no-fake', shell=True)
    tempf = open(outputFile, "r")
    wizOutput = tempf.readline()
    tempf.close()
    import re
    match = re.search("qtree=([^&]*)&", wizOutput)
    if not match:
        return None
    res = match.group(1)
    os.remove(queryFile)
    os.remove(outputFile)
    return res

def getAbsCwd():
    return os.path.abspath(os.getcwd()) + "/"

def startDumpTask(taskId, queryListId, newSerpName, engineUrl, searchByUrlFilter, cgiParams, wizardUrl, documentCachePath):
    p = Popen("python " + get_option_value("homeDir")  + "snipmetricsview/start_daemon.py -d %d %d '%s' '%s' '%s' '%s' '%s' '%s'" % (taskId, queryListId, newSerpName.encode("utf-8"), engineUrl.encode("utf-8"), searchByUrlFilter.encode("utf-8"), cgiParams.encode("utf-8"), wizardUrl.encode("utf-8"), documentCachePath.encode("utf-8")), shell = True, close_fds = True, cwd = getAbsCwd())

def startUploadSnippetsTask(task_id, serpName, engineUrl, cgiParams, wizardUrl, fileFormat, dataFile):
    writeToLog("In startUploadSnippetsTask task_id: %s serpName: %s" % (task_id,serpName.encode("utf-8")))
    p = Popen("python " + get_option_value("homeDir")  + "snipmetricsview/start_daemon.py -s %d '%s' '%s' '%s' '%s' %d '%s'" % (task_id, serpName, engineUrl, cgiParams, wizardUrl, fileFormat, dataFile), shell = True, close_fds = True, cwd = getAbsCwd())

def startUploadQueriesTask(task_id, queriesListName, fileName):
    args = "python " + get_option_value("homeDir")  + "snipmetricsview/start_daemon.py -l %d '%s' '%s'" % (task_id, queriesListName, fileName)
    p = Popen(args, shell = True, close_fds = True, cwd = getAbsCwd())

def startCalcTask(task_id, snipDumpId, metrics, removePrevValues, waitForTaskId):
    writeToLog("In startCalcTask task_id: %s snipDumpId: %s" % (task_id, snipDumpId))
    metrics_ids_str = u"'"
    for metric in metrics:
        metrics_ids_str += unicode(metric) + u","
    metrics_ids_str += u"'"
    Popen("python " + get_option_value("homeDir") + "snipmetricsview/start_daemon.py -m %d %d '%s' %d %d" % (task_id, int(snipDumpId), metrics_ids_str, removePrevValues, waitForTaskId), shell = True, cwd = getAbsCwd(), close_fds = True)


def rollbackDumpingChanges(dumpId, snippetIds):
    # Not implemented
    pass

def rollbackCalcChanges(metricvalues_added):
    if len(metricvalues_added) > 0:
        MetricValue.objects.filter(id__in = metricvalues_added).delete()

def checkIfCanceled(taskId):
    if len(TaskProgress.objects.filter(id = taskId)) > 0:
        return False
    return True

def formatDictionary(source_dict, nameValSeparator = ":", recordsSeparator = "|", valsSeparator = ";" ):
    res = ""
    for key in source_dict.iterkeys():
        res += encode(key,"utf-8")+ nameValSeparator
        vals = source_dict[key]
        res += valsSeparator.join([encode(x, "utf-8") for x in vals ])
        res += recordsSeparator
    return res.strip(recordsSeparator)

@transaction.commit_manually
def addQueriesList(queriesListName, data, taskId):
    writeToLog("[begin] In addQueriesList queryListName: %s" % (queriesListName.encode("utf-8")))
    try:
        ql = QueryList(name = queriesListName)
        ql.save()
        task = TaskProgress.objects.get(id = taskId)
        task.resultObjectId = ql.id
        task.save()
        for query in data:
            (queryText, region, url, characteristics, extraInfo) = query
            queryExtraInfoRecords = []
            for extraInfoType in extraInfo.iterkeys():
                ExtraInfoTypeObj = QueryExtraInfoType.objects.filter(name = extraInfoType)
                if ExtraInfoTypeObj.count() == 0:
                    ExtraInfoTypeObj = QueryExtraInfoType(name = extraInfoType)
                    ExtraInfoTypeObj.save()
                else:
                    ExtraInfoTypeObj = ExtraInfoTypeObj[0]
                for val in queryUrl[3][extraInfoType]:
                    objs = QueryExtraInfo.objects.filter(infoType = ExtraInfoTypeObj, value = val.strip())
                    if len(objs) == 0:
                        record = QueryExtraInfo(infoType = ExtraInfoTypeObj, value = val.strip())
                        record.save()
                    else:
                        record = objs[0]
                    queryExtraInfoRecords.append(record)
            characteristicsRecords = set([])
            for characteristic in characteristics:
               if characteristic not in characteristicRecords:
                  characteristic_obj = QueryCharacteristic.objects.filter(name = characteristic)
                  if characteristic_obj.count() == 0:
                      characteristic_obj = QueryCharacteristic(name = characteristic)
                      characteristic_obj.save()
                  else:
                      characteristic_obj = characteristic_obj[0]
               else:
                  characteristic_obj = characteristicObjects[characteristic]
               characteristicsRecords.add(characteristic_obj)

            queryObj = Query(text = queryText, urlToFind = url, region = region, lenInWords = len(queryText.replace("-","").replace('"',"").replace("'","").split()))
            queryObj.save()
            for extraInfo in queryExtraInfoRecords:
                queryObj.extraInfo.add(extraInfo)
            for c in characteristicsRecords:
               queryObj.characteristics.add(c)
            queryObj.save()
            ql.queries.add(queryObj)
        attempt = 0
        while attempt < 100:
            try:
                transaction.commit()
                return True
            except OperationalError, message:
                if message.find("locked") == -1:
                   break
                attemp += 1
        transaction.rollback()
        return False
    except:
        transaction.rollback()
        raise
    writeToLog("[end] In addQueriesList queryListName: %s" % (queriesListName.encode("utf-8")))

@transaction.commit_manually
def addSnippetsDump(dumpName, engineUrl, cgiParams, wizardUrl, snippets):
    writeToLog("[begin] In addSnippetDump dumpName: %s" % (dumpName))
    snipsDumpId = -1
    try:
        date = datetime.now()
        if snippets != None and len(snippets) > 0:
            try:
                if len(snippets) == 0:
                    raise Exception("Файл пуст или имеет неверный XML формат.")
                snippetsDump = SnippetsDump(name = dumpName, engineUrl = engineUrl, cgiParams = cgiParams, wizardUrl = wizardUrl, date = date)
                snippetsDump.save()
                snipsDumpId = snippetsDump.id
                counter = 0
                for snippetInfo in snippets:
                    (query, url, queryCharacteristics, queryExtraInfo, title, snippetText) = snippetInfo
                    if filterParsedSnip(query, url, title, snippetText):
                        continue
                    try:
                        max_url_len = 512
                        q = Query(text = query, urlToFind = url[:max_url_len], region = 213, lenInWords = len(query.replace("-","").replace('"',"").replace("'","").split()))
                        q.save()
                        snip = Snippet(snippetsDump = snippetsDump, query = q, url = url[:max_url_len], title = title, snippet = snippetText, extraInfo="")
                        snip.save()
                    except _mysql_exceptions.Warning: # Suppress 'truncated data' warning
                        writeToLog("Problem with snippet:\ntitle: %s\ntext: %s" % (title.encode("utf-8"), snippetText.encode("utf-8")))
                        pass
                    counter += 1
                    if counter % 100 == 0:
                        transaction.commit()
            except:
                writeToLog("Error: %s" % (str(exc_info())) )
                transaction.rollback()
                raise
            transaction.commit()
        else:
            raise Exception("Файл пуст или имеет неверный XML формат.")
    except:
        writeToLog("Error: %s" % str(exc_info()) )
        transaction.rollback()
        raise
    writeToLog("[end] In addSnippetDump dumpName: %s result snipsDumpId: %s" % (dumpName,snipsDumpId))
    return snipsDumpId

@transaction.commit_manually
def dumpSnippets(taskId, queryListId, newDumpName, engineUrl, searchByUrlFilter, cgiParams, wizardUrl, documentCachePath):
    writeToLog("[begin] dumpSnippets taskId: %i dumpName: %s" % (taskId,newDumpName))
    snippetsDumpId = -1
    addedSnippets = []
    tempf, progressFile = tempfile.mkstemp()
    errors = ""
    error_log = open(get_option_value("logDir") + "dump_err.log", "a")
    q2id = lambda x: x.strip().replace(" ","").lower()
    try:
        print >> error_log, "======== Dumping, taskId:", taskId, datetime.now(), "========"
        (outputf, outPath) = mkstemp()
        queryList = QueryList.objects.get(id = queryListId)
        queriesCount = queryList.queries.count()
        task = TaskProgress.objects.get(id = taskId)
        query2Id = {}
        dumpProcess = None
        try:
            dumpProcess = Popen(get_option_value("dumpSnippetsScript") + " -u " + engineUrl + " -c '" + documentCachePath + "' -p '" + progressFile + "' -g '" + cgiParams + "' -n '" + newDumpName + "'", shell = True, close_fds = True, cwd=get_option_value("homeDir") , stdin = PIPE, stdout = outputf, stderr = error_log)
            errors = ""
            processed = 0
            for query in queryList.queries.all():
                urlToFind = (searchByUrlFilter + query.urlToFind) if len(query.urlToFind) > 0 else ""
                queryText = query.text
                if query.urlToFind != "":
                    queryText += " " + searchByUrlFilter + query.urlToFind
                query2Id[q2id(queryText)] = query.id
                dumpProcess.stdin.write(encode("\t".join([queryText, query.region])+"\n", "utf-8"))
                processed += 1
                progress = 100 * float(processed) / (20 * queriesCount)
                task = TaskProgress.objects.get(id = taskId)
                if not task:
                    dumpProcess.stdin.close()
                    return
                task.progress = progress
                task.save()
                transaction.commit()
        except:
            errors += str(exc_info()[1]) + ". "
            writeToLog("Error: %s\n%s" % (str(exc_info()),errors) )
            task = TaskProgress.objects.get(id = taskId)
            task.errors = errors
            task.save()
            transaction.commit()
            raise
        try:
            dumpProcess.stdin.close()
            while dumpProcess.poll() == None:
                 if progressFile and os.path.exists(progressFile):
                      inp = open(progressFile, "r")
                      progressStr = inp.read().strip()
                      inp.close()
                      if progressStr != "":
                         progress = float(progressStr)
                         task = TaskProgress.objects.get(id = taskId)
                         if not task:
                             writeToLog("[abort] dumpSnippets taskId: %i dumpName: %s - task None" % (taskId,newDumpName))
                             return
                         task.progress = 5 + 80 * progress
                         task.save()
                         transaction.commit()
                 sleep(10) # Wait 10 seconds and check if dumping has finished
            dump = SnippetsDump(name = newDumpName, queryList = queryList, engineUrl = engineUrl, cgiParams = cgiParams, wizardUrl = wizardUrl, documentCacheUrlTemplate = documentCachePath, date = datetime.now())
            dump.save()
            task = TaskProgress.objects.get(id = taskId)
            task.resultObjectId = dump.id
            task.save()
            transaction.commit()
            previousQuery = ""

            dataFile = codecs.open(outPath, "r", "utf-8")
            for line in dataFile:
                try:
                    lineFields = line.strip(u"\n").split(u"\t")
                    (query, url, documentFilePath, title, snippet) = lineFields
                except ValueError:
                    print >> stderr, "Cannot parse a string (taskId " + str(taskId) + "): " + encode(line, "utf-8")
                    continue
                if not query2Id.has_key(q2id(query)):
                    print >> stderr, "Cannot find query: ", encode(query, "utf-8")
                    continue
                if searchByUrlFilter.strip() != "" and query.find(searchByUrlFilter) != -1 and query.split(searchByUrlFilter)[1] != url:
                    print stderr, "URL doesn't match url in query: ", encode(query,"utf-8"), "---", encode(url,"utf-8")
                    continue
                title = title.replace(u'"',u"'")
                snippet = snippet.replace(u'"',u"'")
                snipObj = Snippet(query_id=int(query2Id[q2id(query)]), url=url[:510],
                                  snippetsDump=dump, title=title, snippet=snippet,
                                  documentFilePath=documentFilePath)
                try:
                    snipObj.save()
                except _mysql_exceptions.Warning:
                    pass
                if query != previousQuery:
                    processed += 1
                    progress = 70 + 10 * float(processed) / queriesCount
                    task = TaskProgress.objects.get(id = taskId)
                    if not task:
                        dumpProcess.stdin.close()
                        return
                    task.progress = progress
                    task.save()
                    transaction.commit()
                previousQuery = query
            dataFile.close()
        except:
            errors += str(exc_info()[0]) + str(exc_info()[1])
            writeToLog("Error: %s\n%s" % (str(exc_info()),errors) )
            task = TaskProgress.objects.get(id = taskId)
            task.errors = errors
            task.save()
            transaction.commit()
            raise

        if dumpProcess.returncode != 0:
            errors += u"Ошибка сбора сниппетов (возможно, сервер не отвечает)"

        task = TaskProgress.objects.get(id = taskId)
        task.errors = errors
        task.progress = 100.0
        task.save()
        transaction.commit()
        os.remove(outPath)
        if os.path.exists(progressFile):
            os.remove(progressFile)
    except:
        writeToLog("Error: %s\n%s" % (str(exc_info()),errors) )
        transaction.rollback()
        task = TaskProgress.objects.get(id = taskId)
        task.progress = 100.0
        task.save()
        transaction.commit()
        raise
    error_log.close()
    writeToLog("[end]dumpSnippets taskId: %i dumpName: %s" % (taskId,newDumpName))

@transaction.commit_manually
def calc_metrics(taskId, snipDumpId, metrics, removePrevValues = False, waitForTaskId = -1):
    writeToLog("[begin] calc_metrics taskId: %s dumpId: %s waitForTaskId: %s" % (taskId,snipDumpId,waitForTaskId))
    try:
        task = TaskProgress.objects.get(id = taskId)
        details = task.details

        # Wait for task if needed
        if waitForTaskId != -1:
            finished = False
            while not finished:
                wait_task = TaskProgress.objects.filter(id = waitForTaskId)
                if wait_task.count() == 0:
                    writeToLog("[after wait] task %s  count is 0" % (taskId))
                    return
                if wait_task[0].progress == 100.0 and wait_task[0].resultObjectId:
                    finished = True
                    snipDumpId = wait_task[0].resultObjectId
                    writeToLog("[after wait] calc_metrics taskId: %s dumpId: %s" % (taskId,snipDumpId))
                sleep(10)

        task.details = details + " ..prepare calculator & metrics list"
        task.save()

        wizardUrl = SnippetsDump.objects.get(id = snipDumpId).wizardUrl
        metricvalues_added = []
        error_log = open(get_option_value("logDir") + "calc_err.log", "a")
        errors = ""
        # Get calculators : BEGIN
        calculators = set([])
        metricsIdByName = {}
        metricIds = set([])
        for metric in metrics:
            metric = Metric.objects.get(id = metric)
            metricsIdByName[metric.shortName] = metric.id
            metricIds.add(metric.id)
            calculators.add((metric.calculator.exePath, metric.calculator.commandLineParams))
        outFiles = []
        for i in range(0, len(calculators)):
            outFiles.append(mkstemp())
        # Get calculators : END

        progress = 0
        (outputf, outPath) = mkstemp()
        calcProcesses = []
        index = 0
        try:
            for calc in calculators:
                args = get_option_value("scriptsHomeDir") + "/".join(calc[0].split("/")[-2:]) + " " + calc[1]
                writeToLog("[subprocess] calc_metrics taskId: %s dumpId: %s\n%s" % (taskId, snipDumpId, args))
                calcProcesses.append(Popen(args, bufsize = 1, shell = True, close_fds = True, cwd=get_option_value("homeDir") , stdin = PIPE, stdout = outFiles[index][0], stderr = error_log))
                index += 1
        except:
            errors += str(exc_info()[1]) + ". "
            writeToLog("Error: %s\n%s" % (str(exc_info()),errors) )
            task.errors = errors
            task.progress = progress
            task.save()
            transaction.commit()

        snippets_count = Snippet.objects.filter(snippetsDump = int(snipDumpId)).count()
        task.details = details + " ..calculating %s snippets" % snippets_count
        task.save()
        try:
            lastQuery = ""
            richTree = ""
            progress = 0
            cursor = connection.cursor()

            if removePrevValues:
                sql = """ delete mv from snipmetricsview_metricvalue mv
                            inner JOIN snipmetricsview_snippet s
                            on mv.snippet_id  = s.id where s.snippetsDump_id = %s;
                        """ % snipDumpId
                cursor.execute(sql)
            sql = """ select s.id, q.text, s.url, s.documentFilePath, s.title, s.snippet
                        from snipmetricsview_snippet s, snipmetricsview_query q
                        where s.snippetsDump_id = %s and s.query_id = q.id;
                    """ % snipDumpId
            cursor.execute(sql)
            res = cursor.fetchone()
            while res:
                if checkIfCanceled(taskId):
                    return
                res = list(res)
                if lastQuery != res[1]:
                    richTree = getRichRequestTree(res[1], wizardUrl)
                    lastQuery = res[1]
                if richTree:
                    res.insert(2, richTree)
                    for calc in calcProcesses:
                        str2print = str(res[0])+"\t"+encode("\t".join(res[1:]) +"\t \n", "utf-8")
                        calc.stdin.write(str2print)
                        res[5] = ""  #Calc metrics without title
                        str2print = str(res[0])+"\t"+encode("\t".join(res[1:]) +"\t \n", "utf-8")
                        calc.stdin.write(str2print)
                progress += 1
                if progress % 100 == 0:
                    task.progress = 90.0 * progress/ snippets_count
                    task.save()
                    transaction.commit()
                res = cursor.fetchone()
            task.details = details+ " ..end calcalators"
            task.save()
            for calc in calcProcesses:
                calc.stdin.write("\n\n")
                calc.wait()
        except:
            errors += str(exc_info()[1]) + ". "
            writeToLog("Error: %s\n%s" % (str(exc_info()),errors) )
            task.errors = errors
            task.progress = 100.0
            task.save()
            transaction.commit()
            raise

        if checkIfCanceled(taskId):
            return  # just go away, skip transaction

        sql ="insert into snipmetricsview_metricvalue (metric_id, snippet_id, value, is_with_title) values (%(mid)s, %(sid)s, %(val)s, %(wt)s);"
        task.details = details + " ..copying info from temp out files to DB"
        task.save()
        metricvalues_added = []
        # Start copying info from temp out files to DB
        for tempFile in outFiles:
            inp = open(tempFile[1], "r")
            fsize = os.path.getsize(tempFile[1])
            lastSnipId = -1
            counter = 0
            for line in inp:
                try:
                    (snipId, query, url, metrics) = line.strip(" \n\r").split("\t")
                    for metric in metrics.split(" "):
                        metric = metric.strip().split(":")
                        if len(metric) != 2:
                            print >> error_log, "Error parsing metric: ", metric
                            continue
                        (metricName, metricValue) = metric
                        if not metricsIdByName.has_key(metricName):
                            continue
                        is_with_title = "1" if lastSnipId != snipId else "0"
                        metricvalues_added.append( {"mid":metricsIdByName[metricName], "sid":snipId, "val":metricValue, "wt":is_with_title} )
                        counter += 1
                        if counter % 1000 == 0:
                            cursor.executemany(sql, metricvalues_added)
                            metricvalues_added = []
                            task.progress = 90 + 10.0 * inp.tell() / fsize
                            task.save()
                    lastSnipId = snipId
                except ValueError:
                    writeToLog("Error: %s\nLine: %s\n%s" % (str(exc_info()), line, errors) )
                    continue
                except:
                    errors += str(exc_info()[1]) + ". "
                    writeToLog("Error: %s\nLine: %s\n%s" % (str(exc_info()), line, errors) )
                    task.errors = errors
                    task.progress = progress
                    task.save()
                    raise
            os.remove(tempFile[1])
        if metricvalues_added:
            cursor.executemany(sql, metricvalues_added)

        if checkIfCanceled(taskId):
            return  # just go away, skip transaction

        task.errors = errors
        task.progress = 100.0
        task.save()
        transaction.commit()
    except:
        writeToLog("Error: %s\n%s" % (str(exc_info()), errors))
        transaction.rollback()
        raise
    writeToLog("[end] calc_metrics taskId: %s dumpId: %s" % (taskId,snipDumpId))

def updateProgressCallback(taskId, progress):
    task = TaskProgress.objects.get(id = taskId)
    task.progress = progress
    task.save()

def uploadQueries(taskId, queriesListName, fileName):
    parser = SnippetsParser(fileName, SnippetsParser.ParserFileFormats["TabSeparated"], taskId, updateProgressCallback)
    addQueriesList(queriesListName.decode("utf-8"), parser.ParseQueriesFile(), taskId)
    task = TaskProgress.objects.get(id = taskId)
    task.progress = 100
    task.save()
    os.remove(fileName)

def uploadSnippets(taskId, dumpName, engineUrl, cgiParams, wizardUrl, fileFormat, dataFile):
    writeToLog("[begin] UploadSnippets taskId: %i dumpName: %s fileFormat: %s file: %s" % (taskId,dumpName,fileFormat, dataFile))
    dumpId = -1
    errors = ""
    try:
        parser = SnippetsParser(dataFile, fileFormat, taskId, updateProgressCallback)
        rawData = parser.ParseSnippets()
        while dumpId == -1:
            dumpId = addSnippetsDump(dumpName, engineUrl, cgiParams, wizardUrl, rawData)
    except:
        errors = str(exc_info()[1]).replace('"'," ") + ". "
        writeToLog("Error: %s" % (str(exc_info())) )
    task = TaskProgress.objects.get(id = taskId)
    task.progress = 100
    task.errors = errors
    task.resultObjectId = dumpId
    task.save()
    os.remove(dataFile)
    writeToLog("[end] UploadSnippets taskId: %i dumpName: %s fileFormat: %s dumpId: %s" % (taskId,dumpName,fileFormat,dumpId))

if __name__ == "__main__":
    if argv[1] == "-d":
        dumpSnippets(int(argv[2]), int(argv[3]), argv[4], argv[5], argv[6], argv[7], argv[8], argv[9])
    elif argv[1] == "-m":
        metric_ids = [int(x) for x in argv[4].strip("',").split(",")]
        calc_metrics(int(argv[2]), argv[3], metric_ids, int(argv[5]), int(argv[6]))
    elif argv[1] == "-c":
        collectingDocs(int(argv[2]), int(argv[3]))
    elif argv[1] == "-l":
        uploadQueries(int(argv[2]), argv[3], argv[4])
    elif argv[1] == "-s":
        uploadSnippets(int(argv[2]), argv[3], argv[4], argv[5], argv[6], int(argv[7]), argv[8])
    elif argv[1] == "-calc_dump":
        calcDump(int(argv[2]))
    else:
        getRichRequestTree("yandex google")
    exit()

