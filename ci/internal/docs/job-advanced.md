# Дополнительные возможности задач

{% note info %}

Для понимания данного раздела необходимо понимать основы создания задач [на основе задач Sandbox](job-sandbox.md) и [технологии Tasklet](job-tasklet.md).

{% endnote %}


В этом разделе описаны дополнительные возможности, которые вы можете использовать при создании собственных задач.

## Версии задач { #versions }

При использовании задач на основе технологии Tasklet или бинарных Sandbox задач можно одновременно запускать несколько разных версий кода одной и той же задачи. В YAML описании задачи в реестре есть поле `versions`, содержащее числовые идентификаторы [Sandbox ресурсов](https://docs.yandex-team.ru/sandbox/resources), в которых хранятся исполняемые файлы с кодом задачи:

```yaml
title: RunCommand
description: Run shell command
maintainers: my-abc-service-slug
sources: /my_project/tasklets/my_run_command

tasklet:
  implementation: MyRunCommand

versions: # Версии задачи с разной стабильностью, ключ stable - обязателен.
  stable: 1902585365 # Ресурс в Sandbox, где хранится скомпилированная задача
  prestable: 1902585365
  testing: 1902585365
```

По умолчанию CI использует исполняемый файл версии `stable`, но в **a.yaml** возможно указать конкретную версию, которую следует использовать:

```yaml
service: my-abc-service-slug
title: My Project in CI
ci:

  # ... secret, runtime, releases, triggers ...

  flows:
    my-flow:
      title: My Flow
      description: This flow does a lot of things.
      jobs:
        run_command:
          title: Run shell command
          task: projects/my_project/my_run_command
          version: testing # Использует тестовую версию задачи
          input:
            config:
              cmd_line: echo 123
```

## Работа с внешними сервисами { #external-services }

В некоторых случаях может потребоваться обратиться из кода задачи во внешние сервисы. Например, вы можете захотеть в коде задачи получить секреты из [Секретницы](https://yav.yandex-team.ru/). Для того, чтобы автоматически получить клиента для нужного сервиса, нужно, чтобы его API было описано в виде [Protobuf](https://developers.google.com/protocol-buffers), который мы будем импортировать в описание задачи.

{% note info %}

1. Описание интерфейса сервиса в Protobuf не означает, что сервис должен использовать эту технологию для сериализации данных. Данный подход работает одинаково, как с обычными [REST](https://en.wikipedia.org/wiki/Representational_state_transfer) API, так и с [GRPC](https://grpc.io/). Как обращаться к внешнему сервису определяется в реализации описанного интерфейса.
2. Примеры описания API к внешним сервисам с помощью Protobuf и реализации можно посмотреть [здесь](https://a.yandex-team.ru/arc/trunk/arcadia/tasklet/services).

{% endnote %}

Например, пусть мы хотим получать значения секретов из Секретницы в коде задачи, который выполняет команды. Для этого мы добавляем в соответствующий *.proto файл дополнительный объект:

```protobuf
syntax = "proto3";

import "tasklet/api/tasklet.proto";
import "tasklet/services/yav/proto/yav.proto";

package my_package;

message Context {
    option (tasklet.context) = true; // Аннотация tasklet.context помечает объект как контекст

    yav_service.YavService yav = 2 [(tasklet.inject) = true]; // Специальная аннотация tasklet.inject позволяет добавлять внешних клиентов
}
```

После этого мы добавляем дополнительное поле в описание интерфейса задачи:

```protobuf
// run_command.proto
syntax = "proto3";

import "ci/tasklet/common/proto/service.proto";
import "tasklet/api/tasklet.proto";
import "tasklet/services/yav/proto/yav.proto";

package my_package;

message Config {
    string cmd_line = 1;
    string token_yav_key = 2; // Добавили новое поле с именем ключа в Секретнице
}

message Input {
    ci.TaskletContext context = 1;
    Config config = 2;
}

message State {
    bool success = 1;
}

message Output {
    State state = 1;
}

message Context {
    option (tasklet.context) = true;

    yav_service.YavService yav = 1 [(tasklet.inject) = true]; // Клиент для Секретницы (Yandex Vault)
}

message RunCommand {
    option (tasklet.tasklet_interface) = true;

    Input input = 1 [(tasklet.input) = true];
    Output output = 2 [(tasklet.output) = true];

    Context ctx = 3; // Добавили новое поле, откуда будем брать клиентов для API
}
```

Теперь в коде задачи можно обращаться к клиенту следующим образом:

```python
from my_project.tasklets.run_command.proto import my_run_command_tasklet
from tasklet.services.yav.proto import yav_pb2 as yav

class MyRunCommandImpl(my_run_command_tasklet.MyRunCommandBase):

    def run(self):
        # Получаем информацию о секрете из Секретницы
        secret_uid = self.input.context.secret_uid
        yav_key_name = self.input.config.token_yav_key
        spec = yav.YavSecretSpec(uuid=secret_uid, key=yav_key_name)
        token_value = self.ctx.yav.get_secret(spec, default_key="my-token").secret
```

## Информация о состоянии задачи { #progress }

![Информация о состоянии задачи](img/job-module.png "Информация о состоянии задачи" =800x)

В коде задачи вы можете периодически сообщать о его состоянии. Технически это реализовано как отдельное API, поэтому подход к написанию такого кода аналогичен работе с внешними сервисами. При этом в Protobuf мы добавляем другого клиента:

```protobuf
syntax = "proto3";

import "ci/tasklet/common/proto/service.proto";
import "tasklet/api/tasklet.proto";
import "tasklet/services/ci/proto/ci.proto";

package my_package;

message Context {
    option (tasklet.context) = true;

    ci.CiService ci = 1 [(tasklet.inject) = true]; // Клиент для API CI
}
```

В коде задачи в нужный момент мы делаем вызов метода `UpdateProgress` и передаем в него новое состояние задачи:

```python
from my_project.tasklets.run_command.proto import my_run_command_tasklet
from ci.tasklet.common.proto import service_pb2 as ci

class MyRunCommandImpl(run_command_tasklet.MyRunCommandBase):

    def run(self):
        # Обновления состояние задачи
        progress = ci.TaskletProgress()
        progress.job_instance_id.CopyFrom(self.input.context.job_instance_id) # Id текущей выполняющейся задачи
        progress.id = "runCommand" # Идентификатор состояния
        progress.progress = 0.4 # 40% работ завершено
        progress.text = "Running command (40% complete)" # Текстовое описание того, что сейчас происходит
        progress.module = "YANDEX_DEPLOY" # Иконка, которую будем показывать
        progress.url = "http://my-service.yandex-team.ru/" # Ссылка при нажатии на значок
        progress.status = ci.TaskletProgress.Status.RUNNING # Текущее состояние: RUNNING, FAILED, SUCCESSFUL
        self.ctx.ci.UpdateProgress(progress)

        # ...
```

Поле `id` позволяет создавать несколько разных иконок. На каждое уникальное значение этого поля создается новая иконка. Поддерживаются следующие иконки в поле `module`:

Иконка | Название | Описание
:--- | :--- | :---
![aqua.png](img/services/aqua.png "aqua.png" =32x) |  AQUA | Ссылка на запуск Java тестов в [Акве](https://aqua.yandex-team.ru/)
![allure.png](img/services/allure.png "allure.png" =32x) |  ALLURE | Ссылка на [Allure](https://github.com/allure-framework/) отчет
![arcadia.png](img/services/arcadia.png "arcadia.png" =32x) |  ARCADIA | Ссылка на страницу в [Arcanum](https://a.yandex-team.ru/)
![balancer_api.png](img/services/balancer_api.png "balancer_api.png" =32x) |  BALANCER_API | Ссылка на Balancer API
![bazel.png](img/services/bazel.png "bazel.png" =32x) | BAZEL | Система сборки [Bazel](https://bazel.build/)
![conductor.png](img/services/conductor.png "conductor.png" =32x) |  CONDUCTOR | Ссылка на [Кондуктор](https://c.yandex-team.ru/)
![dbaas.png](img/services/dbaas.png "dbaas.png" =32x) |  DBAAS | Ссылка на базы данных, например, [YDB](https://ydb.yandex-team.ru/) или [Cloud](https://cloud.yandex-team.ru/)
![github.png](img/services/github.png "github.png" =32x) |  GITHUB | Ссылка на [Github](https://github.yandex-team.ru/)
![gitlab.png](img/services/gitlab.png "gitlab.png" =32x) |  GITLAB | Ссылка на GitLab
![yandex_cloud.png](img/services/yandex_cloud.png "yandex_cloud.png" =32x) |  IAM_YANDEX_CLOUD | Ссылка на ресурсы [Яндекс.Облака](https://cloud.yandex-team.ru/)
![infra.png](img/services/infra.png "infra.png" =32x) |  INFRA | Ссылка на [Infra](https://infra.yandex-team.ru/)
![jenkins.png](img/services/jenkins.png "jenkins.png" =32x) |  JENKINS | Ссылка на сборку в [Jenkins](https://www.jenkins.io/)
![juggler.png](img/services/juggler.png "juggler.png" =32x) |  JUGGLER | Ссылка на мониторинги в [Juggler](https://juggler.yandex-team.ru/)
![kubernetes.png](img/services/kubernetes.png "kubernetes.png" =32x) |  KUBERNETES | Иконка [Kubernetes](https://kubernetes.io/)
![l3manager.png](img/services/l3manager.png "l3manager.png" =32x) |  L3_MANAGER | Ссылка на NOC [L3 Manager](https://l3.tt.yandex-team.ru/)
![lunapark.png](img/services/lunapark.png "lunapark.png" =32x) |  LUNAPARK | Ссылка на результаты нагрузочного тестирования в [Lunapark](https://lunapark.yandex-team.ru/)
![market_front.png](img/services/market_front.png "market_front.png" =32x) |  MARKET_FRONT | Логотип Маркета
![metrika.png](img/services/metrika.png "metrika.png" =32x) |  METRIKA | Ссылка на ресурсы в [Яндекс.Метрике](https://metrika.yandex.ru/)
![molly.png](img/services/molly.png "molly.png" =32x) |  MOLLY | Ссылка на результаты сканирования в [Molly](https://molly.yandex-team.ru/)
![nanny.png](img/services/nanny.png "nanny.png" =32x) |  NANNY | Ссылка на объекты в [Nanny](https://nanny.yandex-team.ru/)
![nirvana.png](img/services/nirvana.png "nirvana.png" =32x) |  NIRVANA | Ссылка на объекты в [Nirvana](https://nirvana.yandex-team.ru/)
![platform.png](img/services/platform.png "platform.png" =32x) |  PLATFORM | Ссылка на [Qloud](https://platform.yandex-team.ru/)
![puncher.png](img/services/puncher.png "puncher.png" =32x) |  PUNCHER | Ссылка на правило в [Puncher](https://puncher.yandex-team.ru/)
![reactor.png](img/services/reactor.png "reactor.png" =32x) |  REACTOR | Ссылка на [Reactor](https://reactor.yandex-team.ru/)
![samogon.png](img/services/samogon.png "samogon.png" =32x) |  SAMOGON | Иконка [Samogon](https://wiki.yandex-team.ru/samogon/)
![sandbox.png](img/services/sandbox.png "sandbox.png" =32x) |  SANDBOX | Ссылка на задачу или ресурс в [Sandbox](https://sandbox.yandex-team.ru/)
![startrek.png](img/services/startrek.png "startrek.png" =32x) |  STARTREK | Ссылка на задачу в [Startek](https://st.yandex-team.ru/)
![storybook.png](img/services/storybook.png "storybook.png" =32x) |  STORYBOOK | Иконка [Storybook](https://storybook.js.org/)
![taxi_tariff_editor.png](img/services/taxi_tariff_editor.png "taxi_tariff_editor.png" =32x) |  TAXI_TARIFF_EDITOR | Ссылка на [Админку всея Такси](https://tariff-editor.taxi.yandex-team.ru/)
![teamcity.png](img/services/teamcity.png "teamcity.png" =32x) |  TEAMCITY | Ссылка на сборку в [Teamcity](htts://teamcity.yandex-team.ru/)
![thirium.png](img/services/thirium.png "thirium.png" =32x) |  THIRIUM | Ссылка на сборку в [Thirium](https://th.yandex-team.ru/)
![tsum.png](img/services/tsum.png "tsum.png" =32x) |  TSUM_RELEASE или TSUM_MULTITESTING | Ссылка на [ЦУМ](https://tsum.yandex-team.ru/)
![yandex_deploy.png](img/services/yandex_deploy.png "yandex_deploy.png" =32x) |  YANDEX_DEPLOY | Ссылка на окружение в [Yandex.Deploy](https://yd.yandex-team.ru/)
![yandex_vault.png](img/services/yandex_vault.png "yandex_vault.png" =32x) |  YANDEX_VAULT | Ссылка на ресурсы в [Секретнице](https://yav.yandex-team.ru/)
![yt.png](img/services/yt.png "yt.png" =32x) |  YT | Ссылка на ресурсы в [YT](https://yt.yandex-team.ru/)

{% note info %}

Если требуется добавить новую иконку, то нужно прислать пулл-реквест.

Хороший пример такого пулл-реквеста: [2690122](https://a.yandex-team.ru/review/2690122/details).

Новую иконку можно использовать в CI сразу же, но появится она только после релиза [интерфейса Arcanum](https://a.yandex-team.ru/projects/devinterfacesdevelopmentteam/ci/releases/timeline?dir=data-ui%2Farcanum&id=release). Новый релиз выкатывается, как правило, раз в неделю по вторникам.

{% endnote %}

## Список коммитов { #changelog }
Получить список коммитов, которые вошли в запуск, можно из ручки `getCommits` в `ci.CiService`.

В случае если это запуск релиза, то будет возвращены коммиты, которые в релизе на момент выполнения запроса `getCommits`.
Этот список может поменяться, если на момент выполнения запроса существует предыдущий незавершенный релиз и который
впоследствии будет отменен.

В случае если флоу запущен как action, то будет возвращаен единственный коммит, на котором произведен запуск.
Для запусков в PR возвращается merge-коммит этого пулл-реквеста в транк.

Пример использования: [ci/tasklet/registry/demo/changelog/impl/__init__.py](https://a.yandex-team.ru/arc_vcs/ci/tasklet/registry/demo/changelog/impl/__init__.py).

Описание входных и выходных параметров: **GetCommitsRequest** и **GetCommitsResponse** в [ci/tasklet/common/proto/service.proto](https://a.yandex-team.ru/arc_vcs/ci/tasklet/common/proto/service.proto).

## Ссылка на отчет в Sandbox { #sandbox-report }

Аналогично информации о состоянии задачи можно настроить отображение ссылки на [отчёт Sandbox](https://docs.yandex-team.ru/sandbox/dev/report) в задачах, созданных на основе задач в Sandbox. Для этого нужно добавить в [YAML](job-sandbox.md#yaml) файл с метаданными задачи следующую секцию:

```yaml
title: TestTask2
description: Run Sandbox TEST_TASK_2
maintainers: my-abc-service-slug

sandbox-task:
  name: TEST_TASK_2
  badges-configs: # Ссылки на результаты выполнения задачи в виде отчетов Sandbox
    - id: report_example # Идентификатор отчета в Sandbox
      module: SANDBOX # Иконка, которую нужно отобразить
```

## Использование выражений { #expressions }

В YAML файлах, хранящихся в [реестре задач](https://a.yandex-team.ru/arc/trunk/arcadia/ci/registry) можно использовать [выражения](expression.md), аналогично тому, как это делается в **a.yaml**:

```yaml
title: TestTask2
description: Run YA_PACKAGE
maintainers: my-abc-service-slug

sandbox-task:
    name: YA_PACKAGE

parameters:
    build_system: ya
    build_type: release
    checkout_arcadia_from_url: "arcadia-arc:/#${context.target_revision.hash}" # Будет подставлена текущая ревизия
    resource_type: YA_PACKAGE
```

## Deprecated задачи { #deprecated }

Задачи в реестре, которые устарели и должны быть заменены, можно пометить флагом `deprecated` и текстом:
```yaml
title: TestTask
description: Run YA_PACKAGE
maintainers: my-abc-service-slug
deprecated: |
    Please consider migration to common/arcadia/ya_package_2
...
```

При наличии action-ов и релизов с такими задачами будет выведено предупреждение в pull request-е при изменении `a.yaml`.
