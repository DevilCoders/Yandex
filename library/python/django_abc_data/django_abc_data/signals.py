# coding: utf-8
from __future__ import unicode_literals

from django.dispatch import Signal


service_created = Signal(providing_args=('object', 'data'))
service_updated = Signal(providing_args=('object', 'data'))
