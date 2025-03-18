#!/usr/bin/env python
# coding: utf-8
# pylint: disable=invalid-name

from __future__ import division, absolute_import, print_function, unicode_literals

import logging

import statface_client

# пример настройки логгера
logger = logging.getLogger('statface_client')
logger.addHandler(logging.StreamHandler())
logger.setLevel(logging.INFO)
# все 400 и 500 от сервера попадают в лог

# Для работы со Статфейсом (отчётами, словарями и т.д.) нужен клиент.
# Клиент хранит в себе все настройки взаимодействия со Статфейсом
# (хост, авторизация и другое).
# Хост по умолчанию -- upload.stat-beta.yandex-team.ru.

client = statface_client.StatfaceClient()

# `oauth_token` берётся из файла настроек `~/.statbox/statface_auth.yaml`

# ВНИМАНИЕ: По-умолчанию используется stat-beta. Для использования production, надо указать
# client = statface_client.StatfaceClient(host=statface_client.STATFACE_PRODUCTION)

# Расширенные параметры указываются через StatfaceClient(client_config=...);
# все доступные параметры см в DEFAULT_STATFACE_CONFIG.

# для работы с отчётом необходим объект StatfaceReport.

# метод get_report
# делает базовую валидацию пути и возвращает объект StatfaceReport
report = client.get_report('Adhoc/Adhoc/ExampleReport')
# один клиент может работать с несколькими отчётами

# StatfaceReport не хранит никакого локального состояния,
# всякое действие синхронизирует с сервером.

# Mетод get_report проверит валидность пути к очтёту,
# и корректно отработает как и для существующего отчёта,
# так и для не существующего.
# Если нужен гарантированно новый отчёт -- нужно использовать
# метод get_new_report (он возбудит исключение, если такой отчёт
# уже есть в системе).
# Аналогично для гарантированно существующих отчётов есть метод get_old_report.

# можно локально настроить желаемый конфиг
config_in_yaml = u"""
---
dimensions:
- fielddate: date
- project: string
measures:
- value: number
titles:
  value: Значение
title: Отчёт
"""
new_config = statface_client.StatfaceReportConfig(
    title=u'Очень важный отчёт',
    user_config=config_in_yaml)
# никаких обращений к серверу до этого не происходило

# чтобы обновить конфиг на сервере нужно явно залить его туда
report.upload_config(new_config)
# этот запрос может падать при некорректных изменениях конфига

# Eсли на сервере отчёт уже существует, можно можно посмотреть его конфиг
# (конфиг скачается с сервера):
print(report.config)
# в этом атрибуте появится объект StatfaceReportConfig, заполненный полями
# конфига с сервера

# чуть более низкоуровневый способ получить конфиг -- report.download_config()
# вернёт словарик с конфигом
# в report.upload_config также можно передавать словарь

# можно работать с конфигом и как со словарём
config_in_dict = report.download_config()
print(config_in_dict['fields'])
report.upload_config(config_in_dict)

my_data = [
    {'fielddate': '2015-02-03', 'project': 'Portal', 'value': 2},
    {'fielddate': '2015-02-03', 'project': 'Morda', 'value': 3},
    {'fielddate': '2015-02-04', 'project': 'Portal', 'value': 5},
    {'fielddate': '2015-02-04', 'project': 'Morda', 'value': 6},
]

report.upload_data(scale='daily', data=my_data)

# единственный обязательный параметр выгрузки данных -- scale.
# для ограничения нагрузки на сервер
# данные по умолчанию скачиваются за ограниченный период
data = report.download_data(scale='daily')
# но диапозон дат можно изменить параметром  _period_distance
# или параметрами date_min + date_max
data = report.download_data(scale='daily', _period_distance=2)
# по умолчанию выгружаемые записи десериализуются в словарики
for record in data[:5]:
    print(record.items())
# По умолчанию Statface отдаёт данные в json, если это медленно --
# стоит указать параметр format=tskv (необходим пакет statbox_bindings2).
# Можно выгружать данные в сериализованном виде.
# Сериализованные данные можно выгружать также в форматах csv и tsv
print(report.download_data(
    scale='daily',
    raw=True,
    format='csv',
    _period_distance=1,
))
