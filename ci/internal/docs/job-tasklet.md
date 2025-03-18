# Создание задачи на основе технологии Tasklet

{% note info %}

Видео с демонстрацией того, что описано ниже, можно посмотреть в [записе встречи Public CI от 24 декабря 2020](https://docs.yandex-team.ru/ci/public-ci#24-dekabrya-2020).

{% endnote %}

Все новые задачи для CI рекомендуется создавать на основе технологии Tasklet. **Tasklet** - это внутренняя технология Яндекса, придуманная командой [Sandbox](https://sandbox.yandex-team.ru/). Основная идея Tasklet состоит в том, чтобы создавать программы, входные и выходные данные которых описаны в виде [Protobuf](https://developers.google.com/protocol-buffers). Код таких программ пишется на одном из поддерживаемых языков программирования: в настоящий момент на [Python](https://www.python.org/) и [C++](https://www.cplusplus.com/). После написания кода, автоматически собирается двоичный исполняемый файл, который запускается в любом окружении, поддерживающем технологию Tasklet. В настоящий момент с такими программами умеет работать сам Sandbox и планируется поддержка в других подобных системах исполнения задач (например, в [YT](https://yt.yandex-team.ru/)).

{% note warning %}

**Тасклеты должны собираться только на Linux.**

Все действия ниже должны проводиться только на машине с Linux.

{% endnote %}

{% note info %}

Шаблон тасклета также можно создать командой `ya project create tasklet`

{% endnote %}

В качестве примера рассмотрим создание задачи, позволяющей **выполнить произвольную команду** в командной строке (run shell command). Чуть более сложный и реально используемый пример такой задачи можно посмотреть [здесь](https://a.yandex-team.ru/arc_vcs/ci/tasklet/registry/common/misc/run_command). В разделе [Описание задач](flow.md) мы уже видели схематичное изображение задачи в виде "черного ящика" с одним **входом** (input) и одним **выходом** (output):

![Вход и выход задачи](img/flow-job.png "Вход и выход задачи" =800x)

Для того чтобы в реестре появилась задача для выполнения команд, необходимо:

1. Описать вход и выход (т.е. **интерфейс**) задачи, используя [Protobuf](https://developers.google.com/protocol-buffers).
2. Написать реализацию задачи на Python или C++.
3. Собрать исполняемый файл и положить его в [Sandbox ресурс](https://docs.yandex-team.ru/sandbox/resources).
4. Добавить метаданные в YAML формате в реестр, аналогично тому, как мы делали это при создании задачи [из Sandbox задачи](job-sandbox.md).

## Шаг 1. Описать интерфейс в Protobuf { #proto-file }

Для описания интерфейса поймем, что является входом и выходом задачи. В качестве **входных данных** мы будем принимать команду, которую необходимо выполнить. Объявим конфигурационные параметры:

```protobuf
syntax = "proto3";

package my_package;

message Config {
    string cmd_line = 1; // Команда, которую будем выполнять
}
```

Дополнительно мы хотим иметь доступ к информации о коммите и пулл-реквесте (т.е. к [контексту](flow.md#context) flow), поэтому итоговый вход задачи будет выглядеть так:

```protobuf
syntax = "proto3";

import "ci/tasklet/common/proto/service.proto";

package my_package;

message Config {
    string cmd_line = 1;
}

// Описание интерфейса входа задачи
message Input {
    ci.TaskletContext context = 1; // Контекст flow
    Config config = 2; // Конфигурационные параметры от пользователя
}
```

В качестве **выходных данных** мы хотим получить информацию о том завершилась ли команда успешно:

```protobuf
syntax = "proto3";

package my_package;

// Объект, описывающий состояние выполнения команды
message Payload {
    string message = 1; // какие-то данные
}

// Описание интерфейса выхода задачи
message Output {
    Payload result = 1;
    // Здесь можно передавать на выход другие данные
}
```

{% note warning %}

Нельзя указывать примитивные типы и map в теле объектов Input и Output. В качестве типов полей разрешено использовать только другие объекты message.

Зато во вложенных объектах примитивные типы и map можно и нужно использовать.

**Неправильно:**

```protobuf
syntax = "proto3";

package my_package;

message Notification {
    string email = 1;
}

message Input {
    string cmd_line = 1; // Примитивные типы запрещены
    map<string, Notification> notifications = 2; // Использование map запрещено
}
```

**Правильно:**

```protobuf
syntax = "proto3";

package my_package;

message Type {
   string index = 1;
}

message Config {
    string cmd_line = 1;
    map<string, Type> types = 2;
}

message Notification {
    string email = 1;
}

message Input {
    Config config = 1;
    repeated Notification notifications = 2;
}
```

{% endnote %}


Остается объявить интерфейс самой задачи, чтобы он содержал вход и выход:

```protobuf
// run_command.proto
syntax = "proto3";

import "ci/tasklet/common/proto/service.proto";
import "tasklet/api/tasklet.proto";

package my_package;

// Вход
message Config {
    string cmd_line = 1;
}

message Input {
    ci.TaskletContext context = 1;
    Config config = 2;
}

// Выход
message Payload {
    string message = 1; // какие-то данные
}

message Output {
    Payload result = 1;
}

// Интерфейс задачи
message MyRunCommand {
    // Специальная аннотация, помечающая интерфейс задачи
    option (tasklet.tasklet_interface) = true;

    Input input = 1 [(tasklet.input) = true]; // Вход
    Output output = 2 [(tasklet.output) = true]; // Выход
}
```

Основная особенность описания интерфейса самой задачи состоит в том, что нужно добавить специальные аннотации (опции в терминах Protobuf) `tasklet.tasklet_interface`, `tasklet.input` и `tasklet.output`, пометив вход и выход задачи.

## Шаг 2. Написать реализацию задачи { #code }

{% note warning %}

Рекомендуемый язык для написания тасклетов - Python 3.

{% endnote %}

Теперь, когда мы определили интерфейс задачи, начнем складывать нужные файлы в единый репозиторий. Все исходные коды задачи, включая описание её интерфейса, могут храниться в любом месте единого репозитория. Предположим, что исходные коды будут храниться в каталоге `/my_project/tasklets/my_run_command/`. Создадим в этом каталоге такую иерархию файлов:

```
/my_project/tasklets/run_command/
                                |
                                - ya.make
                                - proto
                                       |
                                       - ya.make
                                       - run_command.proto
                                - impl
                                      |
                                      - ya.make
                                      - __init__.py
```

* Файл `/my_project/tasklets/my_run_command/ya.make` содержит:

    ```yamake
    PY3_PROGRAM()

    OWNER(<your-login>) # Укажите ваш логин или группу, которая будет отвечать за исходные коды https://docs.yandex-team.ru/ya-make/manual/common/macros#owner

    PY_MAIN(tasklet.cli.main)

    PEERDIR(
        my_project/tasklets/my_run_command/impl # Реализация задачи
        tasklet/cli # Обертка, запускающая код реализации
    )

    END()

    RECURSE(
        impl
        proto
    )
    ```

* Файл `/my_project/tasklets/my_run_command/proto/ya.make` содержит:

    ```yamake
    PROTO_LIBRARY()

    OWNER(<your-login>)

    TASKLET()

    EXCLUDE_TAGS(GO_PROTO)  # Если вы не собираетесь использовать эти proto-файлы из языка Go


    SRCS(
        my_run_command.proto
    )

    PEERDIR(
        ci/tasklet/common/proto
        tasklet/api
        tasklet/services/ci/proto
    )

    END()
    ```

* Файл `/my_project/tasklets/my_run_command/proto/run_command.proto` содержит описание интерфейса задачи, которое мы написали ранее.

* Файл `/my_project/tasklets/my_run_command/impl/ya.make` содержит:

    ```yamake
    PY3_LIBRARY()

    OWNER(<your-login or g:my-abc-service-slug>)

    PY_SRCS(
        __init__.py
    )

    # Интерфейсу MyRunCommand из my_run_command.proto соответствует реализация в виде Python класса MyRunCommandImpl
    TASKLET_REG(MyRunCommand py my_project.tasklets.my_run_command.impl:MyRunCommandImpl)

    PEERDIR(
        my_project/tasklets/my_run_command/proto
        tasklet/services/ci # Рекомендуемый импорт для доступа ко всей функциональности CI
    )

    END()
    ```

* Наконец, файл `/my_project/tasklets/my_run_command/impl/__init__.py` содержит реализацию задачи на языке Python:

    ```python
    import logging
    import os

    import subprocess

    from my_project.tasklets.my_run_command.proto import my_run_command_tasklet

    # Класс наследуется от сгенерированного run_command_tasklet.MyRunCommandBase
    class MyRunCommandImpl(run_command_tasklet.MyRunCommandBase):

        def run(self):
            # Получаем входные данные
            cmd = self.input.config.cmd_line

            # Получаем информацию о текущем коммите из ci.TaskletContext (показано в качестве примера)
            revision = self.input.context.target_revision.hash
            logging.info('Running on revision "%s"', revision)

            # Выполняем команду
            logging.info('Running command "%s"', cmd)
            process = subprocess.Popen(
                cmd,
                env=os.environ.copy(),
                shell=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                close_fds=True
            )
            output, err = process.communicate()
            logging.info('Command stdout is:\n%s', output)
            logging.info('Command stderr is:\n%s', err)
            rc = process.returncode
            logging.info('Command finished with %s return code', rc)
            success = rc == 0

            # Выставляем выходные данные
            self.output.result.message = 'Success' if success else 'Something went wrong'

            # В текущей реализации, вне зависимости от успешности или неуспешности выполнения команды
            # задача в CI будет считаться успешно выполненой. Чтобы сигнализировать CI, что задача завершилась
            # с ошибкой, необходимо бросить исключение. Например:
            # if not success:
            #     raise Exception('Something went wrong')
    ```

    Как видно код содержит описание одного класса, для которого нужно реализовать метод `run` с описанием того,
  что будет делать задача.
  В данном примере предполагается, что команда завершилась успешно, если ее код возврата равен нулю.

## Шаг 3. Собрать исполняемый файл { #build }

Убедимся, что все работает. Для этого соберем исполняемый файл из исходников:

```bash
my_project/tasklets/my_run_command $ ya make -r
```

Мы получили исполняемый файл `run_command`. Полный список доступных команд и флагов можно вывести так:

```bash
my_project/tasklets/my_run_command $ ./my_run_command --help
```

Для отображения возможных входных и выходных данных у получившегося кода можно использовать команды:

```bash
my_project/tasklets/my_run_command $ ./my_run_command list # Вывести список тасклетов
['RemoteExec', 'MyRunCommand']
my_project/tasklets/my_run_command $ ./my_run_command schema MyRunCommand # Вывести схему для MyRunCommand
```

Для того, чтобы отладить код задачи локально, запустим его:

```bash
my_project/tasklets/my_run_command $ ./my_run_command run --local --input '{"config": {"cmd_line": "echo 123;"}, "context": {"target_revision": {"hash": "e84f4c617c693acf5d23c181d35b2a7f53acb259"}}}' RunCommand
```

Для того, чтобы запустить код задачи в задаче Sandbox, замените `--local` на `--sandbox-tasklet`:

```bash
my_project/tasklets/my_run_command $ ./my_run_command run --sandbox-tasklet --input '{...}' MyRunCommand
```

В этом случае будет создана задача и информация о результатах её выполнения будет выведена на экран.

Если всё работает правильно, то необходимо найти загруженный в Sandbox ресурс (например, при выполнении команды run `INFO  ThreadPoolExecutor-0_0 (tasklet/domain/sandbox/utils.py:53) Tasks resource has uploaded: https://sandbox.yandex-team.ru/resource/2590172644`) и удалить его вручную.
После чего загрузить ресурс со схемой:

```bash
$ my_project/tasklets/my_run_command $ ./my_run_command sb-upload --sb-schema
# ...
1902585365 # Идентификатор загруженного Sandbox ресурса
```

{% note warning %}

После завершения отладки не забудьте зафиксировать установленный тасклет, нажав кнопку **Release** в созданной **MDS_UPLOAD** задаче. Эта операция установит время жизни залитому ресурсу `ttl=inf`, т.е. бесконечность.

Если этого не сделать, то ваш тасклет исчезнет из Sandbox-а через две недели.

{% endnote %}

{% note info %}

Не забудьте добавить весь код, который мы написали ранее, в единый репозиторий через пулл-реквест.

{% endnote %}


## Шаг 4. Добавить метаданные задачи в реестр CI { #yaml }

Теперь, когда мы создали задачу с помощью технологии Tasklet, остается добавить её в [реестр](https://a.yandex-team.ru/arc/trunk/arcadia/ci/registry).

1. Для этого создадим YAML файл с описанием метаданных задачи. Создаем в [нужном каталоге](jobs.md#registry) файл `run_command.yaml`. В данном примере файл лежит в `/ci/registry/projects/my_project/my_run_command.yaml`:

    ```yaml
    title: MyRunCommand
    description: Run shell command
    maintainers: my-abc-service-slug # Ваш сервис в ABC
    sources: /my_project/tasklets/my_run_command # Где лежат исходники задачи

    tasklet:
      runtime: sandbox
      implementation: MyRunCommand # Название зарегистрированного тасклета из команды TASKLET_REG

    versions: # Версии задач, ключ stable - обязателен, т.к. используется по умолчанию
      stable: 1902585365 # Ресурс в Sandbox, где хранится скомпилированная задача
      prestable: 1902585365
      testing: 1902585365

    parameters: # Параметры задачи по умолчанию
      config:
        cmd_line: echo 123

    attempts: 4 # Количество попыток выполнения кубика по умолчанию. Может быть переопределена на уровне задачи во флоу.

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
        platform: linux_ubuntu_16.04_xenial
        privileged: true
        priority:
          class: BACKGROUND
          subclass: LOW
    ```

    Видно, что в этом файле указано название задачи, название объекта (тасклета), который мы описали в Protobuf, а также номер Sandbox ресурса, где лежит собранный исполняемый файл с кодом задачи. Дополнительно можно указать значения входных параметров и требований к вычислительным ресурсам по умолчанию. Остается добавить YAML файл в реестр задач через пулл-реквест:

    ```bash
    $ arc checkout -b my_run_command
    $ arc add ci/registry/projects/my_project/my_run_command.yaml
    $ arc commit -m 'Added my_run_command.yaml'
    $ arc pr create --push --publish
    ```

2. Дожидаемся, пока пройдут проверки правильности синтаксиса, и файл будет добавлен в единый репозиторий.

3. Теперь можно использовать новую задачу в своих файлах `a.yaml`:

    ```yaml
    service: my-abc-service-slug
    title: My AYaml
    ci:

      # ... secret, runtime, releases, triggers ...

      flows:
        my-flow:
          title: My Flow
          jobs:
            run-command-example:
              title: Run shell command
              task: projects/my_project/my_run_command # То есть /ci/registry/projects/my_project/my_run_command.yaml
              input:
                cmd_line: whoami
    ```

4. По умолчанию используется ресурс, указанный в версии `stable`. Чтобы выбрать другой ресурс, необходимо [указать версию ресурса в задаче](job-advanced.md#versions).

## FAQ

### Cannot find tasklet schema for implementation { #cannot-find-tasklet-schema }

Как правило, такое происходит, когда тасклет загружен без схемы. Это может произойти, в том числе, неявно.
Например, если тасклет был запущен с опцией `run --sandbox-tasklet`. В этом случае происходит загрузка ресурса с
тасклетом происходит неявно, и повторные загрузки `upload --sb-schema` не приводят к обновлению аттрибутов. 
Чтобы обновить аттрибуты, нужно удалить существующих ресурс в sb и загрузить его с помощью `upload --sb-schema`. 

### Config uses outdated tasklets which require glycine_token { #outdated-tasklet }

В тасклеты добавлена функциональность, позволяющая отказаться от glycine_token.
Это сообщение говорит о том, что в вашей конфигурации используются тасклеты, которые еще не поддерживают это.
Чтобы все работало, обновите ваши тасклеты. Для этого достаточно собрать тасклет на ревизии после r8411621
как показано на [шаге 3](#build) и обновить версию в соответствующем файле в реестре.
