# Получить информацию о виртуальной машине

Вы можете получить основную информацию о каждой созданной вами виртуальной машине в [консоли управления]({{ link-console-main }}), на странице виртуальной машины. Чтобы получить детальную информацию вместе с пользовательскими [метаданными](../../concepts/vm-metadata.md), воспользуйтесь CLI или API.

Также основную информацию и метаданные можно получить [изнутри виртуальной машины](#inside-instance).

## Получить информацию снаружи виртуальной машины {#outside-instance}

{% list tabs %}

- Консоль управления

  В разделе **Compute Cloud**, на странице **Виртуальные машины**, приводится список виртуальных машин в каталоге с краткой информацией о них.

  Для получения подробной информации о виртуальной машине нажмите на строку с ее именем.

  На вкладке:

  * **Обзор** приводится общая информация о виртуальной машине, в том числе IP-адреса, присвоенные машине.
  * **Диски** приводится информация о дисках, подключенных к виртуальной машине.
  {% if product == "yandex-cloud" %}* **Файловые хранилища** приводится информация о подключенных файловых хранилищах.{% endif %}
  * **Операции** приводится список операций с виртуальной машиной и подключенными к ней ресурсами, например дисками.
  * **Мониторинг** приводится информация о потреблении ресурсов на виртуальной машине. Эту информацию можно получить только в консоли управления или изнутри виртуальной машины.
  * **Серийная консоль** предоставлен доступ к серийной консоли, если при создании виртуальной машины вы разрешили к ней доступ.
  * **Последовательный порт** приводится информация, которую виртуальная машина выводит в последовательный порт. Чтобы получить эту информацию через API или CLI воспользуйтесь инструкцией [{#T}](get-serial-port-output.md).

- CLI

  {% include [default-catalogue](../../../_includes/default-catalogue.md) %}

  1. Посмотрите описание команды для получения вывода последовательного порта:

      ```
      yc compute instance get --help
      ```

  1. Выберите виртуальную машину, например `first-instance`:

      {% include [compute-instance-list](../../_includes_service/compute-instance-list.md) %}

  1. Получите основную информацию о виртуальной машине:

      ```
      yc compute instance get first-instance
      ```

      Чтобы получить информацию о виртуальной машине вместе с [метаданными](../../concepts/vm-metadata.md), используйте флаг `--full`:

      ```
      yc compute instance get --full first-instance
      ```

- API

  Для получения основной информации о виртуальной машине используйте метод [get](../../api-ref/Instance/get.md) для ресурса [Instance](../../api-ref/Instance/index.md).

  Основная информация не включает пользовательские метаданные, которые были переданы при создании или изменении виртуальной машины. Чтобы получить информацию вместе с метаданными, укажите в параметрах `view=FULL`.

{% endlist %}

## Получить информацию изнутри виртуальной машины {#inside-instance}

{% include [vm-metadata](../../../_includes/vm-metadata.md) %}

### Google Compute Engine {#gce-metadata}

Сервис метаданных {{ yandex-cloud }} позволяет возвращать метаданные в формате Google Compute Engine.

#### HTTP-запрос {#gce-http}

```
GET http://169.254.169.254/computeMetadata/v1/instance/
  ? alt=<json|text>
  & recursive=<true|false>
  & wait_for_change=<true|false>
  & last_etag=<string>
  & timeout_sec=<int>
Metadata-Flavor: Google
```

Где:

Параметр | Описание
----- | -----
`alt` | Формат ответа (по умолчанию `text`).
`recursive` | Если `true`, возвращает все значения по дереву рекурсивно. По умолчанию `false`.
`wait_for_change` | Если `true`, ответ будет возвращен только когда один из параметров метаданных изменится. По умолчанию `false`.
`last_etag` | Значение ETag из предыдущего ответа на аналогичный запрос. Используйте при `wait_for_change="true"`.
`timeout_sec` | Максимальное время ожидания запроса. Используйте при `wait_for_change="true"`.

#### Примеры запросов {#request-examples}

Узнать идентификатор виртуальной машины изнутри машины:

```
curl -H Metadata-Flavor:Google 169.254.169.254/computeMetadata/v1/instance/id
```

Получить метаданные в формате JSON:

```
curl -H Metadata-Flavor:Google 169.254.169.254/computeMetadata/v1/instance/?recursive=true
```

Получить метаданные в удобном для чтения формате. Воспользуйтесь утилитой [jq](https://stedolan.github.io/jq/):

```
curl -H Metadata-Flavor:Google 169.254.169.254/computeMetadata/v1/instance/?recursive=true | jq -r '.'
```

#### Список возвращаемых элементов {#list-of-returned-items}

Список элементов, которые доступны по этому запросу.

Элемент | Описание
----- | -----
`attributes/` | Пользовательские метаданные, переданные при создании или изменении виртуальной машины в поле `metadata`.
`attributes/ssh-keys` | Список открытых SSH-ключей, переданных при создании виртуальной машины в поле `metadata` в значении ключа `ssh-keys`.
`description` | Текстовое описание, переданное при создании или изменении виртуальной машины.
`disks/` | Диски, подключенные к виртуальной машине.
`hostname` | [FQDN](../../concepts/network.md#hostname), назначенный виртуальной машине.
`id` | Идентификатор виртуальной машины. ID генерируется автоматически при создании виртуальной машины и уникален в пределах {{ yandex-cloud }}.
`name` | Имя, переданное при создании или изменении виртуальной машины.
`networkInterfaces/` | Сетевые интерфейсы, подключенные к виртуальной машине.
`service-accounts` | [Сервисные аккаунты](../../../iam/concepts/users/service-accounts.md), привязанные к виртуальной машине.
`service-accounts/default/token` | [IAM-токен](../../../iam/concepts/authorization/iam-token.md) привязанного сервисного аккаунта.

Другие элементы, например `project`, используются для обратной совместимости и остаются пустыми.

### Amazon EC2 {#ec2-metadata}

Сервис метаданных {{ yandex-cloud }} позволяет возвращать метаданные в формате Amazon EC2.
При этом формат не поддерживает работу с пользовательскими полями метаданных.

#### HTTP-запрос {#ec2-http}

```
GET http://169.254.169.254/latest/meta-data/<элемент>
```

Где:

Параметр | Описание
----- | -----
`<элемент>` | Путь к элементу, который вы хотите получить. Если элемент не задан, в ответе вернется список доступных элементов.

#### Список возвращаемых элементов {#list-of-returned-items}

Список элементов, которые доступны по этому запросу.

{% note info %}

В угловых скобках выделены параметры, которые необходимо заменить значениями. Например, вместо `<mac>` следует подставить MAC-адрес сетевого интерфейса.

{% endnote %}

Элемент | Описание
----- | -----
`hostname` | Имя хоста, присвоенное виртуальной машине.
`instance-id` | Идентификатор виртуальной машины.
`local-ipv4` | Внутренний IPv4-адрес.
`local-hostname` | Имя хоста, присвоенное виртуальной машине.
`mac` | MAC-адрес сетевого интерфейса виртуальной машины.
`network/interfaces/macs/<mac>/ipv6s` | Внутренние IPv6-адреса, ассоциированные с сетевым интерфейсом.
`network/interfaces/macs/<mac>/local-hostname` | Имя хоста, ассоциированное с сетевым интерфейсом.
`network/interfaces/macs/<mac>/local-ipv4s` | Внутренние IPv4-адреса, ассоциированные с сетевым интерфейсом.
`network/interfaces/macs/<mac>/mac` | MAC-адрес сетевого интерфейса виртуальной машины.
`public-ipv4` | Внешний IPv4-адрес.

#### Примеры запросов {#request-examples}

Получить внутренний IP-адрес изнутри виртуальной машины:

```
curl http://169.254.169.254/latest/meta-data/local-ipv4
```
