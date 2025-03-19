import django.urls as du

import cloud.mdb.backstage.apps.mlock.views as views


urlpatterns = [
    du.path('locks', views.LockListSkelView.as_view()),
    du.path('ajax/locks', views.LockListView.as_view()),
]
