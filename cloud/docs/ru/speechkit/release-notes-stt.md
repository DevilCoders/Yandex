# Релизы распознавания

Сервис {{ speechkit-name }} предоставляет обновления в соответствии с системой моделей и версий.

Подробнее о способах распознавания речи читайте в разделе [О технологии](stt/index.md).

## Текущая версия {#current}

### Релиз 29.06.22 {#290622}

1. Многоязычная модель стала доступна в версии `general`.
1. В версиях `general:rc` и `general` многоязычная модель может принимать подсказки, какие языки присутствуют в речи.
1. В модели `general` для русского языка стали доступны изменения в `general:rc` [от 7 июня](#070622). 

## Предыдущие версии {#previous}

### Релиз 07.06.22 {#070622}

1. В модели `general:rc` улучшено качество расстановки пунктуации и распознавание фамилий.
1. Изменения [релиза от 25 апреля](#250422) доступны в модели `general`. 

### Релиз 25.04.22 {#250422}

Изменения в модели `general:rc`:

1. Улучшено распознавание слов газификация и догазификация.
1. Добавлена обратная связь сервиса при обработке формата OGG-OPUS. Если поток не является корректным аудио в формате OPUS, сервис возвращает `Invalid_Argument`.

### Релиз 19.04.22 {#190422}

1. В многоязычную модель распознавания речи добавлена поддержка турецкого языка.
1. [Новая версия API](v3/api-ref/grpc/) доступна для потокового распознавания {{ speechkit-full-name }}. Старый интерфейс также будет поддерживаться, однако все новые возможности будут доступны только в API v3.

### Релиз 14.03.22 {#140322}

[Версия `general:rc`](#020322) от 2 марта 2022 года доступна по тегу `general`.

### Релиз 02.03.22 {#020322}

Улучшенное распознавание имен, адресов и терминов, а также расстановка пунктуации в длинных предложениях и текстах, содержащих цифры, стало доступно в модели `general`.

В модель `general:rc` внесены дальнейшие изменения на основе данных пользователей.

### Релиз 17.02.22 {#170222}

В текущем релизе улучшено качество русскоязычной модели `general:rc` в следующих направлениях:

1. Распознавание фамилий, имен, отчеств и адресов.
1. Распознавание специфичных для клиентов терминов. В модель внесены данные по запросу пользователя от 1 февраля 2022 года, добавлены исправления по данным пользователя от 9 ноября 2021 года.
1. Расстановка пунктуации в длинных предложениях и текстах, содержащих цифры.

### Релиз 3.02.22 {#030222}

1. В модели `general:rc` доступен универсальный режим (язык `"auto"`). В этом режиме модель способна распознавать речь на одном из языков:
   * русский
   * казахский
   * английский
   * немецкий
   * французский
   * финский
   * шведский
   * нидерландский
   * польский
   * португальский
   * итальянский
   * испанский

1. Новые языки также доступны под своими кодами. Модель `general:rc` использует указание как подсказку для распознавания языка. При явном указании языка модель будет использовать его как подсказку для улучшения качества распознавания. В данный момент подсказка влияет только на качество распознавания русского языка.

При работе с `general:rc` рекомендуем включить [автотюнинг](stt/additional-training.md#autotuning).

_Известные проблемы_: в универсальном режиме качество распознавания может деградировать при распознавании непрерывной речи без пауз.

### Релиз 26.01.22 {#260122}

1. Модель распознавания `general` и `general:rc` для казахского языка доступна в режимах потокового и отложенного распознавания.

1. В модели `general:rc` в режимах потокового и отложенного распознавания появился пунктуатор. 

1. В режиме отложенного распознавания появилась поддержка работы с форматом [MP3]{% if lang == "ru" %}(https://ru.wikipedia.org/wiki/MP3){% else %}(https://en.wikipedia.org/wiki/MP3){% endif %}.
