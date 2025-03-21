---
title: Квоты и лимиты в {{ mpg-name }}
description: 'В {{ mpg-name }} действуют лимиты и квоты на количество кластеров, суммарное количество ядер процессора для всех хостов баз данных, суммарный объем виртуальной памяти для всех хостов баз данных, суммарный объем хранилищ для всех кластеров в одном облаке. Более подробно об ограничениях в сервисе вы узнаете из данной статьи.'

editable: false
---

{% if audience != "internal" %}

# Квоты и лимиты в {{ mpg-name }}

В сервисе {{ mpg-name }} действуют следующие ограничения:

{% include [quotes-limits-def.md](../../_includes/quotes-limits-def.md) %}

{% include [mpg-limits.md](../../_includes/mdb/mpg-limits.md) %}

{% else %}

# Технические ограничения {{ mpg-name }}

| Вид ограничения                                          | Значение                          |
|----------------------------------------------------------|-----------------------------------|
| Минимальный класс хоста                                  | s2.nano (1 vCPU, 4 ГБ RAM)        |
| Максимальный класс хоста                                 | m3-c80-m640 (80 vCPU, 640 ГБ RAM) |
| Максимальное количество хостов в одном кластере {{ PG }} | 7                                 |
| Максимальный объем хранилища для кластера {{ PG }}       | 4096 ГБ                           |

{% endif %}
