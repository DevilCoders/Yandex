# Управление хостами кластера

{% if product == "cloud-il" %}

{% include [one-az-disclaimer](../../_includes/overview/one-az-disclaimer.md) %}

{% endif %}

Вы можете добавлять и удалять хосты кластера, а также управлять их настройками.

## Получить список хостов в кластере {#list}

{% list tabs %}

- Консоль управления

  1. Перейдите на [страницу каталога]({{ link-console-main }}) и выберите сервис **{{ mmy-name }}**.
  1. Нажмите на имя нужного кластера, затем выберите вкладку **Хосты**.

- CLI

  {% include [cli-install](../../_includes/cli-install.md) %}

  {% include [default-catalogue](../../_includes/default-catalogue.md) %}

  Чтобы получить список хостов в кластере, выполните команду:

  ```bash
  {{ yc-mdb-my }} host list \
     --cluster-name=<имя кластера>
  ```
  
  Результат:

  {% if audience == "external" %}

  ```text
  +----------------------------+--------------+---------+--------+---------------+
  |            NAME            |  CLUSTER ID  |  ROLE   | HEALTH |    ZONE ID    |
  +----------------------------+--------------+---------+--------+---------------+
  | rc1b...{{ dns-zone }} | c9q5k4ve7... | MASTER  | ALIVE  | {{ region-id }}-b |
  | rc1a...{{ dns-zone }} | c9q5k4ve7... | REPLICA | ALIVE  | {{ region-id }}-a |
  +----------------------------+--------------+---------+--------+---------------+
  ```

  {% else %}

  ```text
  +----------------------+--------------+---------+--------+---------------+
  |         NAME         |  CLUSTER ID  |  ROLE   | HEALTH |    ZONE ID    |
  +----------------------+--------------+---------+--------+---------------+
  | rc1b...{{ dns-zone }} | c9q5k4ve7... | MASTER  | ALIVE  | {{ region-id }}-b |
  | rc1a...{{ dns-zone }} | c9q5k4ve7... | REPLICA | ALIVE  | {{ region-id }}-a |
  +----------------------+--------------+---------+--------+---------------+
  ```

  {% endif %}

  Имя кластера можно запросить со [списком кластеров в каталоге](cluster-list.md#list-clusters).

- API

  Воспользуйтесь методом API [listHosts](../api-ref/Cluster/listHosts.md) и передайте идентификатор кластера в параметре `clusterId` запроса.

  Чтобы узнать идентификатор кластера, [получите список кластеров в каталоге](cluster-list.md).

{% endlist %}

## Добавить хост {#add}

Количество хостов в кластерах {{ mmy-short-name }} ограничено квотами на количество CPU и объем памяти, которые доступны кластерам БД в вашем облаке. Чтобы проверить используемые ресурсы, откройте страницу [Квоты]({{ link-console-quotas }}) и найдите блок **Managed Databases**.

{% list tabs %}

- Консоль управления

  1. Перейдите на [страницу каталога]({{ link-console-main }}) и выберите сервис **{{ mmy-name }}**.
  1. Нажмите на имя нужного кластера и перейдите на вкладку **Хосты**.
  1. Нажмите кнопку **Добавить хост**.
  1. Укажите параметры хоста:

     {% if audience != "internal" %}

     * зону доступности;
     * подсеть (если нужной подсети в списке нет, [создайте ее](../../vpc/operations/subnet-create.md));
     {% else %}
     * подсеть (если нужной подсети в списке нет, создайте ее);
     {% endif %}
     * выберите опцию **Публичный доступ**, если хост должен быть доступен извне {{ yandex-cloud }};
     * приоритет назначения хоста мастером;
     * приоритет хоста как {{ MY }}-реплики для создания резервной копии.

- CLI

  {% include [cli-install](../../_includes/cli-install.md) %}

  {% include [default-catalogue](../../_includes/default-catalogue.md) %}

  Чтобы добавить хост в кластер:

  1. Запросите список подсетей кластера, чтобы выбрать подсеть для нового хоста:

      ```
      yc vpc subnet list
      ```

      Результат:

      ```
      +-----------+-----------+------------+---------------+------------------+
      |     ID    |   NAME    | NETWORK ID |     ZONE      |      RANGE       |
      +-----------+-----------+------------+---------------+------------------+
      | b0cl69... | default-c | enp6rq7... | {{ region-id }}-c | [172.16.0.0/20]  |
      | e2lkj9... | default-b | enp6rq7... | {{ region-id }}-b | [10.10.0.0/16]   |
      | e9b0ph... | a-2       | enp6rq7... | {{ region-id }}-a | [172.16.32.0/20] |
      | e9b9v2... | default-a | enp6rq7... | {{ region-id }}-a | [172.16.16.0/20] |
      +-----------+-----------+------------+---------------+------------------+
      ```
      {% if audience != "internal" %}

      Если нужной подсети в списке нет, [создайте ее](../../vpc/operations/subnet-create.md). 

      {% else %}

      Если нужной подсети в списке нет, создайте ее.

      {% endif %}

  1. Посмотрите описание команды CLI для добавления хостов:

     ```
     {{ yc-mdb-my }} host add --help
     ```

  1. Выполните команду добавления хоста (в примере приведены не все доступные параметры):

     ```bash
     {{ yc-mdb-my }} host add \
       --cluster-name=<имя кластера> \
       --host zone-id=<идентификатор зоны доступности>,
       --subnet-id=<идентификатор подсети>,
       --backup-priority=<приоритет хоста при резервном копировании>
       --priority=<приоритет назначения хоста мастером: от 0 до 100>

     ```

     {{ mmy-short-name }} запустит операцию добавления хоста.

     Идентификатор подсети необходимо указать, если в зоне доступности больше одной подсети, в противном случае {{ mmy-short-name }} автоматически выберет единственную подсеть. Имя кластера можно запросить со [списком кластеров в каталоге](cluster-list.md#list-clusters).

- {{ TF }}

  1. Откройте актуальный конфигурационный файл {{ TF }} с планом инфраструктуры.

      О том, как создать такой файл, см. в разделе [{#T}](cluster-create.md).

  1. Добавьте к описанию кластера {{ mmy-name }} блок `host`:

      ```hcl
      resource "yandex_mdb_mysql_cluster" "<имя кластера>" {
        ...
        host {
          zone             = "<зона доступности>"
          subnet_id        = <идентификатор подсети>
          assign_public_ip = <публичный доступ к хосту: true или false>
          priority         = <приоритет назначения хоста мастером: от 0 до 100>
          ...
        }
      }
      ```

  1. Проверьте корректность настроек.

      {% include [terraform-validate](../../_includes/mdb/terraform/validate.md) %}

  1. Подтвердите изменение ресурсов.

      {% include [terraform-apply](../../_includes/mdb/terraform/apply.md) %}

  Подробнее см. в [документации провайдера {{ TF }}]({{ tf-provider-mmy }}).

  {% include [Terraform timeouts](../../_includes/mdb/mmy/terraform/timeouts.md) %}

- API

    Воспользуйтесь методом API [addHosts](../api-ref/Cluster/addHosts.md) и передайте в запросе:

    * Идентификатор кластера в параметре `clusterId`. Чтобы узнать идентификатор, [получите список кластеров в каталоге](cluster-list.md).
    * Настройки нового хоста в одном или нескольких параметрах `hostSpecs`.

{% endlist %}

{% note warning %}

Если после добавления хоста к нему невозможно [подключиться](connect.md), убедитесь, что [группа безопасности](../concepts/network.md#security-groups) кластера настроена корректно для подсети, в которую помещен хост.

{% endnote %}

## Изменить хост {#update}

Для каждого хоста в кластере {{ mmy-short-name }} можно:

* указать [источник репликации](../concepts/replication.md#manual-source);
* управлять [публичным доступом](../concepts/network.md#public-access-to-host);
* задать [приоритет использования](../concepts/backup.md#size) при резервном копировании;
* задать приоритет назначения хоста мастером при [выходе из строя основного мастера](../concepts/replication.md#master-failover).

{% list tabs %}

- Консоль управления

  Чтобы изменить параметры хоста в кластере:

    1. Перейдите на [страницу каталога]({{ link-console-main }}) и выберите сервис **{{ mmy-name }}**.
    1. Нажмите на имя нужного кластера и выберите вкладку **Хосты**.
    1. Нажмите значок ![image](../../_assets/horizontal-ellipsis.svg) в строке нужного хоста и выберите пункт **Редактировать**.
    1. Задайте новые настройки для хоста:
        1. Выберите источник репликации для хоста, чтобы вручную управлять потоками репликации.
        1. Включите опцию **Публичный доступ**, если хост должен быть доступен извне {{ yandex-cloud }}.
        1. Задайте значение поля **Приоритет мастера**.
        1. Задайте значение поля **Приоритет создания бэкапа**.
    1. Нажмите кнопку **Сохранить**.

- CLI

  {% include [cli-install](../../_includes/cli-install.md) %}

  {% include [default-catalogue](../../_includes/default-catalogue.md) %}

  Чтобы изменить параметры хоста, выполните команду (в примере приведены не все доступные параметры):

  ```bash
  {{ yc-mdb-my }} host update <имя хоста> \
    --cluster-name=<имя кластера> \
    --replication-source=<имя хоста-источника> \
    --assign-public-ip=<публичный доступ к хосту: true или false> \
    --backup-priority=<приоритет хоста при резервном копировании: от 0 до 100>
    --priority=<приоритет назначения хоста мастером: от 0 до 100>

  ```

  Где:

  * `--cluster-name` — имя кластера {{ mmy-name }};
  * `--replication-source` — источник [репликации](../concepts/replication.md) для хоста;
  * `--assign-public-ip` — доступность хоста из интернета по публичному IP-адресу;
  * `--backup-priority` — приоритет хоста при [резервном копировании](../concepts/backup.md#size).
  * `--priority` — приоритет назначения хоста мастером при [выходе из строя основного мастера](../concepts/replication.md#master-failover).

  Имя хоста можно запросить со [списком хостов в кластере](#list), имя кластера — со [списком кластеров в каталоге](cluster-list.md#list-clusters).

- {{ TF }}

  Чтобы изменить параметры хоста в кластере:

    1. Откройте актуальный конфигурационный файл {{ TF }} с планом инфраструктуры.

       О том, как создать такой файл, см. в разделе [{#T}](cluster-create.md).

    1. Измените в описании кластера {{ mmy-name }} атрибуты блока `host`, соответствующего изменяемому хосту.

        ```hcl
        resource "yandex_mdb_mysql_cluster" "<имя кластера>" {
          ...
          host {
            replication_source_name = "<источник репликации>"
            assign_public_ip        = <публичный доступ к хосту: true или false>
            priority                = <приоритет назначения хоста мастером: от 0 до 100>
          }
        }
        ```

    1. Проверьте корректность настроек.

       {% include [terraform-validate](../../_includes/mdb/terraform/validate.md) %}

    1. Подтвердите изменение ресурсов.

       {% include [terraform-apply](../../_includes/mdb/terraform/apply.md) %}

  Подробнее см. в [документации провайдера {{ TF }}]({{ tf-provider-mmy }}).

  {% include [Terraform timeouts](../../_includes/mdb/mmy/terraform/timeouts.md) %}

- API

    Чтобы изменить параметры хоста, воспользуйтесь методом API [updateHosts](../api-ref/Cluster/updateHosts.md) и передайте в запросе:

    * Идентификатор кластера в параметре `clusterId`. Чтобы узнать идентификатор, [получите список кластеров в каталоге](./cluster-list.md#list-clusters).

    * Массив настроек изменяемых хостов в параметре `updateHostSpecs`.

        Для каждого хоста укажите:

        * имя в поле `hostName`;
        * список настроек, которые необходимо изменить, в параметре `updateMask`.

    {% include [Note API updateMask](../../_includes/note-api-updatemask.md) %}

{% endlist %}

{% note warning %}

Если после изменения хоста к нему невозможно [подключиться](connect.md), убедитесь, что [группа безопасности](../concepts/network.md#security-groups) кластера настроена корректно для подсети, в которую помещен хост.

{% endnote %}

## Удалить хост {#remove}

Вы можете удалить хост из {{ MY }}-кластера, если он не является единственным хостом. Чтобы заменить единственный хост, сначала создайте новый хост, а затем удалите старый.

Если хост является мастером в момент удаления, {{ mmy-short-name }} автоматически назначит мастером следующую по приоритету реплику.

{% list tabs %}

- Консоль управления

  1. Перейдите на [страницу каталога]({{ link-console-main }}) и выберите сервис **{{ mmy-name }}**.
  1. Нажмите на имя нужного кластера и выберите вкладку **Хосты**.
  1. Нажмите значок ![image](../../_assets/horizontal-ellipsis.svg) в строке нужного хоста и выберите пункт **Удалить**.

- CLI

  {% include [cli-install](../../_includes/cli-install.md) %}

  {% include [default-catalogue](../../_includes/default-catalogue.md) %}

  Чтобы удалить хост из кластера, выполните команду:

  ```bash
  {{ yc-mdb-my }} host delete <имя хоста> \
     --cluster-name=<имя кластера>
  ```

  Имя хоста можно запросить со [списком хостов в кластере](#list), имя кластера — со [списком кластеров в каталоге](cluster-list.md#list-clusters).

- {{ TF }}

  1. Откройте актуальный конфигурационный файл {{ TF }} с планом инфраструктуры.

      О том, как создать такой файл, см. в разделе [{#T}](cluster-create.md).

  1. Удалите из описания кластера {{ mmy-name }} блок `host`.

  1. Проверьте корректность настроек.

      {% include [terraform-validate](../../_includes/mdb/terraform/validate.md) %}

  1. Подтвердите удаление ресурсов.

      {% include [terraform-apply](../../_includes/mdb/terraform/apply.md) %}

  Подробнее см. в [документации провайдера {{ TF }}]({{ tf-provider-mmy }}).

  {% include [Terraform timeouts](../../_includes/mdb/mmy/terraform/timeouts.md) %}

- API

    Воспользуйтесь методом API [deleteHosts](../api-ref/Cluster/deleteHosts.md) и передайте в запросе:

    * Идентификатор кластера в параметре `clusterId`. Чтобы узнать идентификатор, [получите список кластеров в каталоге](cluster-list.md).
    * Имя или массив имен удаляемых хостов в параметре `hostNames`.

{% endlist %}
