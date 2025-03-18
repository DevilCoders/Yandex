# common/monitoring/juggler_watch
### Описание:
- Поллит указанные juggler мониторинги, фэйлится при "CRIT" на одном из них

### Sources:
- [/ci/tasklet/registry/common/monitoring/juggler_watch](https://a.yandex-team.ru/arc/trunk/arcadia/ci/tasklet/registry/common/monitoring/juggler_watch)
### Input resources:
- type [juggler_watch.Config](https://a.yandex-team.ru/arc/trunk/arcadia/ci/tasklet/registry/common/monitoring/juggler_watch/proto/juggler_watch.proto) (имя поля config) - артефакт конфига.
содержит в себе:
    - ```filters``` (repeated): - артефакт для фильтрации необходимых мониторингов. Содержит в себе фильтры мониторингов по namespace, host, service, tags (см. [документацию Juggler Api](https://juggler.yandex-team.ru/doc/#/checks//v2/checks/get_checks_state)).
    - ```fail_on_warn```: bool (additional, если true, тасклет фэйлится также при статусе "WARN")
    - ```delay_minutes```: integer (additional, если указано, поллинг начнется после указанного времени после старта тасклета)
    - ```duration_minutes```: integer (additional, если не указано, проверяет мониторинги лишь один раз, иначе на протяжении указанного времени, каждые 10 секунд)

### Пример запуска:
- [ci/tasklet/registry/common/monitoring/juggler_watch/run.example.sh](https://a.yandex-team.ru/arc/trunk/arcadia/ci/tasklet/registry/common/monitoring/juggler_watch/run.example.sh)

### Пример использования:
- ```
    monitoring-check:
        title: Проверка мониторингов TestEnv
        task: common/monitoring/juggler_watch
        input:
        config:
            delayMinutes: 10
            durationMinutes: 30
            failOnWarn: true
            filters:
                - namespace: devtools.testenv
                  tags:
                    - testenv_alert
                    - testenv_check
                  service: sampleSolomonService
                  host: sampleSolomonHost
    ```
- [ci/a.yaml::monitoring](https://a.yandex-team.ru/arc/trunk/arcadia/ci/a.yaml)
---
# common/monitoring/infra_create
### Описание:
- Создает event в infra.yandex-team.ru

### Sources:
- [/ci/tasklet/registry/common/monitoring/infra_create](https://a.yandex-team.ru/arc/trunk/arcadia/ci/tasklet/registry/common/monitoring/infra_create/)
### Input resources :
- type [infra_create.Config](https://a.yandex-team.ru/arc/trunk/arcadia/ci/tasklet/registry/common/monitoring/infra_create/proto/infra_create.proto) (имя поля config) - артефакт конфига.
(Подробный формат можно найти [тут](https://a.yandex-team.ru/arc/trunk/arcadia/ci/tasklet/registry/common/monitoring/infra_create/proto/infra_create.proto))
содержит в себе:
  - ```placement```: Содержит данные о service/environment (`!ATTENTION!` `serviceId` и `environmentId` написаны мелким серым шрифтом в UI infra, на соответствующих вкладках)
  - ```status```: Содержит type, severity ивента
  - ```textual```: Содержит заголовок и описание ивента
  - ```id```: integer - Уникальное id, хранящееся в метаданных ивента, защищающее от создания дубликатов
  - ```do_not_send_emails```: bool - При создании ивента, ifra посылает сообщения всем подписчикам сервиса. Выставьте True, если хотите этого избежать.
  - ```yav_infra_oauth_token_key``` - ключ OAuth токена для infra.yandex-team.ru для указанного в a.yaml секрета. (см. Примечание 1)

### Output resources :
- ```InfraIdentifier``` - артефакт, содержащий в себе id созданного ивента

### Пример запуска:
- - [ci/tasklet/registry/common/monitoring/infra_create/run.example.sh](https://a.yandex-team.ru/arc/trunk/arcadia/ci/tasklet/registry/common/monitoring/infra_create/run.example.sh)

### Пример использования:
- ```
        infra-create:
          title: Создание ивента в инфре
          task: common/monitoring/infra_create
          input:
            config:
              textual:
                title: DummyInfraFlowTest
                description: DummyInfraFlowTest
              status:
                severity: MINOR
                type: MAINTENANCE
              placement:
                serviceId: 1877
                environmentId: 2983
                dataCenters:
                    - sas
                    - man
              doNotSendEmails: true
    ```
- [ci/a.yaml::monitoring](https://a.yandex-team.ru/arc/trunk/arcadia/ci/a.yaml)
---
# common/monitoring/infra_close
### Описание:
- Завершает выбранное событие в infra.yandex-team.ru, проставляя finishTime как текущее время.

### Sources:
- [/ci/tasklet/registry/common/monitoring/infra_close](https://a.yandex-team.ru/arc/trunk/arcadia/ci/tasklet/registry/common/monitoring/infra_close/)
### Input resources :
- type [infra_create.Config](https://a.yandex-team.ru/arc/trunk/arcadia/ci/tasklet/registry/common/monitoring/infra_close/proto/infra_close.proto) (имя поля config) - артефакт конфига
содержит в себе:
  -```yav_infra_oauth_token_key``` - ключ OAuth токена для infra.yandex-team.ru для указанного в a.yaml секрета. (см. Примечание 1)
  - ```InfraIdentifier``` - артефакт, содержащий в себе id созданного ивента

### Пример запуска:
- [ci/tasklet/registry/common/monitoring/infra_close/run.example.sh](https://a.yandex-team.ru/arc/trunk/arcadia/ci/tasklet/registry/common/monitoring/infra_close/run.example.sh)

### Пример flow:
- ```
        infra-close:
          title: Закрытие ивента в инфре
          task: common/monitoring/infra_close
          needs: infra-create
          input:
            config: {}  # default
    ```
    Заметьте, что не прописать поле `config` в текущиий момент нельзя - мы это исправим в ближайшее время
- [ci/a.yaml::monitoring](https://a.yandex-team.ru/arc/trunk/arcadia/ci/a.yaml)

### Решение проблем

#### unexpected multiple resources of following types: ci.monitoring.infra_create.InfraIdentifier (got from ..., ...)
Судя по всему, в графе сборке есть 2 генерации события Infra (`infra_create`), причем оба они доступные по графу сборки текущему `infra_close` (например, если отдельное Infra событие на Prestable, и на Stable - и валидация на Stable падает).

Второй `infra_close` автоматически получает все ресурсы с типом "ci.monitoring.infra_create.InfraIdentifier" из графа (т.е. из результата выполнения всех задач выше по графу), хотя ожидает ровно 1 ресурс.

Решить проблему можно с помощью https://docs.yandex-team.ru/ci/expression
И именно, написать что-то вроде
```
    infra-close:
        title: Закрытие ивента в инфре
        task: common/monitoring/infra_close
        needs: infra-create
        input:
          config: {}
          infra_identifier: ${tasks.create_infra_event_stable.infra_identifier}
```

Где `create_infra_event_stable` - название задачи с типом `infra_create`, которая создала Infra событие в релизе и которое нужно закрыть (обычно это задача ближайшая по графу).

# common/monitoring/infra_check_events
### Описание:
- Проверяет наличие текущих событий в infra.yandex-team.ru и может дожидаться их окончания.
- Умеет проверять будут ли события вперед на какое-то время

### Sources:
- [/ci/tasklet/registry/common/monitoring/infra_check_events](https://a.yandex-team.ru/arc/trunk/arcadia/ci/tasklet/registry/common/monitoring/infra_check_events/)
### Input resources :
- type [infra_check_events.Config](https://a.yandex-team.ru/arc/trunk/arcadia/ci/tasklet/registry/common/monitoring/infra_check_events/proto/infra_check_events.proto) (имя поля config) - артефакт конфига
  содержит в себе:
  - ```filters```:
      - ```placement```: Содержит данные о service/environment (`!ATTENTION!` `serviceId` и `environmentId` написаны мелким серым шрифтом в UI infra, на соответствующих вкладках)
      - ```status```: Содержит type, severity ивента
  - ```wait_for_finish```: булевский флаг, в случае `true` тасклет дожидается окончания событий (пока позволяет [kill_timeout](https://docs.yandex-team.ru/ci/runtime#sandbox)), в случае `false` - завершается с ошибкой при наличии текущих событий
  - ```time_before_infra_event```: строка вида "3h 55min", которая задает время вперед на которое не должно быть событий

### Пример запуска:
- [ci/tasklet/registry/common/monitoring/infra_check_events/run.example.sh](https://a.yandex-team.ru/arc/trunk/arcadia/ci/tasklet/registry/common/monitoring/infra_check_events/run.example.sh)

### Пример flow:
- ```
        infra-check-events:
          title: Ожидание завершения infra события
          task: common/monitoring/infra_check_events
          input:
            config:
              wait_for_finish: true
              time_before_infra_event: "3h 15m"
              filters:
                - placement:
                    serviceId: 3346
                    environmentId: 5189
                  status:
                    type: MAINTENANCE
                    severity: MINOR
    ```

---
Примечания
- 1: для получения токена нужно от имени робота зайти [по ссылке](https://oauth.yandex-team.ru/authorize?response_type=token&client_id=f824e92d0ff44e879084f2fe5c1b187b) (ссылка взята [здесь](https://wiki.yandex-team.ru/infra/#faq))
