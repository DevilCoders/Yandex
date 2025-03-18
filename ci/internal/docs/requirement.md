# Требования к задачам

[Sandbox](https://sandbox.yandex-team.ru/) - система исполнения произвольных задач общего назначения. При использовании Sandbox каждая задача flow — это отдельная задача Sandbox.

По умолчанию flow запускаются в окружении **без указания явных требований** к вычислительным ресурсам (числу процессорных ядер, оперативной памяти, типу диска, наличию tmpfs и так далее). Это означает, что будут использоваться значения, назначаемые **средой исполнения** (например, Sandbox) или определенные в коде соответствующей задачи.

При необходимости вы можете явно задавать требования к вычислительным ресурсам:

{% code 'ci/internal/ci/core/src/test/resources/ayaml/sandbox-requirements.yaml' lang='yaml' lines='documentationStart-documentationEnd' %}

Все параметры, указанные выше, являются необязательными. Полный список возможных значений для каждого параметра можно посмотреть в [документации Sandbox](https://docs.yandex-team.ru/sandbox/dev/requirements).

Дополнительные настройки среды выполнения можно задать в блоке [runtime](runtime.md).


## LXC контейнер для запуска задачи { #lxc }
Ссылкка на [ресурс Sandbox](https://sandbox.yandex-team.ru/resource/2278729028/view), в котором сохранён LXC контейнер.
```yaml
requirements:
  sandbox:
    container_resource: 2278729028
```

См. [документацию Sandbox](https://docs.yandex-team.ru/sandbox/dev/environment?searchQuery=porto#containers) для уточнения деталей.

## Порто слои { #porto }
По аналогии с LXC контейнерам можно указывать Porto слои (в виде кодов Sandbox ресурсов), которые будут развёрнуты при запуске задачи.
Смешивать LXC и порто-слои не рекомендуется.

Можно указать один слой:
```yaml
requirements:
  sandbox:
    porto_layers: 2729389408
```

или несколько слоёв:
```yaml
requirements:
  sandbox:
    porto_layers:
      - 2729389408
      - 2729415455
```

У нас нет ни валидации слоёв, ни реестра слоёв, поэтому указывать их нужно аккуратно.
См. [документацию Sandbox](https://docs.yandex-team.ru/sandbox/dev/environment?searchQuery=porto#containers) для уточнения деталей.

## Семафоры для задач { #semaphores }

Существует возможность запускать задачи под семафором:
```yaml
requirements:
  sandbox:
    semaphores:
      acquires:
        - name: my_semaphore_name
```

или под несколькими:
```yaml
requirements:
  sandbox:
    semaphores:
      acquires:
        - name: my_semaphore_name
        - name: ci_semaphore
```

или с указанием `weight` и `capacity`:
```yaml
requirements:
  sandbox:
    semaphores:
      acquires:
        - name: my_semaphore_name
          weight: 2 # По умолчанию 1
          capacity: 10 # По умолчанию 1
```

или даже с [использованием выражений](expression.md):
```yaml
requirements:
  sandbox:
    semaphores:
      acquires:
        - name: user-${context.flow_triggered_by}
```

Можно задать условия освобождения семафоров:
```yaml
requirements:
  sandbox:
    semaphores:
      acquires:
        - name: my_semaphore_name
      release:
        - BREAK
        - FINISH
```


См. [документацию Sandbox](https://docs.yandex-team.ru/sandbox/dev/semaphores) с описанием параметров и значений по умолчанию.

## Переопределение настроек { #override }
Блок `requirements` можно задать на любом уровне, начиная с [реестра задач](jobs.md#registry)

{% code 'ci/internal/ci/core/src/test/resources/task/task-with-requirements.yaml' lang='yaml' lines='documentationStart-documentationEnd' %}

и заканчивая задачей:

{% code 'ci/internal/ci/core/src/test/resources/ayaml/with-requirements.yaml' lang='yaml' lines='documentationStart-documentationEnd' %}

Каждый следующий уровень настроен переопределяет предыдущий, за одним исключеним:
* `tags` и `hints` наследуются.

Поэтому для примера выше результирующие настройки задач будут выглядеть так:
* При запуске задачи `single` из action-а `my-action`

{% code 'ci/internal/ci/core/src/test/resources/ayaml/with-requirements.yaml' lang='yaml' lines='documentationActionStart-documentationActionEnd' %}

* При запуске задачи `single` из release-а `my-release`

{% code 'ci/internal/ci/core/src/test/resources/ayaml/with-requirements.yaml' lang='yaml' lines='documentationReleaseStart-documentationReleaseEnd' %}
