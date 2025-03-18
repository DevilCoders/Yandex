# -*- coding:utf-8 -*-
from django.utils.decorators import decorator_from_middleware

from django_yauth.middleware import YandexAuthRequiredMiddleware


yalogin_required = decorator_from_middleware(YandexAuthRequiredMiddleware)
