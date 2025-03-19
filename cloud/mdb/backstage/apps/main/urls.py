from django.conf import settings
import django.urls as du

import cloud.mdb.backstage.apps.main.ajax as ajax
import cloud.mdb.backstage.apps.main.ping as ping
import cloud.mdb.backstage.apps.main.views as views


handler404 = 'cloud.mdb.backstage.apps.main.views.server_404_handler'
handler500 = 'cloud.mdb.backstage.apps.main.views.server_500_handler'

urlpatterns = [
    du.re_path(r'^$', views.main),
    du.path('ping/app', ping.app),
    du.path('auth/', du.include('cloud.mdb.backstage.apps.iam.urls')),
    du.path('ui/main/dashboard/', views.dashboard),
    du.path('ui/main/audit', views.audit),
    du.path('ui/main/user/profile', views.user_profile),
    du.path('ui/main/stats/versions', views.stats_versions),
    du.path('ui/main/stats/maintenance_tasks', views.stats_maintenance_tasks),
    du.path('ui/main/tools/vector_config', views.vector_config),
    du.path('ui/main/tools/health_ua', views.health_ua),
    du.path('ui/main/ajax/audit', ajax.audit),
    du.path('ui/main/ajax/related_links/<str:fqdn>', ajax.related_links),
    du.path('ui/main/ajax/user/profile', ajax.user_profile),
    du.path('ui/main/ajax/dashboard/failed_tasks', ajax.failed_tasks),
    du.path('ui/main/ajax/dashboard/duties', ajax.duties),
    du.path('ui/main/ajax/dashboard/search', ajax.search),
    du.path('ui/main/ajax/stats/versions', ajax.stats_versions),
    du.path('ui/main/ajax/stats/maintenance_tasks', ajax.stats_maintenance_tasks),
    du.path('ui/main/ajax/tools/vector_schema', ajax.tools_vector_schema),
    du.path('ui/main/ajax/tools/health_ua', ajax.health_ua),
]

for app in settings.ENABLED_BACKSTAGE_APPS:
    urlpatterns.append(
        du.path(f'ui/{app}/', du.include(f'cloud.mdb.backstage.apps.{app}.urls'))
    )
