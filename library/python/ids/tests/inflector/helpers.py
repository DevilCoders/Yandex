# coding: utf-8
from __future__ import unicode_literals


def build_inflector_answer(data):
    return {'Form':
                         [{'Grammar': form_name, 'Text': form_value} for (form_name, form_value) in data]
    }

all_cases = ['им', 'род', 'дат', 'вин', 'твор', 'пр']

word_data = {
    'им': 'автомобиль',
    'род': 'автомобиля',
    'дат': 'автомобилю',
    'вин': 'автомобиль',
    'твор': 'автомобилем',
    'пр': 'автомобиле'
}.items()


title_data = {
    'им': 'persn{Иван}famn{Иванов}patrn{Иванович}gender{m}',
    'род': 'persn{Ивана}famn{Иванова}patrn{Ивановича}gender{m}',
    'дат': 'persn{Ивану}famn{Иванову}patrn{Ивановичу}gender{m}',
    'вин': 'persn{Ивана}famn{Иванова}patrn{Ивановича}gender{m}',
    'твор': 'persn{Иваном}famn{Ивановым}patrn{Ивановичем}gender{m}',
    'пр': 'persn{Иване}famn{Иванове}patrn{Ивановиче}gender{m}'
}.items()


title_data_parsed = {
    'им': {
        'first_name': 'Иван',
        'middle_name': 'Иванович',
        'last_name': 'Иванов'
    },
    'род': {
        'first_name': 'Ивана',
        'middle_name': 'Ивановича',
        'last_name': 'Иванова'
    },
    'дат': {
        'first_name': 'Ивану',
        'middle_name': 'Ивановичу',
        'last_name': 'Иванову'
    },
    'вин': {
        'first_name': 'Ивана',
        'middle_name': 'Ивановича',
        'last_name': 'Иванова'
    },
    'твор': {
        'first_name': 'Иваном',
        'middle_name': 'Ивановичем',
        'last_name': 'Ивановым'
    },
    'пр': {
        'first_name': 'Иване',
        'middle_name': 'Ивановиче',
        'last_name': 'Иванове'
    }
}


class Person(object):
    def __init__(self, first_name, last_name, middle_name='', gender='m'):
        self.first_name = first_name
        self.last_name = last_name
        self.middle_name = middle_name
        self.gender = gender


class PersonOnlyRequired(object):
    def __init__(self, first_name, last_name):
        self.first_name = first_name
        self.last_name = last_name
