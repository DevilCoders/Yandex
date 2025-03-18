from django.urls import include, path
from django.conf.urls.static import static
from django.conf import settings


urlpatterns = [
    path("idm/", include("django_idm_api.urls")),
    path("", include("antirobot.cbb.cbb_django.cbb.urls")),
] + static(settings.STATIC_URL, document_root=settings.STATICFILES_DIRS[0])
