# coding: utf-8
from __future__ import unicode_literals, absolute_import

from django.db import models


class StampRecord(models.Model):
    timestamp = models.DateTimeField(auto_now=True)
    name = models.CharField(max_length=255)
    host = models.CharField(max_length=255)
    group = models.CharField(max_length=255)
    data = models.TextField()

    def __unicode__(self):
        return '%s at `%s`[%s]' % (self.name, self.host, self.group)

    class Meta:
        unique_together = [('name', 'host', 'group')]
