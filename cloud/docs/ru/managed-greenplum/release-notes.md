# История изменений в {{ mgp-full-name }}

{% include [Tags](../_includes/mdb/release-notes-tags.md) %}

## 01.07.2022 {#01.07.2022}

* Добавлена поддержка CLI: доступны команды `{{ yc-mdb-gp }} cluster` и `{{ yc-mdb-gp }} hosts`. {{ tag-cli }}

## 01.06.2022 {#01.06.2022}

* Добавлена поддержка расширения [pgcrypto](https://gpdb.docs.pivotal.io/6-9/ref_guide/modules/pgcrypto.html).

## 01.05.2022 {#01.05.2022}

* Добавлена возможность миграции базы данных из/в {{ mgp-name }} с помощью сервиса {{ data-transfer-full-name }}. Доступные приемники и источники приведены в [документации](../data-transfer/concepts/index.md#connectivity-matrix). Функциональность находится в стадии [Preview](../data-transfer/concepts/index.md#greenplum).
* [Оптимизировано](https://github.com/wal-g/wal-g/pull/1257) создание резервных копий за счет особой обработки append-only сегментов.
* Добавлен модуль [diskquota](https://gpdb.docs.pivotal.io/6-19/ref_guide/modules/diskquota.html), который позволяет ограничивать схемы БД по месту на диске.
* Реализовано автоматическое переключение на резервный мастер через [gpactivatestandby](https://gpdb.docs.pivotal.io/6-3/utility_guide/ref/gpactivatestandby.html).
* Добавлены настройки `max_statement_mem` и `log_statement`, доступные при создании и изменении кластера.
* Добавлена возможность создавать кластер с нечетным количеством сегментов.
* Минимальный размер хранилища на сетевых SSD-дисках для мастер-хоста ограничен 100 ГБ.
* Оптимизирован процесс очистки (`VACUUM`):
  * операция выполняется параллельно по нескольким базам;
  * новые таблицы обрабатываются последними;
  * таблицы с активными блокировками исключаются.

## 14.03.2022 {#14.03.2022}

* {{ mgp-full-name }} перешел в общий доступ. Теперь для него действует [соглашение об уровне обслуживания]({{ link-sla-greenplum }}) (SLA) и {% if audience != "internal" %}[правила тарификации](pricing/index.md){% else %}правила тарификации{% endif %}.
{% if lang == "ru" and product == "yandex-cloud" %}* Добавлен [калькулятор](https://cloud.yandex.ru/promo/dwh-calculator/index) для расчета рекомендуемой конфигурации и оценки стоимости кластера.{% endif %}
* Доступна новая версия {{ GP }} 6.19 с исправлением известных ошибок. 
* Добавлена возможность скрывать содержимое внешних таблиц (external tables). 
* Добавлена возможность изменения размера хранилища, в том числе для локальных SSD-дисков.
* Добавлены [конфигурации](concepts/instance-types.md) `io-optimized` с увеличенным соотношением количества гигабайт RAM к количеству vCPU (8:1).

{% include [greenplum-trademark](../_includes/mdb/mgp/trademark.md) %}
