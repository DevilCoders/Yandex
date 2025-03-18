# coding: utf-8

from django.contrib.admin import site, ModelAdmin
from metrics_framework.models import Metric


site.register(Metric, ModelAdmin)
