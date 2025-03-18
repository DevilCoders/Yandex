from django.conf.urls import url
from django.conf import settings
from django_yauth import views


urlpatterns = (
    url(r'^create-profile/$', views.create_profile, name=settings.YAUTH_CREATE_PROFILE_VIEW),
)
