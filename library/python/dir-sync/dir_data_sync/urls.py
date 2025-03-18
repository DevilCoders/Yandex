# coding: utf-8

from django.conf.urls import url

from . import views

urlpatterns = [
    url(
        r'org/(?P<dir_org_id>\d+)$',
        views.OrganizationView.as_view(),
        name='organization',
    ),
    url(r'changed$', views.process_data_change_request),
    url(r'stats$', views.get_org_data_sync_statistics),
]
