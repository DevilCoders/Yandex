#!/usr/bin/python
# -*- coding: utf-8 -*-

from django import forms
from codecs import encode
from snipmetricsview.models import *
from snipmetricsview.widgets import *

class QueriesForm(forms.Form):
    name = forms.CharField(max_length = 255, required = True, label = "Название списка")
    queriesFile = forms.FileField(label = "Файл", required = False)
    queries = forms.CharField(widget=forms.Textarea(attrs={'cols':40, 'rows': 10}), required = False, label = "Список запросов (запрос#регион[#url])")

class EngineForm(forms.Form):
    id = forms.CharField(initial = "", required = False, widget = forms.HiddenInput)
    name = forms.CharField(max_length = 255, required = True, label = "Название сниппетовщика")
    description = forms.CharField(max_length = 4096, widget=forms.Textarea(attrs={'cols':'40', 'rows': 4}), required = False, label = "Описание сниппетовщика")
    url = forms.URLField(label = "Url (используется для сбора сниппетов, например: http://www.yandex.ru/)", required = True)
    searchByUrlFilter = forms.CharField(max_length = 255, required = True, label = "Фильтр для поиска по URL-у (например 'url:')")
    cgiParams = forms.CharField(max_length = 4096, required = False, label = "CGI параметры")
    wizardUrl = forms.CharField(max_length = 4096, required = False, widget = forms.TextInput(attrs = {'size': 52,}),initial=u"xmlsearch.hamster.yandex.ru", label = "Wizard URL")
    documentCacheUrlTemplate = forms.CharField(label="Шаблон пути к сохраненной копии документов ( %s - место для урла )", required = False)

class MetricForm(forms.Form):
    name = forms.CharField(max_length = 255, label = "Описание", required = True)
    shortName = forms.CharField(max_length = 50, label = "Имя", required = True)
    description = forms.CharField(max_length = 4096, widget=forms.Textarea(attrs={'cols':'40', 'rows': 4}), label = "Описание", required = False)
    calculatorExePath = forms.ChoiceField(choices = [], label = "Путь к программе подсчета", required = False)
    calculatorExeFile = forms.FileField(label = "Файл программы подсчета метрики", required = False)
    categories = forms.MultipleChoiceField(choices = [], required = True, label = "Категории")
    commandLineParams = forms.CharField(max_length = 255, required = False, label = "Параметры командной строки")

class MetricCategoryForm(forms.ModelForm):
    class Meta:
        model = MetricCategory

class SnippetsDumpForm(forms.Form):
    newSerpName = forms.CharField(max_length = 255, required = False, label = "Название списка серпов")
    queries = forms.ChoiceField(choices = [], required = True, label = "Список запросов")
    engine = forms.ChoiceField(choices = [], required = False, label = "Сниппетовщик")
    engineUrl = forms.URLField(required = False, label = "URL сниппетовщика")
    searchByUrlFilter = forms.CharField(max_length = 255, required = False, label = "Фильтр для поиска по URL-у (например 'url:')")
    cgiParams = forms.CharField(max_length = 1024, required = False, label = "CGI параметры")
    wizardUrl = forms.CharField(max_length = 255, widget = forms.TextInput(attrs = {'size': 52,}), initial = u"xmlsearch.hamster.yandex.ru", required = False, label = "Wizard URL")
    documentCacheUrlTemplate = forms.CharField(max_length = 255, required = False, label = "Шаблон пути к сохраненной копии документов ( %s - место для урла )")
    startCalc = forms.BooleanField(required = False, label = u"Начать подсчет всех метрик после загрузки сниппетов")

class SnippetsUploadForm(forms.Form):
    newSerpName = forms.CharField(max_length = 255, required = False, label = "Название списка серпов")
    engine = forms.ChoiceField(choices = [], required = False, label = "Сниппетовщик")
    engineUrl = forms.URLField(required = False, label = "URL сниппетовщика")
    cgiParams = forms.CharField(max_length = 1024, required = False, label = "CGI параметры")
    wizardUrl = forms.CharField(max_length = 255, widget = forms.TextInput(attrs = {'size': 52,}), initial = u"xmlsearch.hamster.yandex.ru", required = False, label = "Wizard URL")
    snipDumpFileType = forms.ChoiceField(choices = [(1, u"Файл с полями, разделенными табами"), (2, u"XML файл (в формате SERPа)"),(3, u"XML файл (в формате сниппетов)"), (5, u"XML файл (в новом формате сниппетов)")], required = True, label = "Тип файла")
    snipsDumpFile = forms.FileField(required = False, label = "Файл с собранными сниппетами")
    startCalc = forms.BooleanField(required = False, label = u"Начать подсчет всех метрик после загрузки сниппетов")

class CalcMetricsForm(forms.Form):
    snipDumps = forms.ChoiceField(choices = [], required = True, label = u"Дампы сниппетов")
    metrics = CheckboxTreeField(choices = [], widget = CheckboxTreeWidget, required = True, label = u"Метрики для подсчета")
    removePreviousValues = forms.BooleanField(required = False, label = u"Удалить текущие значения метрик")

class ViewMetricsForm(forms.Form):
    snipDumps = forms.MultipleChoiceField(choices = [], required = False, widget = forms.SelectMultiple(attrs = {'size' : 10}), label = u"Собранные сниппеты")
    metrics = CheckboxTreeField(choices = [], widget = CheckboxTreeWidget, required = False, label = u"Метрики для подсчета")
    withTitle = forms.BooleanField(required = False, label = u"Посмотреть метрики с учетом заголовка")
    queryCharacteristics = forms.MultipleChoiceField(required = False, label = u"Категории запросов")
    queryWordsCount = forms.MultipleChoiceField(required = False, label = u"Длина запросов")
    sameScale = forms.BooleanField(required = False, label = u"Выводить графики в одинаковом масштабе")
    sameQueryUrls = forms.BooleanField(required = False, label = u"Выводить графики по одинаковым урлам-запросам (дольше)")

class WizardForm(forms.Form):
    query = forms.CharField(max_length = 4096, required = True, widget = forms.TextInput(attrs = {'size': 52,}), label = u"Запрос")
    richTree = forms.CharField(max_length = 4096, widget=forms.Textarea(attrs={'cols':'80', 'rows': 7}), required = False, label = u"Переколдовка")

class SnippetsDiffForm(forms.Form):
    firstDump = forms.ChoiceField(choices = [], required = False, label = u"Дамп 1")
    secondDump = forms.ChoiceField(choices = [], required = False, label = u"Дамп 2")
    needTitle = forms.BooleanField(required = False, label = u"Использовать заголовок")
    needSnippet = forms.BooleanField(required = False, label = u"Использовать сниппет")

class SnippetsAddForm(forms.Form):
    serpName = forms.CharField(max_length = 255, required = False, label = "Название списка серпов")
    engineUrl = forms.URLField(required = False, initial = u"yandex.ru", label = "URL сниппетовщика")
    cgiParams = forms.CharField(max_length = 1024, required = False, label = "CGI параметры")
    wizardUrl = forms.CharField(max_length = 255, widget = forms.TextInput(attrs = {'size': 52,}), initial = u"xmlsearch.hamster.yandex.ru", required = False, label = "Wizard URL")
    snipsDumpFile = forms.FileField(required = False, label = u"Файл с собранными сниппетами")
    snipDumpFileType = forms.ChoiceField(choices = [(5, u"XML файл (в новом формате сниппетов)"), (2, u"XML файл (в формате SERPа)")], required = True, label = "Тип файла")
