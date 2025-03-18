# coding: utf-8

from rest_framework.permissions import BasePermission

from django.conf import settings


class IsAuthenticated(BasePermission):
    """
    Проверяет наличие доступа у пользователя в АПИ синхронизации данных с Директорией.
    Доступ возможен по токену, значение которого должно быть задано в настройках приложения в атрибуте DIR_TOKEN.
    """

    def has_permission(self, request, view):
        return request.user.is_authenticated() or request.META['HTTP_AUTHORIZATION'] == settings.DIRSYNC_DIR_TOKEN
