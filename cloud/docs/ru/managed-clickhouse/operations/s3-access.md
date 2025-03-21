# Настройка доступа к {{ objstorage-name }}

{{ mch-name }} поддерживает работу с {{ objstorage-full-name }} для:
* подключения [моделей машинного обучения](ml-models.md), [схем формата данных](format-schemas.md) и [собственной геобазы](internal-dictionaries.md);
* обработки данных, которые находятся в объектном хранилище, если эти данные представлены в любом из [поддерживаемых {{ CH }} форматов]({{ ch.docs }}/interfaces/formats/).

{% if audience != "internal" %}

Для доступа к данным в [бакете](../../storage/concepts/bucket.md) {{ objstorage-name }} из кластера воспользуйтесь одним из следующих способов: 
* Настройте беспарольный доступ к бакету с помощью [сервисного аккаунта](../../iam/concepts/users/service-accounts.md). Данный способ позволяет получить доступ к объекту в бакете без ввода учетных данных. Чтобы воспользоваться этим способом:
   1. [Подключите сервисный аккаунт к кластеру](#connect-service-account).
   1. [Настройте права доступа для сервисного аккаунта](#configure-acl).  
* Настройте публично доступный бакет, [открыв публичный доступ к нему](../../storage/operations/buckets/bucket-availability.md) на чтение или на запись. 

{% endif %}
   
После настройки любого из этих способов [получите ссылку на объект в бакете](#get-link-to-object), которую можно использовать при работе с данными в кластере. См. также [Примеры работы с объектами](#examples).
   
## Подключить сервисный аккаунт к кластеру {#connect-service-account}

{% if audience != "internal" %}

1. При [создании](cluster-create.md) или [изменении](update.md) кластера выберите существующий сервисный аккаунт, либо создайте новый.

{% else %}

1. При создании или изменении кластера выберите существующий сервисный аккаунт, либо создайте новый.

{% endif %}

1. Убедитесь, что этому аккаунту назначены корректные роли из группы ролей `storage.*`. При необходимости назначьте нужные роли, например, `storage.viewer` и `storage.uploader`.

{% note tip %}

Для связи кластеров {{ mch-name }} с {{ objstorage-name }} рекомендуется использовать специально созданные для этой цели сервисные аккаунты: это позволяет организовать работу с любыми бакетами, в том числе с такими, для которых предоставление публичного доступа нежелательно или невозможно.  

{% endnote %}

## Настроить права доступа {#configure-acl}

{% list tabs %}

- Консоль управления

  {% if audience != "internal" %}
  
  1. В [консоли управления]({{ link-console-main }}) выберите каталог, в котором находится нужный бакет. Если бакета не существует — [создайте](../../storage/operations/buckets/create.md) его и [наполните](../../storage/operations/objects/upload.md) необходимыми данными.

  {% else %}

  1. В [консоли управления]({{ link-console-main }}) выберите каталог, в котором находится нужный бакет. Если бакета не существует — создайте его и наполните необходимыми данными.

  {% endif %}

  1. Выберите сервис **{{ objstorage-name }}**.

  {% if audience != "internal" %}

  1. Настройте [ACL бакета](../../storage/operations/buckets/edit-acl.md) или [ACL объекта](../../storage/operations/objects/edit-acl.md):

  {% else %}

  1. Настройте ACL бакета или ACL объекта:

  {% endif %}

      1. В списке бакетов или объектов выберите нужный элемент и нажмите значок ![image](../../_assets/options.svg).
      1. Нажмите **ACL бакета** или **ACL объекта**.
      1. В выпадающем списке **Выберите пользователя** укажите сервисный аккаунт, [подключенный к кластеру](#connect-service-account).
      1. Нажмите кнопку **Добавить**.
      1. Задайте нужные разрешения для сервисного аккаунта из выпадающего списка.
      1. Нажмите кнопку **Сохранить**.

      {% note info %}

      При необходимости отзовите доступ у одного или нескольких пользователей, нажав кнопку **Отозвать** в нужной строке.
   
      {% endnote %}

{% endlist %}

## Получить ссылку на объект {#get-link-to-object}

{% if audience != "internal" %}

Чтобы работать в {{ mch-name }} с данными объекта в {{ objstorage-name }}, нужно [получить ссылку](../../storage/operations/objects/link-for-download.md) на этот объект в бакете:

{% else %}

Чтобы работать в {{ mch-name }} с данными объекта в {{ objstorage-name }}, нужно получить ссылку на этот объект в бакете:

{% endif %}

* Для бакета с ограниченным доступом ссылку вида `https://{{ s3-storage-host }}/<имя бакета>/<имя объекта>?X-Amz-Algorithm=...` приведите к виду `https://{{ s3-storage-host }}/<имя бакета>/<имя объекта>`, удалив все параметры в строке запроса. 
* Для бакета с публичным доступом ссылка будет сгенерирована сразу в нужном виде.

## Примеры работы с объектами {#examples}

[Ссылки на объекты](#get-link-to-object) вида `https://{{ s3-storage-host }}/<имя бакета>/<имя объекта>` можно использовать при работе с геометками, схемами, а также при использовании табличной функции `s3` и табличного движка `S3`.

Табличный движок `S3` аналогичен движкам [File]({{ ch.docs }}/engines/table-engines/special/file/) и [URL]({{ ch.docs }}/engines/table-engines/special/url/), за исключением того, что данные хранятся в S3-совместимом хранилище (таком как {{ objstorage-full-name }}), а не на файловой системе или удаленном HTTP/HTTPS сервере. Этот движок позволяет читать и записывать данные в хранилище с использование стандартных SQL-запросов `SELECT` и `INSERT`. 

Табличная функция `s3` предоставляет ту же самую функциональность, что и движок таблиц `S3`, но при ее использовании не требуется предварительно создавать таблицу.

Например, если в бакете с ограниченным доступом `my-bucket` {{ objstorage-name }} в файле `table.tsv` хранятся данные таблицы в формате TSV, то можно создать таблицу или функцию, которая будет работать с этим файлом. Предполагается, что настроен беспарольный доступ и получена ссылка на файл `table.tsv`.

{% list tabs %}

- Таблица S3

  1. Создайте таблицу:
  
     ```sql
     CREATE TABLE test (n Int32) ENGINE = S3('https://{{ s3-storage-host }}/my-bucket/table.tsv', 'TSV');
     ```
  
  1. Выполните тестовые запросы к таблице:
  
     ```sql
     INSERT INTO test VALUES (1);
     SELECT * FROM test;
  
     ┌─n─┐
     │ 1 │
     └───┘
     ```

- Функция s3

  1. Вставьте данные:
     
     ```sql
     INSERT INTO FUNCTION s3('https://{{ s3-storage-host }}/my-bucket/table.tsv', 'TSV', 'n Int32') VALUES (1);
     ```
     
  1. Выполните тестовый запрос:
  
     ```sql
     SELECT * FROM s3('https://{{ s3-storage-host }}/my-bucket/table.tsv', 'TSV', 'n Int32');

     ┌─n─┐
     │ 1 │
     └───┘
     ```

{% endlist %}
