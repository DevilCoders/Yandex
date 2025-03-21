---
title: Управление хостами Elasticsearch
description: 'Вы можете получать список хостов в кластере Elasticsearch, а также добавлять и удалять хосты кластера. Вы можете управлять хостами только с ролью Data Node.'
keywords:
  - управление хостами Elasticsearch
  - хосты Elasticsearch
  - Elasticsearch
---

# Управление хостами

Вы можете получить список хостов в кластере {{ ES }}, а также добавлять и удалять хосты кластера.

{% note info %}

Вы можете добавлять и удалять только хосты с ролью [_Data Node_](../concepts/index.md).

{% endnote %}

## Получить список хостов в кластере {#list-hosts}

{% list tabs %}

- Консоль управления

  1. В [консоли управления]({{ link-console-main }}) перейдите на страницу каталога и выберите сервис **{{ mes-name }}**.
  1. Нажмите на имя нужного кластера, затем выберите вкладку **Хосты**.

- CLI

    {% include [cli-install](../../_includes/cli-install.md) %}

    {% include [default-catalogue](../../_includes/default-catalogue.md) %}

    Чтобы получить список хостов в кластере, выполните команду:

    ```bash
    {{ yc-mdb-es }} host list --cluster-name <имя кластера>
    ```

    Имя кластера можно запросить со [списком кластеров в каталоге](cluster-list.md#list-clusters).

- API

  Воспользуйтесь методом API `listHosts`: передайте значение идентификатора требуемого кластера в параметре `clusterId` запроса.

  Чтобы узнать идентификатор кластера, [получите список кластеров в каталоге](cluster-list.md#list-clusters).

{% endlist %}

## Добавить хосты в кластер {#add-hosts}

{% note warning %}

Назначить публичный IP-адрес хосту после его создания невозможно.

{% endnote %}

{% list tabs %}

- Консоль управления

    1. В [консоли управления]({{ link-console-main }}) перейдите на страницу каталога и выберите сервис **{{ mes-name }}**.
    1. Нажмите на имя нужного кластера, затем выберите вкладку **Хосты**.
    1. Нажмите кнопку **Добавить хост**.
    1. Укажите параметры хоста:

        * Зону доступности.
        * {% if audience != "internal" %}Подсеть (если нужной подсети в списке нет, [создайте ее](../../vpc/operations/subnet-create.md)).{% else %}Подсеть (если нужной подсети в списке нет, создайте ее).{% endif %}
        * Выберите опцию **Публичный доступ**, если хост должен быть доступен извне {{ yandex-cloud }}.

- CLI

    {% include [cli-install](../../_includes/cli-install.md) %}

    {% include [default-catalogue](../../_includes/default-catalogue.md) %}

    Чтобы добавить хосты в кластер, выполните команду:

    ```bash
    {{ yc-mdb-es }} host add \
       --cluster-name <имя кластера> \
       --host zone-id=<зона доступности>,subnet-name=<имя подсети>,assign-public-ip=<true или false>,type=<роль хоста: datanode или masternode>
    ```

    Имя кластера можно запросить со [списком кластеров в каталоге](cluster-list.md#list-clusters).

- API

  Воспользуйтесь методом API `addHosts`: передайте значение идентификатора требуемого кластера в параметре `clusterId` запроса.

  Чтобы узнать идентификатор кластера, [получите список кластеров в каталоге](cluster-list.md#list-clusters).

  Добавьте требуемое количество параметров `hostSpecs` с настройками хоста (по одному параметру на каждый добавляемый хост).

{% endlist %}

{% note warning %}

Если после добавления хоста к нему невозможно [подключиться](cluster-connect.md), убедитесь, что [группа безопасности](../concepts/network.md#security-groups) кластера настроена корректно для подсети, в которую помещен хост.

{% endnote %}

## Удалить хосты из кластера {#delete-hosts}

При удалении хостов действуют ограничения:

* Нельзя удалить единственный хост с ролью _Data node_.
* Для кластеров, состоящих из нескольких хостов с ролью _Data node_, нельзя удалить последние два хоста.

{% list tabs %}

- Консоль управления

    1. В [консоли управления]({{ link-console-main }}) перейдите на страницу каталога и выберите сервис **{{ mes-name }}**.
    1. Нажмите на имя нужного кластера, затем выберите вкладку **Хосты**.
    1. Нажмите на значок ![image](../../_assets/options.svg) в строке нужного хоста и выберите пункт **Удалить**.

- CLI

    {% include [cli-install](../../_includes/cli-install.md) %}

    {% include [default-catalogue](../../_includes/default-catalogue.md) %}

    Чтобы удалить хост из кластера, выполните команду:

    ```bash
    {{ yc-mdb-es }} host delete <имя хоста> --cluster-name <имя кластера>
    ```

    Имя хоста можно запросить со [списком хостов в кластере](#list-hosts), имя кластера — со [списком кластеров в каталоге](cluster-list.md#list-clusters).

- API

  Воспользуйтесь методом API `deleteHosts`: передайте значение идентификатора требуемого кластера в параметре `clusterId` запроса.

  Чтобы узнать идентификатор кластера, [получите список кластеров в каталоге](cluster-list.md#list-clusters).

  В одном или более параметре `hostNames[]` укажите имена хостов, которые вы хотите удалить из кластера.

{% endlist %}
