#!/usr/bin/python
# -*- coding: utf-8 -*-


# TODO: FIX THIS
ROOT_DIRECTORY = "/place/viewers-data/snipmetrics/"
ROOT_URL = "http://scrooge.yandex.ru:8080/"

metrics_options = {
                   "dumpSnippetsScript"             : ROOT_DIRECTORY + "snipmetricsview/dump_scripts/serp_dumper.py",
                   "metricCalculatorPath"           : ROOT_DIRECTORY + "snipmetricsview/metric_calculators/",
                   "tempDir"                        : "/var/tmp",
                   "homeDir"                        : ROOT_DIRECTORY,
                   "scriptsHomeDir"                 : ROOT_DIRECTORY + "snipmetricsview/",
                   "logDir"                         : ROOT_DIRECTORY + "snipmetricsview/log/",
                   "documentCacheDir"               : ROOT_DIRECTORY + "snipmetricsview/doc_cache/",
                   "wizardsScript"                  : ROOT_DIRECTORY + "snipmetricsview/wizard/wizards.py",
                   "snippetXmlFormatParser"         : ROOT_DIRECTORY + "snipmetricsview/snippet_xml_parser/python/",
                   "defaultCriticalErrorMessage"    : "Неизвестная ошибка!",
                   "defaultQueryRegion"             : 213,
                  }

def get_option_value(option_name):
   return metrics_options[option_name]

