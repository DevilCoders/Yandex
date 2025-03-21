# Управление хостами кластера

{% if product == "cloud-il" %}

{% include [one-az-disclaimer](../../_includes/overview/one-az-disclaimer.md) %}

{% endif %}

Вы можете добавлять и удалять хосты кластера, а также управлять настройками {{ CH }} для отдельных кластеров.

{% note warning %}

Если вы создали кластер без поддержки [{{ CK }}](../concepts/replication.md#ck), то прежде чем добавлять новые хосты в любой из [шардов](../concepts/sharding.md), [включите отказоустойчивость](./zk-hosts.md#add-zk) с использованием хостов {{ ZK }}.

{% endnote %}


## Получить список хостов в кластере {#list-hosts}

{% list tabs %}

- Консоль управления

  1. Перейдите на [страницу каталога]({{ link-console-main }}) и выберите сервис **{{ mch-name }}**.
  1. Нажмите на имя нужного кластера, затем выберите вкладку **Хосты**.
  
- CLI
  
  {% include [cli-install](../../_includes/cli-install.md) %}
  
  {% include [default-catalogue](../../_includes/default-catalogue.md) %}
  
  Чтобы получить список хостов в кластере, выполните команду:
  
  ```bash
  {{ yc-mdb-ch }} host list \
     --cluster-name=<имя кластера>
  ```

  {% if audience == "external" %}

  ```text
  +----------------------------+--------------+---------+--------+---------------+
  |            NAME            |  CLUSTER ID  |  ROLE   | HEALTH |    ZONE ID    |
  +----------------------------+--------------+---------+--------+---------------+
  | rc1b...{{ dns-zone }} | c9qp71dk1... | MASTER  | ALIVE  | {{ region-id }}-b |
  | rc1a...{{ dns-zone }} | c9qp71dk1... | REPLICA | ALIVE  | {{ region-id }}-a |
  +----------------------------+--------------+---------+--------+---------------+
  ```

  {% else %}

  ```text
  +----------------------+--------------+---------+--------+---------------+
  |         NAME         |  CLUSTER ID  |  ROLE   | HEALTH |    ZONE ID    |
  +----------------------+--------------+---------+--------+---------------+
  | rc1b...{{ dns-zone }} | c9qp71dk1... | MASTER  | ALIVE  | {{ region-id }}-b |
  | rc1a...{{ dns-zone }} | c9qp71dk1... | REPLICA | ALIVE  | {{ region-id }}-a |
  +----------------------+--------------+---------+--------+---------------+
  ```

  {% endif %}

  Имя кластера можно запросить со [списком кластеров в каталоге](cluster-list.md#list-clusters).

- API
  
  Воспользуйтесь методом API [listHosts](../api-ref/Cluster/listHosts.md) и передайте идентификатор кластера в параметре `clusterId` запроса.

  Чтобы узнать идентификатор кластера, [получите список кластеров в каталоге](cluster-list.md).
  
{% endlist %}


## Добавить хост {#add-host}

Количество хостов в кластерах {{ mch-short-name }} ограничено квотами на количество CPU и объем памяти, которые доступны кластерам БД в вашем облаке. Чтобы проверить используемые ресурсы, откройте страницу [Квоты]({{ link-console-quotas }}) и найдите блок **Managed Databases**.

{% list tabs %}

- Консоль управления
  
  1. Перейдите на [страницу каталога]({{ link-console-main }}) и выберите сервис **{{ mch-name }}**.
  1. Нажмите на имя нужного кластера и перейдите на вкладку **Хосты**. 
  1. Нажмите кнопку ![image](../../_assets/plus-sign.svg) **Добавить хост**.
  
  {% if audience != "internal" %}

  1. Укажите параметры хоста:

      * зону доступности;
      * подсеть (если нужной подсети в списке нет, [создайте ее](../../vpc/operations/subnet-create.md));
      * выберите опцию **Публичный доступ**, если хост должен быть доступен извне {{ yandex-cloud }};
      * имя шарда;
      * выберите опцию **Копировать схему данных**, чтобы скопировать схему со случайной реплики на новый хост.

  {% endif %}

- CLI
  
  {% include [cli-install](../../_includes/cli-install.md) %}
  
  {% include [default-catalogue](../../_includes/default-catalogue.md) %}
  
  Чтобы добавить хост в кластер:
  
  {% if audience != "internal" %}

  1. Запросите список подсетей кластера, чтобы выбрать подсеть для нового хоста:
  
      ```bash
      yc vpc subnet list
      
      +-----------+-----------+------------+---------------+------------------+
      |     ID    |   NAME    | NETWORK ID |     ZONE      |      RANGE       |
      +-----------+-----------+------------+---------------+------------------+
      | b0cl69... | default-c | enp6rq7... | {{ region-id }}-c | [172.16.0.0/20]  |
      | e2lkj9... | default-b | enp6rq7... | {{ region-id }}-b | [10.10.0.0/16]   |
      | e9b0ph... | a-2       | enp6rq7... | {{ region-id }}-a | [172.16.32.0/20] |
      | e9b9v2... | default-a | enp6rq7... | {{ region-id }}-a | [172.16.16.0/20] |
      +-----------+-----------+------------+---------------+------------------+
      ```
  
      Если нужной подсети в списке нет, [создайте ее](../../vpc/operations/subnet-create.md). 
  
  {% endif %} 
  
  1. Посмотрите описание команды CLI для добавления хостов:
  
     ```bash
     {{ yc-mdb-ch }} host add --help
     ``` 
  
  1. Выполните команду добавления хоста:
  
     {% if audience != "internal" %}

     ```bash
     {{ yc-mdb-ch }} host add \
        --cluster-name=<имя кластера> \
        --host zone-id=<зона доступности>,`
              `subnet-id=<идентификатор подсети>,`
              `assign-public-ip=<публичный доступ к хосту: true или false>,`
              `shard-name=<имя шарда>,`
              `type=clickhouse
     ```

     {% else %}

     ```bash
     {{ yc-mdb-ch }} host add \
        --cluster-name=<имя кластера> \
        --host zone-id=<зона доступности>,`
              `subnet-id=<идентификатор подсети>,`
              `shard-name=<имя шарда>`
              `type=clickhouse
     ```

     {% endif %}

     Чтобы скопировать схему данных со случайной реплики на новый хост, укажите необязательный параметр `--copy-schema`.

     {{ mch-short-name }} запустит операцию добавления хоста.

     {% if audience != "internal" %}

     Идентификатор подсети необходимо указать, если в зоне доступности больше одной подсети, в противном случае {{ mch-short-name }} автоматически выберет единственную подсеть. Имя кластера можно запросить со [списком кластеров в каталоге](cluster-list.md#list-clusters).

     {% else %}

     Имя кластера можно запросить со [списком кластеров в каталоге](cluster-list.md#list-clusters).

     {% endif %}

- {{ TF }}

    1. Откройте актуальный конфигурационный файл {{ TF }} с планом инфраструктуры.

        О том, как создать такой файл, см. в разделе [{#T}](cluster-create.md).

    1. Добавьте к описанию кластера {{ mch-name }} блок `host`.

        ```hcl
        resource "yandex_mdb_clickhouse_cluster" "<имя кластера>" {
          ...
          host {
            type             = "CLICKHOUSE"
            zone             = "<зона доступности>"
            subnet_id        = "<идентификатор подсети>"
            assign_public_ip = <публичный доступ к хосту: true или false>
          }
        }
        ```

    1. Проверьте корректность настроек.

        {% include [terraform-validate](../../_includes/mdb/terraform/validate.md) %}

    1. Подтвердите изменение ресурсов.

        {% include [terraform-apply](../../_includes/mdb/terraform/apply.md) %}

    Подробнее см. в [документации провайдера {{ TF }}]({{ tf-provider-link }}/mdb_clickhouse_cluster).

    {% include [Terraform timeouts](../../_includes/mdb/mch/terraform/timeouts.md) %}

- API

  Воспользуйтесь методом API [addHosts](../api-ref/Cluster/addHosts.md) и передайте в запросе:

  * Идентификатор кластера в параметре `clusterId`. Чтобы узнать идентификатор, [получите список кластеров в каталоге](cluster-list.md#list-clusters).
  * Настройки нового хоста в одном или нескольких параметрах `hostSpecs`.

  Чтобы скопировать схему данных со случайной реплики на новый хост, передайте в запросе параметр `copySchema` со значением `true`.

{% endlist %}

{% note warning %}

Если после добавления хоста к нему невозможно [подключиться](connect.md), убедитесь, что [группа безопасности](../concepts/network.md#security-groups) кластера настроена корректно для подсети, в которую помещен хост.

Используйте опцию копирования схемы данных только в том случае, когда схема одинакова на всех хостах-репликах кластера.

{% endnote %}

## Изменить хост {#update}

Для каждого хоста в кластере {{ mch-short-name }} можно изменить настройки публичного доступа.

{% list tabs %}

- Консоль управления

  Чтобы изменить параметры хоста в кластере:

    1. Перейдите на [страницу каталога]({{ link-console-main }}) и выберите сервис **{{ mch-name }}**.
    1. Нажмите на имя нужного кластера и выберите вкладку **Хосты**.
    1. Нажмите на значок ![image](../../_assets/options.svg) в строке нужного хоста и выберите пункт **Редактировать**.
    1. Включите опцию **Публичный доступ**, если хост должен быть доступен извне {{ yandex-cloud }}.
    1. Нажмите кнопку **Сохранить**.

- CLI

    {% include [cli-install](../../_includes/cli-install.md) %}

    {% include [default-catalogue](../../_includes/default-catalogue.md) %}

    Чтобы изменить параметры хоста, выполните команду:

    ```bash
    {{ yc-mdb-ch }} host update <имя хоста> \
       --cluster-name=<имя кластера> \
       --assign-public-ip=<публичный доступ к хосту: true или false>
    ```

    Имя хоста можно запросить со [списком хостов в кластере](#list-hosts), имя кластера — со [списком кластеров в каталоге](cluster-list.md#list-clusters).

{% if audience == "draft" %}

- {{ TF }}

    Чтобы изменить параметры хоста в кластере:

    1. Откройте актуальный конфигурационный файл {{ TF }} с планом инфраструктуры.

        О том, как создать такой файл, см. в разделе [{#T}](cluster-create.md).

    1. Измените в описании кластера {{ mch-name }} атрибуты блока `host`, соответствующего изменяемому хосту.

        ```hcl
        resource "yandex_mdb_clickhouse_cluster" "<имя кластера>" {
          ...
          host {
            assign_public_ip = <публичный доступ к хосту: true или false>
          }
        }
        ```

    1. Проверьте корректность настроек.

        {% include [terraform-validate](../../_includes/mdb/terraform/validate.md) %}

    1. Подтвердите изменение ресурсов.

        {% include [terraform-apply](../../_includes/mdb/terraform/apply.md) %}

    Подробнее см. в [документации провайдера {{ TF }}]({{ tf-provider-mch }}).

{% endif %}

- API

    Чтобы изменить параметры хоста, воспользуйтесь методом API [updateHosts](../api-ref/Cluster/updateHosts.md) и передайте в запросе:

    - Идентификатор кластера, в котором нужно изменить хост, в параметре `clusterId`. Чтобы узнать идентификатор, получите [список кластеров в каталоге](cluster-list.md#list-clusters).
    - Имя хоста, который нужно изменить, в параметре `updateHostSpecs.hostName`. Чтобы узнать имя, получите [список хостов в кластере](#list-hosts).
    - Настройки публичного доступа к хосту в параметре `updateHostSpecs.assignPublicIp`.
    - Список полей конфигурации кластера, подлежащих изменению (в данном случае — `assignPublicIp`), в параметре `updateMask`.

    {% include [Note API updateMask](../../_includes/note-api-updatemask.md) %}

{% endlist %}

{% note warning %}

Если после изменения хоста к нему невозможно [подключиться](connect.md), убедитесь, что [группа безопасности](../concepts/network.md#security-groups) кластера настроена корректно для подсети, в которую помещен хост.

{% endnote %}

## Удалить хост {#remove-host}

Вы можете удалить хост из {{ CH }}-кластера, если в кластере 3 и более хостов.

{% note info %}

Кластер, созданный с поддержкой механизма репликации [{{ CK }}](../concepts/replication.md#ck), должен состоять из трех или более хостов.

{% endnote %}

{% list tabs %}

- Консоль управления
  
  1. Перейдите на [страницу каталога]({{ link-console-main }}) и выберите сервис **{{ mch-name }}**.
  1. Нажмите на имя нужного кластера и выберите вкладку **Хосты**.
  1. Нажмите на значок ![image](../../_assets/options.svg) в строке нужного хоста и выберите пункт **Удалить**.
  
- CLI
  
  {% include [cli-install](../../_includes/cli-install.md) %}
  
  {% include [default-catalogue](../../_includes/default-catalogue.md) %}
  
  Чтобы удалить хост из кластера, выполните команду:
  
  ```bash
  {{ yc-mdb-ch }} host delete <имя хоста> \
     --cluster-name=<имя кластера>
  ```
  
  Имя хоста можно запросить со [списком хостов в кластере](#list-hosts), имя кластера — со [списком кластеров в каталоге](cluster-list.md#list-clusters).

- {{ TF }}

    1. Откройте актуальный конфигурационный файл {{ TF }} с планом инфраструктуры.

        О том, как создать такой файл, см. в разделе [{#T}](cluster-create.md).

    1. Удалите из описания кластера {{ mch-name }} блок `host` с типом `CLICKHOUSE`.

    1. Проверьте корректность настроек.

        {% include [terraform-validate](../../_includes/mdb/terraform/validate.md) %}

    1. Подтвердите удаление ресурсов.

        {% include [terraform-apply](../../_includes/mdb/terraform/apply.md) %}

    Подробнее см. в [документации провайдера {{ TF }}]({{ tf-provider-link }}/mdb_clickhouse_cluster).

    {% include [Terraform timeouts](../../_includes/mdb/mch/terraform/timeouts.md) %}

- API

  Воспользуйтесь методом API [deleteHosts](../api-ref/Cluster/deleteHosts.md) и передайте в запросе:
  
  * Идентификатор кластера в параметре `clusterId`. Чтобы узнать идентификатор, [получите список кластеров в каталоге](cluster-list.md#list-clusters).
  * Имя или массив имен удаляемых хостов в параметре `hostNames`.

{% endlist %}
