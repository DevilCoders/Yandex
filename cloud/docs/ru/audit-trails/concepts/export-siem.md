# Экспорт аудитных логов в SIEM

{% if product == "yandex-cloud" %}
Файлы аудитного лога доступны для экспорта в SIEM.

Команда {{ yandex-cloud }} подготовила комплект решений для экспорта аудитных логов для нескольких SIEM. Файлы решений приведены в соответствующих разделах [репозитория {{ yandex-cloud }} Security Solution Library](https://github.com/yandex-cloud/yc-solution-library-for-security/).

[{{ yandex-cloud }} Security Solution Library](https://cloud.yandex.ru/solutions/security-solution-library) — это набор примеров и рекомендаций по построению безопасной инфраструктуры в {{ yandex-cloud }}, собранных в публичном репозитории на GitHub.

В настоящее время подготовлены решения для следующих SIEM:
* [{{ mes-full-name }}](https://github.com/yandex-cloud/yc-solution-library-for-security/tree/master/auditlogs/export-auditlogs-to-ELK_main)
* [ArcSight](https://github.com/yandex-cloud/yc-solution-library-for-security/tree/master/auditlogs/export-auditlogs-to-ArcSight)
* [Splunk](https://github.com/yandex-cloud/yc-solution-library-for-security/tree/master/auditlogs/export-auditlogs-to-Splunk)

Все решения обеспечивают непрерывную доставку файлов аудитных логов из бакетов в SIEM, а также содержат правила корреляции, дашборды и запросы для поиска важных событий безопасности. Команда {{ yandex-cloud }} поддерживает и регулярно обновляет все решения.
{% endif %}
{% if product == "cloud-il" %}
Для настройки экспорта в любые SIEM подходят утилиты [GeeseFS](../../storage/tools/geesefs.md) или [s3fs](../../storage/tools/s3fs.md). Они позволяют смонтировать бакет {{ objstorage-name }} как локальный диск виртуальной машины. Далее на ВМ необходимо установить коннектор для SIEM и настроить вычитывание JSON-файлов из бакета.
{% endif %}