### Хранилище и резервные копии {#prices-storage}

Услуга | Цена за ГБ в месяц
----- | -----
Хранение данных на группах хранения из SSD-накопителей | {{ sku|KZT|ydb.cluster.v1.ssd|month|string }}
Хранение резервных копий по требованию в {{ objstorage-full-name }} | 10,05 ₸

{% note info "Минимальный размер группы" %}

Одна [группа хранения](../../ydb/concepts/resources.md#storage-groups) позволяет разместить до 100 Гб пользовательских данных. Минимальная гранулярность выделения места для базы данных – одна группа хранения.

{% endnote %}
