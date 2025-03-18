# coding: utf-8
from __future__ import unicode_literals

from django.db import models
from django.utils import timezone
from django.contrib.postgres.fields import JSONField

class Metric(models.Model):
    slug = models.CharField(max_length=255, unique=True, primary_key=True)
    timedelta = models.DurationField()
    max_timedelta = models.DurationField()
    is_exportable = models.BooleanField()
    report_path = models.CharField(max_length=255, unique=True, null=True, blank=True)

    def __unicode__(self):
        return self.slug

    def __str__(self):
        return self.__unicode__()


class MetricPoint(models.Model):
    metric = models.ForeignKey(Metric, on_delete=models.CASCADE, null=False,
                               db_index=True, related_name='metric_points')
    created_at = models.DateTimeField(auto_now=True, db_index=True)
    context = JSONField(null=True, blank=True)

    def is_outdated(self):
        return timezone.now() - self.created_at > self.metric.max_timedelta


class Value(models.Model):
    slug = models.CharField(max_length=255)
    metric_point = models.ForeignKey(MetricPoint, on_delete=models.CASCADE, null=False, related_name='values')
    value = models.FloatField()

    def __unicode__(self):
        return '{} for {} has value {}'.format(self.metric_point.metric.slug, self.slug, self.value)

    def __str__(self):
        return self.__unicode__()
