# coding: utf-8
from __future__ import unicode_literals

from django.db import models


class RandomModel(models.Model):
    name = models.CharField(max_length=254)
