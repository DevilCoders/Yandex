# Спецификация группы виртуальных машин в формате YAML

Группу ВМ можно создать или изменить по спецификации в формате [YAML]{% if lang == "ru" %}(https://ru.wikipedia.org/wiki/YAML){% endif %}{% if lang == "en" %}(https://en.wikipedia.org/wiki/YAML){% endif %}. В спецификации описываются:

* базовые атрибуты и настройки группы — имя, описание, [метки](../../../overview/concepts/services.md#labels), [сервисный аккаунт](../../../iam/concepts/users/service-accounts.md) и защита от удаления;
* [шаблон ВМ](instance-template.md) и использующиеся в нем [переменные](variables-in-the-template.md);
* политики [распределения](policies/allocation-policy.md), [развертывания](policies/deploy-policy.md) и [масштабирования](policies/scale-policy.md);
* настройки балансировки трафика между ВМ с помощью [{{ network-load-balancer-full-name }}](../../../network-load-balancer/index.yaml) или [{{ alb-full-name }}](../../../application-load-balancer/index.yaml).

## Пример {#example}

По следующей спецификации можно создать автоматически масштабируемую группу ВМ, как в [сценарии обработки сообщений из очереди {{ message-queue-full-name }}](../../tutorials/autoscale-monitoring.md):

```yaml
folder_id: b1gken0eihqn2oa0fm2k
name: queue-autoscale-ig
instance_template:
  platform_id: standard-v3
  resources_spec:
    memory: 2g
    cores: 2
    core_fraction: 100
  boot_disk_spec:
    mode: READ_WRITE
    disk_spec:
      type_id: network-hdd
      size: 5g
      image_id: fd8m5bal0bi9tjhu7av2
  network_interface_specs:
  - network_id: enpocgefm44fp31qpdru
    subnet_ids:
    - e2ljvdp4j2762fl4qh1i
    primary_v4_address_spec:
      one_to_one_nat_spec:
        ip_version: IPV4
  scheduling_policy: {}
scale_policy:
  auto_scale:
    min_zone_size: 0
    max_size: 5
    measurement_duration: 60s
    warmup_duration: 0s
    stabilization_duration: 300s
    initial_size: 1
    auto_scale_type: REGIONAL
    custom_rules:
    - rule_type: WORKLOAD
      metric_type: GAUGE
      metric_name: queue.messages.stored_count
      labels:
        queue: queue-autoscale-queue
      target: 5
      service: message-queue
deploy_policy:
  max_unavailable: 1
  startup_duration: 0s
  strategy: OPPORTUNISTIC
allocation_policy:
  zones:
  - zone_id: {{ region-id }}-b
service_account_id: ajefnb8427bhl9t0pvf8
```

## Поля {#fields}

Список, структура и {% if lang == "ru" %} англоязычные {% endif %} описания полей спецификации представлены:

* в спецификации структуры [CreateInstanceGroupRequest](https://github.com/yandex-cloud/cloudapi/blob/master/yandex/cloud/compute/v1/instancegroup/instance_group_service.proto#L219) и [используемых в ней структур](https://github.com/yandex-cloud/cloudapi/blob/master/yandex/cloud/compute/v1/instancegroup/instance_group.proto) в репозитории API на GitHub (формат [Protobuf]{% if lang == "ru" %}(https://ru.wikipedia.org/wiki/Protocol_Buffers){% endif %}{% if lang == "en" %}(https://en.wikipedia.org/wiki/Protocol_Buffers){% endif %});
* в описании метода [create](../../api-ref/InstanceGroup/create) ресурса `InstanceGroup` REST API (формат [JSON]{% if lang == "ru" %}(https://ru.wikipedia.org/wiki/JSON){% endif %}{% if lang == "en" %}(https://en.wikipedia.org/wiki/JSON){% endif %});
* в описании вызова [InstanceGroupService/Create](../../api-ref/grpc/instance_group_service.md#Create) gRPC API.

Справочники API генерируются из спецификаций, размещенных на GitHub.

Некоторые поля первого уровня и вложенные в них поля также описаны {% if lang == "ru" %} на русском языке {% endif %} в разделах документации:

* [{#T}](instance-template.md) (поле `instance_template`).
* [{#T}](variables-in-the-template.md) (поля `instance_template` и `variables`).
* [{#T}](policies/allocation-policy.md) (поле `allocation_policy`).
* [{#T}](policies/deploy-policy.md) (поле `deploy_policy`).
* [{#T}](policies/scale-policy.md) (поле `scale_policy`).

Эти разделы обновляются вручную. Они могут быть менее актуальными, чем справочники API и спецификация на GitHub.

## Преобразование JSON и Protobuf в YAML {#translation-to-yaml}

Создать YAML-спецификацию по описанию в формате JSON или Protobuf можно по следующим правилам: 

* Объекты (JSON), структуры `message` и поля типа `map` (Protobuf) преобразуются в словарные объекты YAML (пары «ключ — значение»). Ключи объектов и названия полей преобразуются из `lowerCamelCase` и `CamelCase` в `snake_case`.

  {% list tabs %}
  
  - JSON
  
    ```json
    "targetGroupSpec": {
      "name": "spec-example-tg",
      "description": "ALB target group for example instance group",
      "labels": {
        "foo": "bar",
        "environment": "production"     
      }
    }
    ```
  
  - Protobuf
  
    ```protobuf
    message TargetGroupSpec {
      string name = 1;
      string description = 2;
      map<string, string> labels = 3;
    }
    ```
  
  - YAML
  
    ```yaml
    target_group_spec:
      name: spec-example-tg
      description: ALB target group for example instance group
      labels:
        foo: bar
        environment: production
    ```
  
  {% endlist %}

* Массивы (JSON) и поля типа `repeated` (Protobuf) преобразуются в списки YAML.

  {% list tabs %}
  
  - JSON
  
    ```json
    "variables": [
      {
        "key": "foo",
        "value": "bar"
      },
      {
        "key": "baz",
        "value": "foobar"
      }
    ]
    ```
  
  - Protobuf
  
    ```protobuf
    message Variable {
      string key = 1;
      string value = 2;
    }
    
    repeated Variable variables = 1;
    ```
  
  - YAML
  
    ```yaml
    variables:
      - key: foo
        value: bar
      - key: baz
        value: foobar
    ```
  
  {% endlist %}

* Структуры типа `enum` (Protobuf) преобразуются в скалярные значения YAML — строки, соответствующие названиям полей в Protobuf.

  {% list tabs %}
  
  - Protobuf
  
    ```protobuf
    message AttachedDiskSpec {
      enum Mode {
        MODE_UNSPECIFIED = 0;
        READ_ONLY = 1;
        READ_WRITE = 2;
      }
      Mode mode = 1;
    }
    ```
  
  - YAML
  
    ```yaml
    attached_disk_spec:
      mode: READ_WRITE
    ```
  
  {% endlist %}

## Формат значений {#values}

В формате YAML все значения в словарных объектах неявно являются строками, использовать для их обозначения кавычки необязательно. Содержимое строк должно соответствовать типам, указанным в справочнике API или в спецификации в формате Protobuf: `string`, `int64`, `bool` и т. п.

### Суффиксы для типа `int64` {#int64}

В значениях типа `int64` поддерживаются следующие суффиксы:

{% include [yaml-int64-suffixes](../../../_includes/instance-groups/yaml-int64-suffixes.md) %}

Если в значении используется суффикс, перед ним должно стоять целое число.

### Тип `boolean` {#boolean}

В полях типа `bool` или `boolean` поддерживаются все значения, перечисленные в [спецификации формата YAML](https://yaml.org/type/bool.html):

* Истина: `y`, `Y`, `yes`, `Yes`, `YES`, `true`, `True`, `TRUE`, `on`, `On`, `ON`.
* Ложь: `n`, `N`, `no`, `No`, `NO`, `false`, `False`, `FALSE`, `off`, `Off`, `OFF`.

## Как создать или изменить группу ВМ по спецификации {#how}

Создать группу виртуальных машин по YAML-спецификации можно через интерфейс командной строки (CLI) или API. Подробнее см. в инструкциях:

* [{#T}](../../operations/instance-groups/create-from-yaml.md)
* [{#T}](../../operations/instance-groups/update-from-yaml.md) 