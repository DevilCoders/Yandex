#!/usr/bin/python
# -*- coding: utf-8 -*-

import forms
from copy import copy
import sys
import traceback
import random
from django.shortcuts import render_to_response, redirect, get_object_or_404, get_list_or_404
from django.http import HttpResponseRedirect, HttpResponse
from django.template import RequestContext
from snipmetricsview.options import get_option_value, ROOT_URL
from snipmetricsview.models import *
from urlparse import urlsplit
from snipmetricsview.utils import *
from snipmetricsview.tasks import getRichRequestTree
from snipmetricsview.tasksaux import wizName
from snipmetricsview.cache import MetricsCache, StatsTypes
from xml.parsers import expat
from datetime import datetime

INF = 1E400

# Handler

def clear_cache_view(request):
    MetricsCache.clear()
    return redirect(ROOT_URL + 'view_metrics.html')

def short_url_view(request, shortUrl):
    shortUrl = ShortUrl.objects.get(shortUrl = shortUrl)
    return redirect(shortUrl.fullUrl)

def index_html(request):
#    prepareDB()
    context = {}
    errors = []
    try:
        context["snippetsDumps"] = SnippetsDump.objects.all()
    except:
        errors.append(get_option_value("defaultCriticalErrorMessage"))
        reqVars = copy(request.GET)
        reqVars.update(request.POST)
        writeToLog(request.path, reqVars)
    context["errors"] = errors
    return render_to_response("index.html", context, context_instance=RequestContext(request))

def add_engine_html(request):
    errors = []
    try:
        engineForm = forms.EngineForm()
        if request.method == "POST":
            engineForm = forms.EngineForm(request.POST)
            if engineForm.is_valid():
                try:
                    if engineForm.cleaned_data["id"] != "":
                       edit_engine(id = int(engineForm.cleaned_data["id"]), name = engineForm.cleaned_data["name"], desc = engineForm.cleaned_data["description"], url = engineForm.cleaned_data["url"], searchByUrlFilter = engineForm.cleaned_data["searchByUrlFilter"], cgi = engineForm.cleaned_data["cgiParams"], wizardUrl = engineForm.cleaned_data["wizardUrl"], documentCacheUrlTemplate = engineForm.cleaned_data["documentCacheUrlTemplate"])
                    else:
                       add_engine(name = engineForm.cleaned_data["name"], desc = engineForm.cleaned_data["description"], url = engineForm.cleaned_data["url"], searchByUrlFilter = engineForm.cleaned_data["searchByUrlFilter"], cgi = engineForm.cleaned_data["cgiParams"], wizardUrl = engineForm.cleaned_data["wizardUrl"], documentCacheUrlTemplate = engineForm.cleaned_data["documentCacheUrlTemplate"])
                    return HttpResponseRedirect(ROOT_URL + "engines.html")
                except MetricsException:
                    errors.append(sys.exc_info()[1])

        current_engine = None
        if int(request.GET.get("engine_id", -1)) != -1:
            current_engine = SnippetsEngine.objects.get(id = int(request.GET.get("engine_id")))

        engineForm.fields["searchByUrlFilter"].initial = "url:"
        if current_engine != None:
           engineForm.fields["id"].initial = current_engine.id
           engineForm.fields["name"].initial = current_engine.name
           engineForm.fields["description"].initial = current_engine.description
           engineForm.fields["wizardUrl"].initial = current_engine.wizardUrl
           engineForm.fields["url"].initial = current_engine.url
           engineForm.fields["searchByUrlFilter"].initial = current_engine.searchByUrlFilter
           engineForm.fields["cgiParams"].initial = current_engine.cgiParams
           engineForm.fields["documentCacheUrlTemplate"].initial = current_engine.documentCacheUrlTemplate
    except:
        errors.append(get_option_value("defaultCriticalErrorMessage"))
        reqVars = copy(request.GET)
        reqVars.update(request.POST)
        writeToLog(request.path, reqVars)

    context = { "engineForm" : engineForm, "errors":errors }
    return render_to_response("add_engine.html", context, context_instance=RequestContext(request))

def queries_html(request):
    errors = []
    viewQueryListId = int(request.GET.get("id", -1))
    try:
        delId = int(request.GET.get("del_id", -1))
        if delId != -1:
            queryList2Del = QueryList.objects.get(id = delId)
            for q in queryList2Del.queries.all():
                if q.querylist_set.count() == 1: #delete only if this query belongs to one query list
                    q.delete()
            queryList2Del.delete()
            return HttpResponseRedirect(ROOT_URL + "queries.html")
        queryLists = QueryList.objects.all()
    except:
        errors.append(get_option_value("defaultCriticalErrorMessage"))
        reqVars = copy(request.GET)
        reqVars.update(request.POST)
        writeToLog(request.path, reqVars)
    context = { "queryLists": queryLists, "errors" : errors, "viewQueryListId" : viewQueryListId }
    if viewQueryListId != -1:
        from django.core.paginator import Paginator, PageNotAnInteger, EmptyPage
        paginator = Paginator(QueryList.objects.get(id = viewQueryListId).queries.all(), 100)
        page = int(request.GET.get("page", 1))
        try:
            queries = paginator.page(page)
        except PageNotAnInteger:
            # If page is not an integer, deliver first page.
            queries = paginator.page(1)
        except EmptyPage:
            # If page is out of range (e.g. 9999), deliver last page of results.
            queries = paginator.page(paginator.num_pages)
        context["queries"] = queries
    return render_to_response("queries.html", context, context_instance=RequestContext(request))

def queries_txt(request):
    errors = []
    viewQueryListId = int(request.GET.get("id", -1))
    if viewQueryListId == -1:
        return HttpResponseRedirect(ROOT_URL + "queries.html")
    queryList = get_object_or_404(QueryList, id = viewQueryListId)
    return render_to_response("queries.txt", { "queryList": queryList }, context_instance=RequestContext(request), mimetype = 'plain/text')

def snippets_txt(request):
    dumpId = int(request.GET.get("id", -1))
    if dumpId == -1:
        return HttpResponseRedirect(ROOT_URL + "snippets.html")
    snippets = get_list_or_404(Snippet, snippetsDump = dumpId)
    return render_to_response("snippets.txt", { "snippets": snippets }, context_instance=RequestContext(request), mimetype = 'plain/text')

def snips_info(request):
    saveId = int(request.GET.get("saveId", -1))
    if saveId>=0:
        dump = SnipDump.objects.get(id = saveId)
        if dump.fileName:
            writeToLog("[download request] %s" % dump.fileName.name.encode("utf8"))
            file_data = open(dump.fileName.path, "rb").read()
            response = HttpResponse(file_data, mimetype="plain/text")
            response['Content-Disposition'] = 'attachment; filename='+dump.fileName.path.split('/')[-1]
            return response

    viewId = int(request.GET.get("viewId", -1))
    if viewId >=0:
        dump = SnipDump.objects.get(id = viewId)
        if dump.fileRes:
            writeToLog("[results view] %s" % dump.fileRes.name.encode("utf8"))
            html = open(dump.fileRes.path, "rb").read()
            return HttpResponse(html)

    calcId = int(request.GET.get("calcId", -1))
    if calcId>=0:
        startCalcDump(calcId)
        return HttpResponseRedirect(ROOT_URL + "tasks.html")

    delId = int(request.GET.get("delId", -1))
    if delId>= 0:
        delDump = SnipDump.objects.get(id = delId)
        if delDump.fileName:
            w = wizName(delDump.fileName.path)
            if os.path.exists(w):
                os.remove(w)
            delDump.fileName.delete()
        if delDump.fileRes:
            delDump.fileRes.delete()
        delDump.delete()

    return HttpResponseRedirect(ROOT_URL + "snippets.html")


def add_queries_html(request):
    errors = []
    try:
        if request.method == "POST":
            queriesForm = forms.QueriesForm(request.POST)
            if queriesForm.is_valid():
                try:
                    if request.FILES.has_key("queriesFile"):
                        dataFileName = flush_uploaded_to_temp(request.FILES["queriesFile"])
                    else:
                        data = queriesForm.cleaned_data["queries"].replace("#","\t")
                        dataFileName, fileObj = getRandomFileName()
                        dataFile = open(dataFileName, "w")
                        print >> dataFile, encode(data, "utf8")
                        dataFile.close()
                    addQueriesListFromFile(queriesForm.cleaned_data["name"], dataFileName.decode("utf-8"))
                    return HttpResponseRedirect(ROOT_URL + "tasks.html")
                except:
                    errors.append("Неверный формат файла.")
                    reqVars = copy(request.GET)
                    reqVars.update(request.POST)
                    writeToLog(request.path, reqVars)
        else:
            queriesForm = forms.QueriesForm()
    except:
        errors.append(get_option_value("defaultCriticalErrorMessage"))
        reqVars = copy(request.GET)
        reqVars.update(request.POST)
        writeToLog(request.path, reqVars)
    context = { "queriesForm" : queriesForm, "errors" : errors }
    return render_to_response("add_queries.html", context, context_instance=RequestContext(request))

def engines_html(request):
    errors = []
    engines = None
    try:
        engine2Del = int(request.GET.get("engine_id", -1))
        if engine2Del >= 0:
            obj2Del = SnippetsEngine.objects.get(id = engine2Del)
            obj2Del.delete()
            return HttpResponseRedirect(ROOT_URL+"engines.html")
        engines = SnippetsEngine.objects.all()
    except:
        errors.append(get_option_value("defaultCriticalErrorMessage"))
        reqVars = copy(request.GET)
        reqVars.update(request.POST)
        writeToLog(request.path, reqVars)

    context = { "engines" : engines, "errors" : errors }
    return render_to_response("engines.html", context, context_instance=RequestContext(request))

def metrics_html(request):
    errors = []
    try:
        delId = int(request.GET.get("delId", -1))
        if delId >= 0:
            metric = Metric.objects.get(id = delId)
            metric.delete()
            return HttpResponseRedirect(ROOT_URL+"metrics.html")
        metrics = Metric.objects.all()
    except:
        errors.append(get_option_value("defaultCriticalErrorMessage"))
        reqVars = copy(request.GET)
        reqVars.update(request.POST)
        writeToLog(request.path, reqVars)
    context = { "metrics" : metrics, "errors" : errors }
    return render_to_response("metrics.html", context, context_instance=RequestContext(request))

def add_metric_html(request):
    errors = []
    context = {}
    try:
        metricForm = forms.MetricForm()
        del metricForm.fields["categories"].choices[:]
        for category in MetricCategory.objects.all():
            metricForm.fields["categories"].choices.append((category.name, category.name))
        del metricForm.fields["calculatorExePath"].choices[:]
        for calc in MetricCalculator.objects.all():
            metricForm.fields["calculatorExePath"].choices.append((calc.exePath, calc.exePath.split("/")[-1]))
        if request.method == "POST":
            metricForm = forms.MetricForm(request.POST)
            if metricForm.is_valid():
                try:
                    editMetricId = int(request.POST.get("editMetricId", -1))
                    uploadedFile = request.FILES["calculatorExeFile"] if request.FILES.has_key("calculatorExeFile") else None
                    add_metric(metricForm.cleaned_data["name"], metricForm.cleaned_data["shortName"], metricForm.cleaned_data["description"], metricForm.cleaned_data["categories"], metricForm.cleaned_data["calculatorExePath"], uploadedFile, metricForm.cleaned_data["commandLineParams"], editMetricId)
                    return HttpResponseRedirect(ROOT_URL + "metrics.html")
                except MetricsException:
                    errors.append(sys.exc_info()[1])
        editMetricId = int(request.GET.get("metric_id", -1))
        if editMetricId >= 0:
            metric = Metric.objects.get(id = editMetricId)
            fillMetricEditForm(metricForm, metric)
            context["editMetricId"] = editMetricId
    except:
        errors.append(get_option_value("defaultCriticalErrorMessage"))
        reqVars = copy(request.GET)
        reqVars.update(request.POST)
        writeToLog(request.path, reqVars)
    context["metricForm"] = metricForm
    context["errors"] = errors
    return render_to_response("add_metric.html", context, context_instance=RequestContext(request))

def add_metric_category_html(request):
    errors = []
    metricCategories = []
    try:
        metricCategoryForm = forms.MetricCategoryForm()
        if request.method == "POST":
            metricCategoryForm = forms.MetricCategoryForm(request.POST)
            if metricCategoryForm.is_valid():
                metricCategoryForm.save()
                return HttpResponseRedirect(ROOT_URL+"add_metric.html")

        metricCategories = MetricCategory.objects.all()
    except:
        errors.append(get_option_value("defaultCriticalErrorMessage"))
        reqVars = copy(request.GET)
        reqVars.update(request.POST)
        writeToLog(request.path, reqVars)
    context = { "metricCategoryForm" : metricCategoryForm, "metricCategories" : metricCategories, "errors" : errors }
    return render_to_response("add_metric_category.html", context, context_instance=RequestContext(request))

def downloadResponse(snipDumps, withTitle, metricsToView, allowedWordsCount):
    from StringIO import StringIO
    from django.db import connection
    log = StringIO()
    cursor = connection.cursor()
    for dumpId in snipDumps:
        writeToLog("SQL request for dump: %s" % (dumpId))
        query = '''
            SELECT q.text, s.url, s.title, s.snippet, GROUP_CONCAT(CONCAT_WS(':', m.shortName, mv.value) SEPARATOR ' ')
            FROM
                snipmetricsview_metricvalue mv,
                snipmetricsview_snippet s,
                snipmetricsview_snippetsdump sd,
                snipmetricsview_metric m,
                snipmetricsview_query q,
                snipmetricsview_metriccalculator c
            WHERE
                sd.id = s.snippetsDump_id AND
                s.snippetsDump_id = %s AND
                mv.is_with_title = %s AND
                mv.metric_id IN (%s) AND
                mv.snippet_id = s.id AND
                m.calculator_id = c.id AND
                m.id = mv.metric_id AND
                s.query_id = q.id AND
                q.lenInWords IN (%s)
            GROUP BY mv.snippet_id
        ''' % (dumpId, withTitle, ', '.join(map(str, metricsToView)), ', '.join(map(str,allowedWordsCount)))
        writeToLog("SQL request for dump: %s%s" % (dumpId, query))
        cursor.execute(query)
        res = cursor.fetchone()
        while res:
            log.write("%s\n" % '\t'.join([v for v in res]))
            res = cursor.fetchone()

    filename = '%s_%stitle.txt' % ('_'.join(d.name for d in SnippetsDump.objects.filter(id__in = snipDumps)), '' if withTitle else 'no')
    writeToLog("Send file: %s" % filename)
    response = HttpResponse(log.getvalue(), mimetype="plain/text")
    response['Content-Disposition'] = 'attachment; filename="%s"' % filename
    return response

def view_metrics_html(request):
    errors = []
    context = {}
    try:
        getParamsDict = {"metrics" : request.GET.getlist("metrics"), "snipDumps" : request.GET.getlist("snipDumps"), "queryCharacteristics" : request.GET.getlist("queryCharacteristics"), "queryWordsCount" : request.GET.getlist("queryWordsCount"),  "notCalcConfInt" : request.GET.get("notCalcConfInt"), "withTitle" : request.GET.get("withTitle"), "sameScale" : request.GET.get("sameScale"), "sameQueryUrls" : request.GET.get("sameQueryUrls", False)}
        cache = MetricsCache(getParamsDict)

        viewMetricsForm = forms.ViewMetricsForm(getParamsDict)
        fillSnipDumpsList(viewMetricsForm.fields["snipDumps"].choices, True)
        fillQueryCharacteristicList(viewMetricsForm.fields["queryCharacteristics"].choices)
        fillQueryWordsCountList(viewMetricsForm.fields["queryWordsCount"].choices)
        fillMetricsTree(viewMetricsForm.fields["metrics"].choices, getParamsDict["metrics"])
        sameQueryUrls = getParamsDict["sameQueryUrls"]
        context["viewMetricsForm"] = viewMetricsForm
        if not viewMetricsForm.is_valid():
            errors.append("The data in the form are invalid.")
        else:
            snipDumps = viewMetricsForm.cleaned_data["snipDumps"]
            if len(snipDumps) >= 0:
                metricsToView = set([])
                # fill metrics to display list
                for id in viewMetricsForm.cleaned_data["metrics"]:
                    if id.startswith("met"):
                        metric = id.strip("met")
                        metricsToView.add(int(metric))
                queryCharacteristics = viewMetricsForm.cleaned_data["queryCharacteristics"]
                queryWordsCount = viewMetricsForm.cleaned_data["queryWordsCount"]
                sameScale = viewMetricsForm.cleaned_data["sameScale"]
                # fill word counts
                allowedWordsCount = set([]) if len(queryWordsCount) > 0 else set(range(1,255))
                for qwc in queryWordsCount:
                    if qwc == ">6":
                        for i in range(7,255):
                            allowedWordsCount.add(i)
                    else:
                        allowedWordsCount.add(int(qwc))

                snippetsByDump = {}
                queriesByDump = {}
                context["metricsNames"] = {}
                metricsNames = {}
                context["snippetsDumps"] = []
                context["metricsMeanConfidence"] = {}
                context["metricsMinimums"] = {}
                context["metricsMaximums"] = {}
                context["metricsMean"] = {}
                context["metricsMedian"] = {}
                context["metricsStd"] = {}
                context["metricsHistograms"] = {}
                context["barWidthes"] = {}
                context["metricsScaleMin"] = {}
                context["metricsScaleMax"] = {}
                context["metricsScaleYMax"] = {}

                if request.GET.has_key("view"):
                    sameUrls = []
                    sameQueries = []
                    if sameQueryUrls:
                        queryUrls = set([])
                        i = 0
                        for dumpId in snipDumps:
                            queryUrlsSet = set([])
                            for snip in Snippet.objects.filter(snippetsDump__id = dumpId):
                                queryUrlsSet.add((snip.query.text, snip.url))

                            queryUrls = queryUrlsSet if i == 0 else queryUrls & queryUrlsSet
                            i += 1
                        for qu in queryUrls:
                            sameQueries.append(qu[0])
                            sameUrls.append(qu[1])

                    # Main loop
                    for dumpId in snipDumps:
                        dumpId = int(dumpId)
                        metrics = {}
                        metricsIdByName = {}
                        metricsMin = {}
                        metricsMax = {}
                        metricsMean = {}
                        metricsMedian = {}
                        metricsStd = {}
                        metricsMeanConfidenceInterval = {}
                        metricsHistograms = {}
                        maxBinVal = {}

                        hasMetricsValues = False

                        metricsNotInCache = set([])
                        for metric in metricsToView:
                           if not cache.isInCache(dumpId, metric) or sameQueryUrls:
                                metricsNotInCache.add(metric)
                           elif cache.getValue(dumpId, metric, StatsTypes["meanStatsType"]) != None:
                                metricsMean[metric] = cache.getValue(dumpId, metric, StatsTypes["meanStatsType"])
                                metricsMedian[metric] = cache.getValue(dumpId, metric, StatsTypes["medianStatsType"])
                                metricsMin[metric] = cache.getValue(dumpId, metric, StatsTypes["minStatsType"])
                                metricsMax[metric] = cache.getValue(dumpId, metric, StatsTypes["maxStatsType"])
                                metricsMeanConfidenceInterval[metric] = cache.getValue(dumpId, metric, StatsTypes["confIntStatsType"])
                                metricsStd[metric] = cache.getValue(dumpId, metric, StatsTypes["stdStatsType"])
                                metricsHistograms[metric] = cache.getValue(dumpId, metric, StatsTypes["histStatsType"])
                        if len(metricsNotInCache) > 0:
                            snipsCount = 0
                            if sameQueryUrls:
                                metricValuesRecords = MetricValue.objects.filter(snippet__snippetsDump = dumpId, snippet__query__text__in = sameQueries, snippet__url__in = sameUrls, snippet__query__lenInWords__in = allowedWordsCount, is_with_title = viewMetricsForm.cleaned_data["withTitle"], metric__in = metricsNotInCache).only("metric_id", "value").values()
                            else:
                                metricValuesRecords = MetricValue.objects.filter(snippet__snippetsDump = dumpId, snippet__query__lenInWords__in = allowedWordsCount, is_with_title = viewMetricsForm.cleaned_data["withTitle"], metric__in = metricsNotInCache).only("metric_id", "value").values()
                            for metricVal in metricValuesRecords:
                                if not metrics.has_key(metricVal["metric_id"]):
                                    metrics[metricVal["metric_id"]] = []
                                metrics[metricVal["metric_id"]].append(metricVal["value"])
                                hasMetricsValues = True
                        # Get metrics statistics
                        GetMetricsStatistics(metrics, 20, metricsMin, metricsMax, metricsMean, metricsMedian, metricsStd, metricsHistograms, metricsMeanConfidenceInterval)
                        if not sameQueryUrls:
                            for metric in metricsNotInCache:
                                if metrics.has_key(metric):
                                    cache.setValue(dumpId, metric, StatsTypes["meanStatsType"], metricsMean[metric])
                                    cache.setValue(dumpId, metric, StatsTypes["medianStatsType"], metricsMedian[metric])
                                    cache.setValue(dumpId, metric, StatsTypes["minStatsType"], metricsMin[metric])
                                    cache.setValue(dumpId, metric, StatsTypes["maxStatsType"], metricsMax[metric])
                                    cache.setValue(dumpId, metric, StatsTypes["confIntStatsType"], metricsMeanConfidenceInterval[metric])
                                    cache.setValue(dumpId, metric, StatsTypes["stdStatsType"], metricsStd[metric])
                                    cache.setValue(dumpId, metric, StatsTypes["histStatsType"], metricsHistograms[metric])
                                else:
                                    cache.setValue(dumpId, metric, StatsTypes["meanStatsType"], None)
                                    cache.setValue(dumpId, metric, StatsTypes["medianStatsType"], None)
                                    cache.setValue(dumpId, metric, StatsTypes["minStatsType"], None)
                                    cache.setValue(dumpId, metric, StatsTypes["maxStatsType"], None)
                                    cache.setValue(dumpId, metric, StatsTypes["confIntStatsType"], None)
                                    cache.setValue(dumpId, metric, StatsTypes["stdStatsType"], None)
                                    cache.setValue(dumpId, metric, StatsTypes["histStatsType"], None)

                        context["metricsMean"][dumpId] = metricsMean
                        context["metricsMedian"][dumpId] = metricsMedian
                        context["snippetsDumps"].append(SnippetsDump.objects.get(id = dumpId))
                        context["metricsMinimums"][dumpId] = metricsMin
                        context["metricsMaximums"][dumpId] = metricsMax
                        # Iterate over existing metrics only. Some metrics from metricsToView can be missing.
                        for metric in metricsMean.iterkeys():
                            if not metricsNames.has_key(metric):
                                met = Metric.objects.get(id = metric)
                                metricsNames[metric] = (met.name, met.description)
                            if not context["metricsScaleMin"].has_key(metric):
                                context["metricsScaleMin"][metric] = INF
                            if metricsMin[metric] < context["metricsScaleMin"][metric]:
                                context["metricsScaleMin"][metric] = metricsMin[metric]
                            if not context["metricsScaleMax"].has_key(metric):
                                context["metricsScaleMax"][metric] = -INF
                            if metricsMax[metric] > context["metricsScaleMax"][metric]:
                                context["metricsScaleMax"][metric] = metricsMax[metric]
                        context["metricsMeanConfidence"][dumpId] = metricsMeanConfidenceInterval
                        context["metricsStd"][dumpId] = metricsStd
                        context["metricsHistograms"][dumpId] = metricsHistograms
                        for metric in metricsHistograms.iterkeys():
                            if not context["metricsScaleYMax"].has_key(metric):
                                context["metricsScaleYMax"][metric] = -INF
                            for val in metricsHistograms[metric][1]:
                                if context["metricsScaleYMax"][metric] < val:
                                    context["metricsScaleYMax"][metric] = val
                        barWidthes = {}
                        for metricName in metricsHistograms.iterkeys():
                            if metricName not in barWidthes:
                                barWidthes[metricName] = {}
                            barWidthes[metricName] = abs(metricsHistograms[metricName][0][1] -  metricsHistograms[metricName][0][0])
                        context["barWidthes"][dumpId] = barWidthes

                    currentUrl = ROOT_URL.strip("/") + request.META['PATH_INFO'] + "?" + request.META['QUERY_STRING']
                    shortUrl = ShortUrl.objects.filter(fullUrl = currentUrl).all()
                    if shortUrl.count() == 0:
                        fail = True
                        # generate while non-unique
                        while fail:
                            shortUrlStr = generateShortUrl()
                            shortUrl = ShortUrl.objects.filter(shortUrl = shortUrlStr)
                            fail = shortUrl.count() != 0
                        shortUrl = ShortUrl(shortUrl = shortUrlStr, fullUrl = currentUrl)
                        shortUrl.save()
                    else:
                        shortUrl = shortUrl[0]
                    generateShortUrl()
                    context["metricsCount"] = len(metricsNames)
                    context["metricsNames"] = metricsNames
                    context["shortUrl"] = ROOT_URL + 'su/' + shortUrl.shortUrl
                    context["sameScale"] = sameScale
                    context["showMetrics"] = True
                elif request.GET.has_key("download"):
                    return downloadResponse(snipDumps, viewMetricsForm.cleaned_data["withTitle"], metricsToView, allowedWordsCount)
    except:
        errors.append(get_option_value("defaultCriticalErrorMessage"))
        reqVars = copy(request.GET)
        reqVars.update(request.POST)
        writeToLog(request.path, reqVars)
        raise
    context["errors"] =  errors
    return render_to_response("view_metrics.html", context, context_instance=RequestContext(request))

def snips_dump_html(request):
    errors = []
    snippetsDumpForm = forms.SnippetsDumpForm()
    try:
        del snippetsDumpForm.fields["engine"].choices[:]
        snippetsDumpForm.fields["engine"].choices.append(("-", "Задать вручную"))
        for engine in SnippetsEngine.objects.all():
            snippetsDumpForm.fields["engine"].choices.append((engine.id, engine.name))
        del snippetsDumpForm.fields["queries"].choices[:]
        for queryList in QueryList.objects.all():
            snippetsDumpForm.fields["queries"].choices.append((queryList.id, queryList.name))
        if request.method == "POST":
            snippetsDumpForm = forms.SnippetsDumpForm(request.POST)
            if snippetsDumpForm.is_valid():
                try:
                    newSerpName = snippetsDumpForm.cleaned_data["newSerpName"]
                    engineId = snippetsDumpForm.cleaned_data["engine"]
                    if engineId == "-":
                        engineUrl = snippetsDumpForm.cleaned_data["engineUrl"]
                        searchByUrlFilter = snippetsDumpForm.cleaned_data["searchByUrlFilter"]
                        cgiParams = snippetsDumpForm.cleaned_data["cgiParams"]
                        wizardUrl = snippetsDumpForm.cleaned_data["wizardUrl"]
                        documentCachePath = snippetsDumpForm.cleaned_data["documentCacheUrlTemplate"]
                    else:
                        engine = SnippetsEngine.objects.get(id = int(engineId))
                        engineUrl = engine.url
                        searchByUrlFilter = engine.searchByUrlFilter
                        cgiParams = engine.cgiParams
                        wizardUrl = engine.wizardUrl
                        documentCachePath = engine.documentCacheUrlTemplate
                    if len(engineUrl) == 0:
                       errors.append("Поле URL сниппетовщика не может быть пустым.")
                    elif len(wizardUrl) == 0:
                       errors.append("Поле Wizard URL не может быть пустым.")
                    else:
                        queryListId = int(snippetsDumpForm.cleaned_data["queries"])
                        startCalc = snippetsDumpForm.cleaned_data["startCalc"]
                        dumpSnippets(queryListId, newSerpName, engineUrl, searchByUrlFilter, cgiParams, wizardUrl, documentCachePath, startCalc)
                        return HttpResponseRedirect(ROOT_URL + "tasks.html")
                except MetricsException:
                    errors.append(sys.exc_info()[1])
    except:
        errors.append(get_option_value("defaultCriticalErrorMessage"))
        reqVars = copy(request.GET)
        reqVars.update(request.POST)
        writeToLog(request.path, reqVars)

    context = { "errors" : errors, "snippetsDumpForm" : snippetsDumpForm }
    return render_to_response("snips_dump.html", context, context_instance=RequestContext(request))

def snips_upload_html(request):
    errors = []
    snippetsUploadForm = forms.SnippetsUploadForm()
    try:
        del snippetsUploadForm.fields["engine"].choices[:]
        snippetsUploadForm.fields["engine"].choices.append(("-", "Задать вручную"))
        for engine in SnippetsEngine.objects.all():
            snippetsUploadForm.fields["engine"].choices.append((engine.id, engine.name))
        if request.method == "POST":
            snippetsUploadForm = forms.SnippetsUploadForm(request.POST)
        if snippetsUploadForm.is_valid():
            try:
                if not request.FILES.has_key("snipsDumpFile"):
                    raise MetricsException("Файл не выбран.")
                if snippetsUploadForm.cleaned_data["engine"] == "-":
                    engineUrl = snippetsUploadForm.cleaned_data["engineUrl"]
                    cgiParams = snippetsUploadForm.cleaned_data["cgiParams"]
                    wizardUrl = snippetsUploadForm.cleaned_data["wizardUrl"]
                else:
                    engine = SnippetsEngine.objects.get(id = int(snippetsUploadForm.cleaned_data["engine"]))
                    engineUrl = engine.url
                    cgiParams = engine.cgiParams
                    wizardUrl = engine.wizardUrl
                if len(engineUrl) == 0:
                    errors.append("Поле URL сниппетовщика не может быть пустым.")
                elif len(wizardUrl) == 0:
                    errors.append("Поле Wizard URL не может быть пустым.")
                else:
                    startCalc = int(snippetsUploadForm.cleaned_data["startCalc"])
                    addSnippetsDump(snippetsUploadForm.cleaned_data["newSerpName"], engineUrl, cgiParams, wizardUrl, int(snippetsUploadForm.cleaned_data["snipDumpFileType"]), request.FILES["snipsDumpFile"], startCalc)
                    return HttpResponseRedirect(ROOT_URL + "tasks.html")
            except MetricsException:
                errors.append(sys.exc_info()[1])
    except:
        errors.append(get_option_value("defaultCriticalErrorMessage"))
        reqVars = copy(request.GET)
        reqVars.update(request.POST)
        writeToLog(request.path, reqVars)

    context = { "errors" : errors, "snippetsUploadForm" : snippetsUploadForm }
    return render_to_response("snips_upload.html", context, context_instance=RequestContext(request))


def snips_add_html(request):
    errors = []
    if request.method == "POST":
        writeToLog("Processing form data")
        try:
            form = forms.SnippetsAddForm(request.POST)
            if form.is_valid():
                try:
                    if not request.FILES.has_key("snipsDumpFile"):
                        raise MetricsException("Файл не выбран.")
                    engineUrl = form.cleaned_data["engineUrl"]
                    cgiParams = form.cleaned_data["cgiParams"]
                    wizardUrl = form.cleaned_data["wizardUrl"]
                    if len(engineUrl) == 0:
                        errors.append("Поле URL сниппетовщика не может быть пустым.")
                    elif len(wizardUrl) == 0:
                        errors.append("Поле Wizard URL не может быть пустым.")
                    else:
                        addSnippets(form.cleaned_data["serpName"], engineUrl, cgiParams, wizardUrl, int(form.cleaned_data["snipDumpFileType"]), request.FILES["snipsDumpFile"])
                        return HttpResponseRedirect(ROOT_URL + "tasks.html")
                except MetricsException:
                    errors.append(sys.exc_info()[1])
        except:
            errors.append(get_option_value("defaultCriticalErrorMessage"))
            reqVars = copy(request.GET)
            reqVars.update(request.POST)
            writeToLog(request.path, reqVars)
    else:
        form = forms.SnippetsAddForm()
    context = { "errors" : errors, "snippetsAddForm" : form}
    return render_to_response("snips_add.html", context, context_instance=RequestContext(request))

def calc_metrics_html(request):
    errors = []
    context = {}
    calcMetricsForm = forms.CalcMetricsForm()
    try:
       if request.method == "POST":
           postDict = {"metrics" : request.POST.getlist("metrics"), "snipDumps" : request.POST.get("snipDumps"), "removePreviousValues" : request.POST.get("removePreviousValues")}
           calcMetricsForm = forms.CalcMetricsForm(postDict)
           fillMetricsTree(calcMetricsForm.fields["metrics"].choices)
           fillSnipDumpsList(calcMetricsForm.fields["snipDumps"].choices)
           if calcMetricsForm.is_valid():
               if bool(calcMetricsForm.cleaned_data["removePreviousValues"]):
                    MetricsCache.clear()
               metrics = calcMetricsForm.cleaned_data["metrics"]
               metricsOnly = []
               for id in metrics:
                   if id.startswith("met"):
                       metric = id.strip("met")
                       metricsOnly.append(int(metric))
               calc_metrics(metricsOnly, calcMetricsForm.cleaned_data["snipDumps"], str(SnippetsDump.objects.get(id = calcMetricsForm.cleaned_data["snipDumps"])), calcMetricsForm.cleaned_data["removePreviousValues"])
               return HttpResponseRedirect(ROOT_URL+"tasks.html")
       else:
           fillMetricsTree(calcMetricsForm.fields["metrics"].choices)
           fillSnipDumpsList(calcMetricsForm.fields["snipDumps"].choices)
    except:
       errors.append(get_option_value("defaultCriticalErrorMessage"))
       reqVars = copy(request.GET)
       reqVars.update(request.POST)
       writeToLog(request.path, reqVars)

    context = {"errors" : errors, "calcMetricsForm" : calcMetricsForm }
    return render_to_response("calc_metrics.html", context, context_instance=RequestContext(request))

def tasks_html(request):
    errors = []
    context = {}
    try:
        if request.method == "POST":
            if request.POST.get("clearAllFinished", False):
                for task in TaskProgress.objects.filter(progress = 100):
                    task.delete()
        else:
            cancelTaskId = int(request.GET.get("cancelTaskId", -1))
            if cancelTaskId != -1:
                cancelTask(cancelTaskId)
                return HttpResponseRedirect(ROOT_URL+"tasks.html")
        tasks = TaskProgress.objects.all()
        lastTask = TaskProgress.objects.order_by("modificationDate").reverse().all()
        context = {"tasks" : tasks}
        if len(lastTask) > 0:
            context["lastTaskId"] = lastTask[0].id
    except:
        errors.append(get_option_value("defaultCriticalErrorMessage"))
        reqVars = copy(request.GET)
        reqVars.update(request.POST)
        writeToLog(request.path, reqVars)
    context["errors"] = errors
    return render_to_response("tasks.html", context, context_instance=RequestContext(request))

def snippets_html(request):
    from django.core.paginator import Paginator, PageNotAnInteger, EmptyPage
    from django.db import connection
    errors = []
    context = {}
    try:
        snips2Del = int(request.GET.get("delDumpId", -1))
        snips2Archive = int(request.GET.get("archiveDumpId", -1))
        if snips2Del >= 0:
            obj2Del = SnippetsDump.objects.get(id = snips2Del)
            for snip in Snippet.objects.filter(snippetsDump = obj2Del.id).all():
                if snip.documentFilePath != "":
                    removeFileIfExists(get_option_value("homeDir") + snip.documentFilePath)
            cursor = connection.cursor()
            cursor.execute("""DELETE mv FROM snipmetricsview_metricvalue AS mv
                              INNER JOIN snipmetricsview_snippet AS s ON (s.snippetsDump_id = %s)
                              WHERE mv.snippet_id = s.id""", [snips2Del])
            cursor.execute("""
                              DELETE FROM snipmetricsview_snippet WHERE snippetsDump_id = %s
                           """, [snips2Del])
            obj2Del.delete()
            return HttpResponseRedirect(ROOT_URL+"snippets.html")
        if snips2Archive != -1:
            dump = SnippetsDump.objects.get(id = snips2Archive)
            dump.isArchive = not dump.isArchive
            dump.save()
            return HttpResponseRedirect(ROOT_URL+"snippets.html")
        collectCachedDocsDumpId = int(request.GET.get("collectCachedDocs", -1))
        if collectCachedDocsDumpId >= 0:
            collectCachedDocs(collectCachedDocsDumpId)
            return HttpResponseRedirect(ROOT_URL+"tasks.html")
        snippetsDumps = SnippetsDump.objects.filter(isArchive = False)
        snippetsDumpsArchived = SnippetsDump.objects.filter(isArchive = True)
        context["snippetsDumps"] = snippetsDumps
        context["snippetsDumpsArchived"] = snippetsDumpsArchived
        snipDumpId = int(request.GET.get("snipdumpid", 0))
        if snipDumpId != 0:
            snippets = Snippet.objects.filter(snippetsDump = snipDumpId)
            paginator = Paginator(snippets, 20)
            page = request.GET.get('page', 1)
            try:
                snippets = paginator.page(page)
            except PageNotAnInteger:
                # If page is not an integer, deliver first page.
                snippets = paginator.page(1)
            except EmptyPage:
                # If page is out of range (e.g. 9999), deliver last page of results.
                snippets = paginator.page(paginator.num_pages)
            context["snipDumpId"] = snipDumpId
            context["snippets"] = snippets

        context["snipDumps"] = SnipDump.objects.all()
    except:
        errors.append(get_option_value("defaultCriticalErrorMessage"))
        reqVars = copy(request.GET)
        reqVars.update(request.POST)
        writeToLog(request.path, reqVars)
    context["errors"] = errors

    return render_to_response("snippets.html", context, context_instance=RequestContext(request))

def snippets_diff_html(request):
    snippetsDiffForm = forms.SnippetsDiffForm(request.GET)
    context = { "snippetsDiffForm" : snippetsDiffForm, }
    fillSnipDumpsList(snippetsDiffForm.fields["firstDump"].choices)
    fillSnipDumpsList(snippetsDiffForm.fields["secondDump"].choices)
    def get_data(snip):
        return ( snip.title if snippetsDiffForm.cleaned_data["needTitle"] else "",
                 snip.snippet if  snippetsDiffForm.cleaned_data["needSnippet"] else "" )
    if snippetsDiffForm.is_valid():
        firstDump = snippetsDiffForm.cleaned_data["firstDump"]
        secondDump = snippetsDiffForm.cleaned_data["secondDump"]
        if firstDump != '' and secondDump != '':
            firstSnips = {}
            for snip in Snippet.objects.filter(snippetsDump = firstDump).select_related():
                firstSnips[(snip.query.text, snip.url)] = get_data(snip)
            secondSnips = {}
            for snip in Snippet.objects.filter(snippetsDump = secondDump).select_related():
                secondSnips[(snip.query.text, snip.url)] = get_data(snip)
            inter = list(set(firstSnips.keys()) & set(secondSnips.keys()))
            context["firstDumpSnipsCount"] = len(firstSnips)
            context["secondDumpSnipsCount"] = len(secondSnips)
            context["queryUrlsDiffPercent"] = "%.2f" % (100 - 100*(len(inter) / float(len(secondSnips.keys())))) if len(secondSnips.keys()) != 0 else 0.0
            diff = 0
            differentSnips = []
            for k in inter:
                if firstSnips[k] != secondSnips[k]:
                    diff += 1
                    differentSnips.append((k[0], k[1], firstSnips[k][0], firstSnips[k][1], secondSnips[k][0], secondSnips[k][1]))
            context["snipsDiffPercent"] = "%.2f" % (100*(diff / float(len(inter)))) if len(inter) != 0 else 0.0

            from django.core.paginator import Paginator, PageNotAnInteger, EmptyPage
            paginator = Paginator(differentSnips, 100)
            page = int(request.GET.get("page", 1))
            try:
                differentSnips = paginator.page(page)
            except PageNotAnInteger:
                # If page is not an integer, deliver first page.
                differentSnips = paginator.page(1)
            except EmptyPage:
                # If page is out of range (e.g. 9999), deliver last page of results.
                differentSnips = paginator.page(paginator.num_pages)
            context["differentSnips"] = differentSnips
            context["firstDumpId"] = firstDump
            context["secondDumpId"] = secondDump

    return render_to_response("snippets_diff.html", context, context_instance=RequestContext(request))

def help_html(request):
    return render_to_response("help.html", context_instance=RequestContext(request))

def wizard_html(request):
    context = {}
    if request.method == "POST":
        wizardForm = forms.WizardForm(request.POST)
        if wizardForm.is_valid():
            query = wizardForm.cleaned_data["query"]
            richTree = getRichRequestTree(query, "xmlsearch.hamster.yandex.ru")
            request.POST["richTree"] = richTree

    else:
       wizardForm = forms.WizardForm()
    context["wizardForm"] = wizardForm
    return render_to_response("wizard.html", context, context_instance=RequestContext(request))

