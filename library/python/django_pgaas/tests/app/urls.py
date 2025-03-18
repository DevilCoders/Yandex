# coding: utf-8
from __future__ import unicode_literals

from django.conf.urls import url
from django.http import HttpResponse

urlpatterns = [
    url('^hello/?$', lambda request: HttpResponse('world'))
]
