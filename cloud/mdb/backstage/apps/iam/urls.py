from django.conf import settings
import django.urls as du

import cloud.mdb.backstage.apps.iam.ajax as ajax
import cloud.mdb.backstage.apps.iam.views as views


urlpatterns = [
    du.re_path(r'^callback', views.callback)
]


if settings.CONFIG.iam.get('debug'):
    urlpatterns.extend([
        du.path('debug', views.debug),
        du.path('ajax/debug_config', ajax.debug_config),
        du.path('ajax/debug_auth_info', ajax.debug_auth_info),
    ])
