# Создание задачи на основе задачи Sandbox

[Sandbox](https://sandbox.yandex-team.ru/) уже содержит множество готовых задач, позволяющих автоматизировать разные части процесса разработки: компилировать исходный код, собирать образы и пакеты с дистрибутивами, загружать файлы в различные хранилища и так далее. Вместо того, чтобы заново описывать задачи для CI с той же самой логикой, можно легко переиспользовать уже готовую задачу. Для этого требуется сделать несколько простых шагов:

1. Создать YAML файл с описанием Sandbox задачи, ее параметров и требуемых вычислительных ресурсов.
2. Добавить YAML файл в [реестр задач](https://a.yandex-team.ru/arc/trunk/arcadia/ci/registry) через пулл-реквест.

## Шаг 1. Создать YAML файл { #yaml }

Например, мы хотим добавить в реестр CI возможность запускать задачу типа [TEST_TASK_2](https://sandbox.yandex-team.ru/tasks?type=TEST_TASK_2). Создаем в [нужном каталоге](jobs.md#registry) файл `test_task_2.yaml`:

```yaml
title: TestTask2
description: Run Sandbox TEST_TASK_2
maintainers: my-abc-service-slug # Ваш сервис в ABC

sandbox-task:
  name: TEST_TASK_2 # Тип задачи в Sandbox
```

Так выглядит минимальный работоспособный YAML файл для добавления новой Sandbox задачи. Тем не менее оказывается удобным добавить в этот файл некоторую дополнительную информацию, например, список обязательных параметров, значения входных параметров задачи по умолчанию и [требования к вычислительным](requirement.md) ресурсам задачи по умолчанию:

```yaml
title: TestTask2
description: Run Sandbox TEST_TASK_2
maintainers: my-abc-service-slug

sandbox-task:
  name: TEST_TASK_2
  required-parameters: # Обязательные параметры задачи
      - create_sub_task
  # Список статусов Sandbox ресурсов, которые могут быть обработаны. По умолчанию - READY
  # Ресурсы с другими статусами (BROKEN, DELETED, NOT_READY) будут проигнорированы
  accept-resource-states:
    - READY

parameters: # Значения параметров задачи по умолчанию
  create_sub_task: true
  number_of_subtasks: 3

attempts: 4 # Количество попыток выполнения кубика. Может быть переопределена на уровне задачи во флоу.

# Требования к вычислительным ресурсам по умолчанию (аналогично a.yaml)
# Заполнять их необязательно, это просто примеры
requirements:
  disk: 15GB
  cores: 2
  ram: 4 GB
  tmpfs: 300 MB

  sandbox:
    client_tags: SSD
    container_resource: 9
    dns: dns64
    host: sas1-1337
    cpu_model: 'E5-2650 v2'
    platform: linux
    privileged: true
    priority:
      class: BACKGROUND
      subclass: LOW

```

Конкретные названия ключей для входных параметров задачи зависят от используемой задачи и определены в её коде. Можно посмотреть названия параметров прямо на странице задачи в секции `Input parameters`:

![Названия входных параметров Sandbox задачи](img/sandbox-task-parameters.png "Названия входных параметров Sandbox задачи" =800x)

Другой возможный вариант - это изучить [код](https://a.yandex-team.ru/arc/trunk/arcadia/sandbox/projects/sandbox/test_task_2/__init__.py) задачи и найти там класс `Parameters`:

```python
from sandbox import sdk2

# ...
class TestTask2(sdk2.ServiceTask):
    # ...
    class Parameters(sdk2.Parameters):
        # ...
        create_sub_task = sdk2.parameters.Bool("Create subtask") # В YAML используем ключ create_sub_task
```

### Бинарные Sandbox задачи { #binaries }

Помимо обычных Sandbox задач, можно запускать и [бинарные Sandbox задачи](https://docs.yandex-team.ru/sandbox/dev/binary-task).

Для этого достаточно [указать версию ресурса](job-advanced.md#versions).
```yaml
title: TestTask2
description: Run Sandbox TEST_BINARY_TASK_2
maintainers: my-abc-service-slug

sandbox-task:
  name: TEST_BINARY_TASK_2

parameters: # Значения параметров задачи по умолчанию
  create_sub_task: true
  number_of_subtasks: 3

versions: # Версии задач, ключ stable - обязателен, т.к. используется по умолчанию
  stable: 1902585365 # Ресурс в Sandbox, где хранится скомпилированная задача
  prestable: 1902585365
  testing: 1902585365
...
```


### Sandbox задачи из шаблонов { #templates }

Помимо обычных Sandbox задач, можно запускать и задачи из [Sandbox шаблонов](https://docs.yandex-team.ru/sandbox/templates).

{% note warning %}

Использовать шаблоны стоит с осторожностью.
При их использовании конфигурация становиться негерметичная, т.е. изменение в шаблонах неявно меняет поведение flow.
По этой причине использование шаблонов запрещено в common секции реестра.

{% endnote %}

Для этого необходимо настроить свой шаблон, а потом указать его в параметре `template`:
```yaml
title: TestTask2
description: Run Sandbox template SB_TEMPLATE_1
maintainers: my-abc-service-slug

sandbox-task:
  template: SB_TEMPLATE_1

parameters: # Значения параметров задачи по умолчанию
  create_sub_task: true
  number_of_subtasks: 3

...
```

Такой шаблон будет передан в Sandbox в параметре `template_alias`.

{% note info %}

Название задачи в этом случае указывать не нужно.

{% endnote %}

## Шаг 2. Добавить YAML файл в единый репозиторий { #pr }

1. Предположим, что файл `test_task_2.yaml`, созданный на предыдущем этапе, лежит в каталоге `/ci/registry/projects/my_project`. Создаем пулл-реквест:

    ```bash
    $ arc checkout -b test-task-2
    $ arc add ci/registry/projects/my_project/test_task_2.yaml
    $ arc commit -m 'Added test_task_2.yaml'
    $ arc pr create --push --publish
    ```

2. Дожидаемся, пока пройдут проверки правильности синтаксиса, и файл будет добавлен в единый репозиторий.

3. Теперь можно использовать новую задачу в своих файлах `a.yaml`:

    ```yaml
    service: ci
    title: Yandex CI
    ci:

      # ... secret, runtime, releases, triggers ...

      flows:
        my-flow:
          title: My Flow
          jobs:
            run-test-task-2:
              title: Run TEST_TASK_2 task in Sandbox
              task: projects/my_project/test_task_2 # То есть /ci/registry/projects/my_project/test_task_2.yaml
              input:
                create_sub_task: false # Переопределяем значения параметров по умолчанию
                create_resources: true # Передаем параметры, для которых не указаны значения по умолчанию
    ```
