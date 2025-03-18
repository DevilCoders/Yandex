#!/usr/bin/python
# -*- coding: utf-8 -*-

from django.db import models
from options import get_option_value
from settings import MEDIA_ROOT
from time import gmtime, strftime

class QueryCharacteristic(models.Model):
    name = models.CharField(max_length = 255, primary_key = True)

    def __unicode__(self):
        return self.name

class QueryExtraInfoType(models.Model):
    name = models.CharField(max_length = 127, primary_key = True)

    def __unicode__(self):
        return self.name

class QueryExtraInfo(models.Model):
    infoType = models.ForeignKey(QueryExtraInfoType)
    value = models.CharField(max_length = 1024)

    def __unicode__(self):
        return self.infoType.name + " = " + self.value

class Query(models.Model):
    text = models.CharField(max_length = 2048)
    urlToFind = models.URLField(max_length = 512, blank = True)
    region = models.CharField(max_length = 10, blank = True)
    characteristics = models.ManyToManyField(QueryCharacteristic)
    lenInWords = models.PositiveIntegerField(db_index = True)
    extraInfo = models.ManyToManyField(QueryExtraInfo)

    def __unicode__(self):
        res = self.text
        res += (u"\turl:" + self.urlToFind) if len(self.urlToFind) > 0 else u""
        res += (u"\tregion:" + self.region) if len(self.region) > 0 else u""
        return res

class QueryList(models.Model):
    queries = models.ManyToManyField(Query)
    name = models.CharField(max_length = 512)

    def __unicode__(self):
        return self.name

class SnippetsEngine(models.Model):
    name = models.CharField(max_length = 512)
    description = models.CharField(max_length = 4096, blank = True)
    url = models.URLField(max_length = 255)
    searchByUrlFilter = models.CharField(max_length = 255, default = "url:")
    cgiParams = models.CharField(max_length = 1024, blank = True)
    wizardUrl = models.CharField(max_length = 255, default = "xmlsearch.hamster.yandex.ru", blank = True)
    documentCacheUrlTemplate = models.CharField(max_length = 255)

    def __unicode__(self):
        return self.name + " >> " + self.url + "?" + self.cgiParams

class SnippetsDump(models.Model):
    name = models.CharField(max_length = 1024)
    date = models.DateTimeField(auto_now_add = True)
    queryList = models.ForeignKey(QueryList, blank=True, null=True)
    engineUrl = models.URLField(max_length = 255)
    cgiParams = models.CharField(max_length = 1024, blank = True)
    wizardUrl = models.CharField(max_length = 255, default = "xmlsearch.hamster.yandex.ru", blank = True)
    documentCacheUrlTemplate = models.CharField(max_length = 255, blank = True)
    isArchive = models.BooleanField(blank=True, db_index = True)

    def __unicode__(self):
        return self.name + "\t[" + self.date.strftime("%d-%m-%Y %H:%M") + "]"

    class Meta:
        ordering = ['-date']



def make_upload_path(instance, filename):
    return strftime("%H%M%S_", gmtime()) + filename

class SnipDump(models.Model):
    name = models.CharField(max_length = 1024)
    date = models.DateTimeField(auto_now_add = True)
    fileType = models.PositiveIntegerField(default=0) 
    fileName = models.FileField(upload_to = make_upload_path)
    fileRes = models.FileField(upload_to = make_upload_path)

    def __unicode__(self):
        return self.name + "\t[" + self.date.strftime("%d-%m-%Y %H:%M") + "]"

    class Meta:
        ordering = ['-date']



class Snippet(models.Model):
    query = models.ForeignKey(Query)
    url = models.URLField(max_length = 512)
    snippetsDump = models.ForeignKey(SnippetsDump)
    title = models.CharField(max_length = 1024, blank = True)
    snippet = models.CharField(max_length = 2048, blank = True)
    documentFilePath = models.CharField(max_length = 2048, blank = True)
    extraInfo = models.CharField(max_length = 1024, blank = True)

    def __unicode__(self):
        return str(self.id) + " - " + self.title + self.snippet

class MetricCategory(models.Model):
    name = models.CharField(max_length = 255, unique = True, verbose_name = u"Название")

    def __unicode__(self):
        return self.name

class MetricCalculator(models.Model):
    exePath = models.FilePathField(max_length = 255, unique = True, path = get_option_value("metricCalculatorPath"))
    commandLineParams = models.CharField(max_length=255, blank = True)

    def __unicode__(self):
        return self.exePath.split("/")[-1]

class Metric(models.Model):
    name = models.CharField(max_length = 1024)
    shortName = models.CharField(max_length = 255, db_index = True)
    categories = models.ManyToManyField(MetricCategory)
    calculator = models.ForeignKey(MetricCalculator)
    description = models.CharField(max_length = 4096, blank = True)

    def __unicode__(self):
        return self.name

class MetricValue(models.Model):
    metric = models.ForeignKey(Metric)
    snippet = models.ForeignKey(Snippet)
    value = models.FloatField()
    is_with_title = models.BooleanField(db_index = True)

    def __unicode__(self):
        return str(self.snippet.id) + ": " + self.metric.name + "=" + str(self.value)

    class Meta:
        ordering = ["snippet"]

class TaskType(models.Model):
    id = models.CharField(primary_key = True, max_length = 100)
    name = models.CharField(unique = True, max_length = 255)

    def __unicode__(self):
        return self.name

class TaskProgress(models.Model):
    taskType = models.ForeignKey(TaskType)
    progress = models.PositiveIntegerField()
    details = models.CharField(max_length = 4096)
    errors = models.CharField(max_length = 4096, blank = True)
    resultObjectId = models.PositiveIntegerField(blank = True, null = True) # id of objects created by the task: SnippetsDump id for dump and upload, QueryList id for uploading query list, nothing for metrics calculation
    startDate = models.DateTimeField(auto_now_add = True)
    modificationDate = models.DateTimeField(blank = True)

    def __unicode__(self):
        return self.taskType.name + ": " + self.details + " (" + str(self.progress) + "%)"

    class Meta:
        ordering = ['progress', '-modificationDate']

    def update(self, details=None, progress=None, errors=None, res_id=None):
        if details:
            self.details = details
        if progress:
            self.progress = progress
        if errors:
            self.errors = errors
        if res_id:
            self.resultObjectId = res_id
        self.save()

class ShortUrl(models.Model):
    shortUrl = models.URLField(max_length = 255, primary_key = True)
    fullUrl = models.URLField(max_length = 4096)

