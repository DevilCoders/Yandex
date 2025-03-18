# coding: utf-8
from __future__ import unicode_literals

import json

from django.contrib.auth.models import Group
try:
    from django.utils.encoding import force_text
except ImportError:
    from django.utils.encoding import force_str as force_text
from django_idm_api.compat import get_user_model


def refresh(obj):
    return type(obj).objects.get(pk=obj.pk)


class JsonClient(object):
    """Класс, аналогичный django.test.client.Client, но сокращённым интерфейсом. Для POST-запросов делает запросы
    с Content-Type multipart/form-data, но кодирующий сериализующий все вложенные словари в JSON.
    Также добавляет к ответу метод json(), который пытается декодировать тело ответа из формата JSON.
    """
    def __init__(self, client):
        self.client = client

    def get(self, path, data={}, **extra):
        return self.jsonify(self.client.get(path, data=data, **extra))

    def post(self, path, data={}, **extra):
        data = {key: json.dumps(value) if isinstance(value, dict) else value
                for key, value in data.items()}
        return self.jsonify(self.client.post(path, data=data, **extra))

    def jsonify(self, response):
        response.json = lambda: json.loads(force_text(response.content)) if hasattr(response, 'content') else None
        return response


def create_user(username):
    user, _ = get_user_model().objects.get_or_create(username=username)
    return user


def create_superuser(username):
    user, _ = get_user_model().objects.get_or_create(username=username, defaults={'is_superuser': True})
    return user


def create_group(name):
    group, _ = Group.objects.get_or_create(name=name)
    return group
