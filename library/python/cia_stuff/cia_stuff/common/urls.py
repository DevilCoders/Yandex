# coding: utf-8

from __future__ import unicode_literals

import yenv

from django.conf.urls import url

from cia_stuff.common import views


if yenv.type == 'production':
    urlpatterns = []
else:
    urlpatterns = [
        url(r'^celery_tasks/$', views.CeleryTasksView.as_view()),
    ]
