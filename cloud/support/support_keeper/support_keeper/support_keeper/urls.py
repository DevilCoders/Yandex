"""support_keeper URL Configuration

The `urlpatterns` list routes URLs to views. For more information please see:
    https://docs.djangoproject.com/en/3.2/topics/http/urls/
Examples:
Function views
    1. Add an import:  from my_app import views
    2. Add a URL to urlpatterns:  path('', views.home, name='home')
Class-based views
    1. Add an import:  from other_app.views import Home
    2. Add a URL to urlpatterns:  path('', Home.as_view(), name='home')
Including another URLconf
    1. Import the include() function: from django.urls import include, path
    2. Add a URL to urlpatterns:  path('blog/', include('blog.urls'))
"""
from django.contrib import admin
from django.contrib.staticfiles.urls import staticfiles_urlpatterns
from django.urls import path, include
from rest_framework.routers import DefaultRouter
from s_keeper.views import Queue_FilterViewSet, Support_UnitViewSet, ComponentsDictViewSet, RanksDictViewSet, \
    Support_Unit_AbsentViewSet, Support_UnitPendingViewSet, UserRetrieveUpdateAPIView, \
    RegistrationAPIView, LoginAPIView
from s_keeper.st_connector import ExtAPI

router = DefaultRouter()
router.register('filters', Queue_FilterViewSet)
router.register('s_units', Support_UnitViewSet)
router.register('s_units_absent', Support_Unit_AbsentViewSet)
router.register('components', ComponentsDictViewSet)
router.register('ranks', RanksDictViewSet)


urlpatterns = [
    path('admin/', admin.site.urls),
    path('api-auth/', include('rest_framework.urls')),
    path('api/', include(router.urls)),
    path('api/ext', ExtAPI.as_view()),
    path('api/s_units_pending/', Support_UnitPendingViewSet.as_view()),
    path('user/', UserRetrieveUpdateAPIView.as_view()),
    path('user/register/', RegistrationAPIView.as_view()),
    path('user/login/', LoginAPIView.as_view()),

]
urlpatterns += staticfiles_urlpatterns()