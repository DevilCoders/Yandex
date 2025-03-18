## Table of Contents
0. [TLDR](#tldr)
1. [Deploy](#deploy-tasklet)
2. [Create dev branch](#create_dev_branch-tasklet)
3. [Remove dev branch](#remove_dev_branch-tasklet)
4. [Wait job](#wait_job-tasklet)

## TLDR <a name="tldr"></a>

В файле [example-a.yaml](https://a.yandex-team.ru/arc/trunk/arcadia/ci/registry/projects/taxi/clownductor/example-a.yaml) описаны несколько реальных примеров использования тасклетов вместе.

### Создание дев стенда на PR
1. При создании/обновлении PR
    1. Создаем стенд через тасклет `create_dev_branch`
    2. Билдим проект
    3. Ожидаем создания стенда через тасклет `wait_job`
    4. Деплоим проект на стенд через тасклет `deploy`
    5. Ожидаем окончания деплоя через `wait_job`
    6. Пишем в telegram чат об этом замечательном событии через `send_telegram_message`
2. При мерже/отмене PR
    1. Останавливаем работающие джобы
    2. Удаляем стенд через тасклет `remove_dev_branch`

<details>
<summary markdown="span">Флоу в интерфейсе</summary>

![ci flow example](https://jing.yandex-team.ru/files/rifler/2022-03-25_19-37-54.png "ci flow example")
</details>

## projects/taxi/clownductor/deploy <a name="deploy-tasklet"></a>

Тасклет вызывает ручку `/api/teamcity_deploy` у Клауна

[Реализация](https://a.yandex-team.ru/arc_vcs/taxi/tasklets/clownductor/deploy)

[Proto схема](https://a.yandex-team.ru/arc_vcs/taxi/tasklets/clownductor/deploy/proto/deploy.proto)

Конфигурирование
```yaml
ci:
  # ... secret, runtime, releases, actions ...

  flows:
    my-flow:
      title: My Flow
      jobs:
        clown-deploy-example:
          title: Deploy to clown
          task: projects/taxi/clownductor/deploy
          input:
            config:
              origin: https://any.host.ru # по умолчанию `https://clownductor.taxi.yandex.net`
              secret_key: any_key # ключ из секрета, по умолчанию `clownductor-token`
              data: # всё что принимает ручка на вход, см. proto схему
                env: unstable
                service_name: grocery-superapp
                project_name: lavka-frontend
                docker_image: lavka/grocery-superapp/unstable:0.2.808
                branch_name: unstable
```

## projects/taxi/clownductor/create_dev_branch <a name="create_dev_branch-tasklet"></a>

Тасклет вызывает ручку `/v1/create_dev_branch` у Клауна

[Реализация](https://a.yandex-team.ru/arc_vcs/taxi/tasklets/clownductor/create_dev_branch)

[Proto схема](https://a.yandex-team.ru/arc_vcs/taxi/tasklets/clownductor/create_dev_branch/proto/create_dev_branch.proto)

В коде объявлены дефолтные значения для allocate_request и для env (`unstable`).

Для быстрого старта, по сути, достаточно передать только `service_id` и `data.name`

Конфигурирование
```yaml
ci:
  # ... secret, runtime, releases, actions ...

  flows:
    my-flow:
      title: My Flow
      jobs:
        clown-create-dev-branch-example:
          title: Create dev branch
          task: projects/taxi/clownductor/create_dev_branch
          input:
            config:
              service_id: 355571 # числовой id сервиса из Админки, обязательный
              origin: https://any.host.ru # по умолчанию `https://clownductor.taxi.yandex.net`
              secret_key: any_key # ключ из секрета, по умолчанию `clownductor-token`
              # в пресете содержится инфа о cpu и ram
              # значения - piko | nano, по умолчанию piko
              # будут использоваться значения из пресета, если только они явно не переданы в allocate_request
              preset: nano
              # список регионов, из которых будет рандомно выбран один регион, если только он явно не передан в allocate_request
              # по умолчанию ['sas', 'vla', 'man']
              regions:
                - sas
                - some_new_dc
              data: # всё что принимает ручка на вход, см. proto схему
                name: my-new-dev-branch
                env: unstable
                allocate_request:
                  ram: 100500
                  regions:
                    - sas
```

## projects/taxi/clownductor/remove_dev_branch <a name="remove_dev_branch-tasklet"></a>

Тасклет вызывает ручку `/v1/remove_dev_branch` у Клауна

[Реализация](https://a.yandex-team.ru/arc_vcs/taxi/tasklets/clownductor/remove_dev_branch)

[Proto схема](https://a.yandex-team.ru/arc_vcs/taxi/tasklets/clownductor/remove_dev_branch/proto/remove_dev_branch.proto)

Есть два варианта использования:
1. Известен числовой идентификатор бранчи. В таком случае достаточно передать только `branch_id`.
2. Известно имя бранчи. В таком случае нужно передать `service_id` и `branch_name`.

---
**NOTE**

Клаун принудительно добавляет приставку `dev-` при создании бранча.

Если создавали бранч с именем `my-branch`, то удалять нужно `dev-my-branch`

---

В файле [example-a.yaml](https://a.yandex-team.ru/arc/trunk/arcadia/ci/registry/projects/taxi/clownductor/example-a.yaml) продемонстрированы оба варианта.

Конфигурирование
```yaml
ci:
  # ... secret, runtime, releases, actions ...

  flows:
    my-flow:
      title: My Flow
      jobs:
        clown-remove-dev-branch-example:
          title: Remove clown dev branch job
          task: projects/taxi/clownductor/remove_dev_branch
          input:
            config:
              origin: https://any.host.ru # по умолчанию `https://clownductor.taxi.yandex.net`
              secret_key: any_key # ключ из секрета, по умолчанию `clownductor-token`
              # вариант 1
              branch_id: 1234
              # вариант 2
              service_id: 4567
              branch_name: dev-my-branch
```


## projects/taxi/clownductor/wait_job <a name="wait_job-tasklet"></a>

Ожидает перехода джобы в статус `success`

[Реализация](https://a.yandex-team.ru/arc_vcs/taxi/tasklets/clownductor/wait_job)

[Proto схема](https://a.yandex-team.ru/arc_vcs/taxi/tasklets/clownductor/wait_job/proto/wait_job.proto)

Конфигурирование
```yaml
ci:
  # ... secret, runtime, releases, actions ...

  flows:
    my-flow:
      title: My Flow
      jobs:
        clown-deploy-example:
          title: Wait clown job
          task: projects/taxi/clownductor/wait_job
          input:
            config:
              origin: https://any.host.ru # по умолчанию `https://clownductor.taxi.yandex.net`
              job_id: 1497610
              wait_before_attempt: 20 # пауза перед запросами в секундах, по умолчанию 15
```
