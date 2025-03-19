from django.conf import settings
from django.conf.urls import url
from django.conf.urls.static import static
from django.contrib import admin
from django.urls import path

from cloud.mdb.ui.internal.mdbui import views

admin.site.site_header = settings.INSTALLATION.value
admin.site.site_title = 'MDB UI ' + settings.INSTALLATION.value
admin.site.index_title = 'Dashboard'


urlpatterns = (
    static(settings.SOURCE_STATIC_URL, document_root=settings.SOURCE_STATIC_ROOT)
    + static(settings.STATIC_URL, document_root=settings.STATIC_ROOT)
    + static(settings.MEDIA_URL, document_root=settings.MEDIA_ROOT)
    + [
        url(r'^login/', views.login_redirect),
        path('', admin.site.urls),
        url(r'^ping', views.ping),
    ]
)
