# coding: utf-8
from __future__ import unicode_literals
from django_idm_api.compat import url, include


urlpatterns = [
    url(r'^client-api/', include('django_idm_api.urls', namespace='client-api')),
    url(r'^(?P<system>\w+)/client-api/', include('django_idm_api.urls', namespace='system-client-api'))
]
