# Подготовка релиза
[Общая инструкция по деплою сервисов Облака](https://wiki.yandex-team.ru/cloud/devel/deploy/)

## Создать релизный тикет

Создать [заявку на релиз](https://forms.yandex-team.ru/surveys/11002), указать Component `IAM` и Roles to update `iam`. При необходимости добавить Roles to update `common`.

Добавить в релизный тикет чек-лист с нужными окружениями:
```
1d testing phase: internal-dev
1d testing phase: testing
1d pre-prod phase: internal-prestable
1d pre-prod phase: pre-prod
2d prod phase: internal-prod part 1 (myt1)
2d prod phase: internal-prod part 2 (all except myt1)
3d prod phase: prod part 1 (myt1), israel part 1 (iam-il1-a1)
4d prod phase: prod part 2 (myt except myt1), israel part 2 (all but iam-il1-a1)
5d prod phase: prod part 3 (vla and sas)
```

Найти предыдущий релизный тикет и слинковать его с текущим.
[Пример поиска](https://st.yandex-team.ru/CLOUD/order:key:false/filter?type=12&components=2369).

Дальше в скриптах использовать переменную с идентификатором текущего релизного тикета:
```bash
TICKET="CLOUD_XXX"
```
## Обновить `fixture_permissions.py`
{% cut "Выполняем этот пункт только для стендов, на которые роли деплоятся через identity, например для hw-labs." %}
### Обновить `fixture_permissions.py` в репозитории `identity`

Сборка [iam-compile-role-fixtures](https://teamcity.yandex-team.ru/buildConfiguration/Cloud_CloudGo_IamCompileRoleFixtures?branch=%3Cdefault%3E&mode=builds) запускается автоматически при измении файлов в `cloud-go` по маске `private-api/yandex/cloud/priv/**.yaml`)

1. Либо скачать `fixture_permissions.py` в ручную с вкладки `Artifacts` последней успешной сборки [iam-compile-role-fixtures](https://teamcity.yandex-team.ru/buildConfiguration/Cloud_CloudGo_IamCompileRoleFixtures?branch=%3Cdefault%3E&mode=builds).
2. Либо скачать [fixture_permissions.py](https://teamcity.yandex-team.ru/repository/download/Cloud_CloudGo_IamCompileRoleFixtures/.lastFinished/fixture_permissions.py) (ссылка ведёт непосредственно на нужный файл в артефактах последней завершённой сборки).
3. Заменить [identity/yc_identity/fixture_permissions.py](https://bb.yandex-team.ru/projects/CLOUD/repos/identity/browse/identity/yc_identity/fixture_permissions.py) на скачанный файл.

Вспомогательный скрипт:
```bash
cd identity

git checkout master
git pull

git checkout -b ${TICKET?}-update-fixtures

# заменить identity/yc_identity/fixture_permissions.py на скачанный файл
mv --force ~/Downloads/fixture_permissions.py identity/yc_identity/fixture_permissions.py
# or use the following command for Mac OS
#mv -f ~/Downloads/fixture_permissions.py identity/yc_identity/fixture_permissions.py

git add identity/yc_identity/fixture_permissions.py
git commit -m "update fixtures
${TICKET?}"

git push -u origin HEAD
```

Merge the changes into master ([repository](https://bb.yandex-team.ru/projects/CLOUD/repos/identity/pull-requests)).

### Зафиксировать изменения, попадающие в обновление `fixture_permissions.py` из репозитория `cloud-go`

Взять хеш последнего коммита в репозиторию `cloud-go`, попавшего в обновление `fixture_permissions.py`, из параметров сборки [iam-compile-role-fixtures](https://teamcity.yandex-team.ru/buildConfiguration/Cloud_CloudGo_IamCompileRoleFixtures?branch=%3Cdefault%3E&mode=builds), которую использовали для обновления фикстур.
В крайнем случае использовать `HEAD` вместо конкретного коммита.
```bash
CURRENT_FIXTURES_HASH="xxxxxxx"
```

Найти хеш последнего коммита в `cloud-go`, попавшего в предыдущий релизный тикет (должен присутствовать в секции `Changelog`/Фикстуры в качестве значения `CURRENT_FIXTURES_HASH`).
```bash
PREVIOUS_FIXTURES_HASH="xxxxxxx"
```

Обновить рабочую копию `cloud-go`:
```bash
cd cloud-go

git checkout master
git pull
```

Получить список коммитов с изменениями в `private-api/yandex/cloud/priv/**.yaml` файлах, произошедших с предыдущего релиза:
```bash
git log --oneline ${PREVIOUS_FIXTURES_HASH?}..${CURRENT_FIXTURES_HASH?} -- 'private-api/yandex/cloud/priv/**.yaml'
```

Получить список тикетов из коммитов:
```bash
# Linux
git log ${PREVIOUS_FIXTURES_HASH?}..${CURRENT_FIXTURES_HASH?} -- 'private-api/yandex/cloud/priv/**.yaml' \
  | grep --perl-regexp --regexp='(?<![A-Z0-9])[A-Z]+-[0-9]+(?![A-Z0-9])' --only-matching \
  | sort \
  | uniq
# Mac
git log ${PREVIOUS_FIXTURES_HASH?}..${CURRENT_FIXTURES_HASH?} -- 'private-api/yandex/cloud/priv/**.yaml' \
  | egrep -o "[a-zA-Z0-9]+-[0-9]+" \
  | sort \
  | uniq
```

Занести результаты в релизный тикет в секцию `Changelog` в виде
````
### Фикстуры
<{коммиты в `cloud-go` с изменениями в `private-api/yandex/cloud/priv/**.yaml`
```
CURRENT_FIXTURES_HASH="xxxxxxx"
PREVIOUS_FIXTURES_HASH="xxxxxxx"
git log --oneline ${PREVIOUS_FIXTURES_HASH?}..${CURRENT_FIXTURES_HASH?} -- 'private-api/yandex/cloud/priv/**.yaml'
<<<список коммитов>>>
```
}>
<<<список тикетов>>>
````
{% endcut %}

## Собрать пакет `identity`

Собрать [yc-identity](https://teamcity.aw.cloud.yandex.net/buildConfiguration/IAM_Packages_YcIdentity).

Дальше в скриптах использовать переменную с версией пакета:
```bash
IDENTITY_VERSION="0_X_X_XXXX_YYMMDD"
```

## Зафиксировать изменения, попадающие в обновление пакета `identity`

Взять хеш последнего коммита в репозиторию `identity`, попавшего в сборку пакета `identity`, из параметров сборки [yc-identity](https://teamcity.aw.cloud.yandex.net/buildConfiguration/IAM_Packages_YcIdentity), которая идёт в релиз.
В крайнем случае использовать `HEAD` вместо конкретного коммита.
```bash
CURRENT_IDENTITY_HASH="xxxxxxx"
```

Найти хеш последнего коммита в `identity`, попавшего в предыдущий релизный тикет (должен присутствовать в секции `Changelog`/Identity в качестве значения `CURRENT_IDENTITY_HASH`).
```bash
PREVIOUS_IDENTITY_HASH="xxxxxxx"
```

Обновить рабочую копию `identity`:
```bash
cd identity

git checkout master
git pull
git submodule update --recursive
```

Получить список коммитов, произошедших с предыдущего релиза:
```bash
git log --oneline ${PREVIOUS_IDENTITY_HASH?}..${CURRENT_IDENTITY_HASH?}
```

Получить список тикетов из коммитов:
```bash
# Linux
git log ${PREVIOUS_IDENTITY_HASH?}..${CURRENT_IDENTITY_HASH?} \
  | grep --perl-regexp --regexp='(?<![A-Z0-9])[A-Z]+-[0-9]+(?![A-Z0-9])' --only-matching \
  | sort \
  | uniq
# Mac
git log ${PREVIOUS_IDENTITY_HASH?}..${CURRENT_IDENTITY_HASH?} \
  | egrep -o "[a-zA-Z0-9]+-[0-9]+" \
  | sort \
  | uniq
```

Занести результаты в релизный тикет в секцию `Changelog` в виде
````
### Identity
<{коммиты в `identity`
```
CURRENT_IDENTITY_HASH="xxxxxxx"
PREVIOUS_IDENTITY_HASH="xxxxxxx"
git log --oneline ${PREVIOUS_IDENTITY_HASH?}..${CURRENT_IDENTITY_HASH?}
<<<список коммитов>>>
```
}>
<<<список тикетов>>>
````

## Обновить версию `identity` в репозитории `salt-formula`

Вспомогательный скрипт:
```bash
cd salt-formula

git checkout master
git pull

git checkout -b ${TICKET?}-update-identity

# поменять значение identity_version в pillar/common/iam-common.sls

git add pillar/common/iam-common.sls
git commit -m "update identity to ${IDENTITY_VERSION?}
${TICKET?}"

git push -u origin HEAD
```

Как вариант, можно создать пул-реквест из UI, отредактировав [iam-common.sls](https://bb.yandex-team.ru/projects/CLOUD/repos/salt-formula/browse/pillar/common/iam-common.sls).
В качестве текста коммита использовать
```
update identity to ${IDENTITY_VERSION?}
${TICKET?}
```

При необходимости добавить в этот же пул-реквест другие изменения в `iam-identity.sls` файлах, для которых не делалось отдельных пул-реквестов. Не забыть слинковать тикеты на эти изменения не только с пул-реквестом, но и срелизным тикетом.

Create a new Pull Request [here](https://bb.yandex-team.ru/projects/CLOUD/repos/salt-formula/pull-requests/).

В описании PR необходимо на отдельной строке написать `needci` для запуска CI для PR — без этого комментария необходимая проверка в TeamCity запущена не будет.

## Обновить версию `salt-formula` в репозитории `cluster-configs`

После сборки пакета [salt-formula](https://teamcity.yandex-team.ru/buildConfiguration/Cloud_Packages_Platform_SaltFormula?branch=%3Cdefault%3E&mode=builds#all-projects) 
прописать новую версию в `cluster-configs` в [testing/base-role-releases/iam.yaml](https://bb.yandex-team.ru/projects/CLOUD/repos/cluster-configs/browse/testing/base-role-releases/iam.yaml)
и в [prod/base-role-releases/iam-internal-dev.yaml](https://bb.yandex-team.ru/projects/CLOUD/repos/cluster-configs/browse/prod/base-role-releases/iam-internal-dev.yaml) для роли `iam-ya-internal-dev`.

## Зафиксировать изменения, попадающие в обновление `salt-formula`

Взять хеш последнего коммита в репозиторию `salt-formula`, попавшего в сборку пакета `salt-formula`, из параметров сборки [salt-formula](https://teamcity.yandex-team.ru/buildConfiguration/Cloud_Packages_Platform_SaltFormula?branch=%3Cdefault%3E&mode=builds#all-projects), которая идёт в релиз.
В крайнем случае использовать `HEAD` вместо конкретного коммита.
```bash
CURRENT_SALT_HASH="xxxxxxx"
```

Найти хеш последнего коммита в `salt-formula`, попавшего в предыдущий релизный тикет (должен присутствовать в секции `Changelog`/Salt в качестве значения `CURRENT_SALT_HASH`).
```bash
PREVIOUS_SALT_HASH="xxxxxxx"
```

Обновить рабочую копию `salt-formula`:
```bash
cd salt-formula

git checkout master
git pull
```

Получить список коммитов с изменениями в `pillar/**/iam-identity.sls`, произошедших с предыдущего релиза:
```bash
git log --oneline ${PREVIOUS_SALT_HASH?}..${CURRENT_SALT_HASH?} -- 'pillar/**/iam-identity.sls'
```

Получить список тикетов из коммитов:
```bash
# Linux
git log ${PREVIOUS_SALT_HASH?}..${CURRENT_SALT_HASH?} -- 'pillar/**/iam-identity.sls' \
  | grep --perl-regexp --regexp='(?<![A-Z0-9])[A-Z]+-[0-9]+(?![A-Z0-9])' --only-matching \
  | sort \
  | uniq
# Mac
git log ${PREVIOUS_SALT_HASH?}..${CURRENT_SALT_HASH?} -- 'pillar/**/iam-identity.sls' \
  | egrep -o "[a-zA-Z0-9]+-[0-9]+" \
  | sort \
  | uniq
```

В результатах может потребоваться отфильтровать обновления версии `identity`, сделанные в рамках текущего или предыдущего релизного тикета.

Занести результаты в релизный тикет в секцию `Changelog` в виде
````
### Salt
<{коммиты в `salt-formula` c изменениями в `pillar/**/iam-identity.sls`
```
CURRENT_SALT_HASH="xxxxxxx"
PREVIOUS_SALT_HASH="xxxxxxx"
git log --oneline ${PREVIOUS_SALT_HASH?}..${CURRENT_SALT_HASH?} -- 'pillar/**/iam-identity.sls'
<<<список коммитов>>>
```
}>
<<<список тикетов>>>
````

# Релиз

**ВНИМАНИЕ! Роль iam не выкатывать, если в зоне осталось меньше 4 рабочих машин с ней.**
1. Релиз обычно выкатывает дежурный
2. После каждого стенда **надо посмотреть на мониторинги и логи**, убедиться, что всё ок:
    ```bash
    # Для просмотра логов Identity на машине iam смотрим:
    sudo journalctl -u 'yc-iden*' -n 10000
    ```
4. После этого тикет переводится в состояние Testing in Dev/Testing in PreProd/Released в зависимости от стенда.
5. Прод сначала выкатывается на одну машину, потом в одну AZ, потом все остальные.
6. Если на проде что-то пошло не так в процессе выкатки на машину или AZ, то сначала нужно погасить на них сервисы,
потом разбираться.
7. Между релизами стендов и отдельных машин нужно выдерживать определённые паузы на обнаружение ошибок и откат в соответствии с [регламентом](https://docs.yandex-team.ru/iam-playbook-operation/release/)


1. Зайти на специальный хост для выкатки обновлений:
    ```bash
    pssh bootstrap.cloud.yandex.net
    ```
2. Заходим в screen/tmux в закладку, из которой будем проходить весь релиз:
    ```bash
    # один вариант из двух
    screen
    tmux
    ```
3. Сохраняем общие параметры релиза для `yc-bootstrap` в сессию на bootstrap машине и в описание релизного тикета :
    ```bash
    TICKET="CLOUD_XXXXX"
    SALT_VERSION="X_X_XXXX_YYMMDD"
    ```

## Проверить наличие миграций

1. Заходим на страницу [migrations](https://wiki.yandex-team.ru/cloud/iamrm/deploy/migrations/) и проверяем наличие необходимых для релиза миграций БД.
2. Если миграции есть, то выполняем скрипты на соответствующим стенде на каждом шаге релиза. [Доступ к БД стендов](https://wiki.yandex-team.ru/cloud/iamrm/duty/faq/#tochkidostupadobazstendoviam).
3. На всякий случай перед релизом всё равно уточнить у коллег в IAM&RM чате все ли указали свои миграции, если они были.
4. Если релиз в какой-то момент упадёт из-за отсутствия миграций, то идёт к автору изменения и просим добавить миграцию в список.

## testing phase: internal-dev

{% note info %}

Ожидается, что при подготовке релиза новая версия уже была прописана в [prod/base-role-releases/iam-internal-dev.yaml](https://bb.yandex-team.ru/projects/CLOUD/repos/cluster-configs/browse/prod/base-role-releases/iam-internal-dev.yaml)
для salt ролей iam-internal-dev и при необходимости common.

{% endnote %}

#### Шаблон для шага в релизном тикете

Копируем шаблон ниже в новый комментарий в релизный тикет.

Начинаем выполнять последовательно и после каждой операции фиксируем результат в этом же комментарии через его редактирование.

    ## testing phase: internal-dev

    ### set [downtime](https://juggler.yandex-team.ru/downtimes/) on:

    ```
    host=yc_iam_svm_internal-dev
    ```

    ### Миграции

    Не забыть применить миграции, если они есть. См. "Подготовка к релизу" в инструкции.

    ### Параметры для yc-bootstrap

    Убеждаемся, что работаем во вкладке с переменными окружения `TICKET` и `SALT_VERSION`.

    ### release common

    ```bash
    TARGET_ENV="prod"
    SALT_ROLE="common"
    BASE_ROLE="iam-internal-dev"
    yc-bootstrap --apply \
        --template "workflows/${TARGET_ENV?}/${SALT_ROLE?}/update/${BASE_ROLE?}.yaml" \
        --ticket-id "${TICKET?}" \
        --filter "%${BASE_ROLE?}"
    ```

    <[
    TODO copy the command output here, starting from the Running step finalizers section
    ]>

    ### release iam

    ```bash
    TARGET_ENV="prod"
    SALT_ROLE="iam-internal-dev"
    BASE_ROLE="iam-internal-dev"
    yc-bootstrap --apply \
        --template "workflows/${TARGET_ENV?}/${SALT_ROLE?}/update/allaz.yaml" \
        --ticket-id "${TICKET?}" \
        --filter "%${BASE_ROLE?}"
    ```

    <[
    TODO copy the command output here, starting from the Running step finalizers section
    ]>

    ### unset [downtimes](https://juggler.yandex-team.ru/downtimes/)

    После всех проверок снимаем downtimes, если они ещё активны.


## testing phase: testing

#### Шаблон для шага в релизном тикете

Копируем шаблон ниже в новый комментарий в релизный тикет.

Начинаем выполнять последовательно и после каждой операции фиксируем результат в этом же комментарии через его редактирование.

    ## testing phase: testing

    ### Monitoring

    Следим за
    [сервисами](https://grafana.yandex-team.ru/d/iam-duty-deploy/iam-duty-deploy?orgId=1&from=now-3h&to=now&refresh=1m&var-appFilter=*_server&var-ds=Solomon%20Cloud%20Preprod&var-cluster=testing&var-host=All),
    за выкатываемыми нодами
    [Indentity](https://grafana.yandex-team.ru/d/iam-identity_dd-identity-by-host/identity-by-hosts?orgId=1&from=now-3h&to=now&var-ds=Solomon%20Cloud%20Preprod&var-cluster=testing&var-host=All&var-method=All&var-path=All&refresh=1m).

    ### set [downtime](https://juggler.yandex-team.ru/downtimes/) on:

    ```
    host=yc_iam_svm_testing
    ```

    ### Миграции

    Не забыть применить миграции, если они есть. См. "Подготовка к релизу" в инструкции.

    ### Параметры для yc-bootstrap

    Убеждаемся, что работаем во вкладке с переменными окружения `TICKET` и `SALT_VERSION`.

    ### release common

    ```bash
    TARGET_ENV="testing"
    SALT_ROLE="common"
    BASE_ROLE="iam"
    yc-bootstrap --apply \
        --template "workflows/${TARGET_ENV?}/${SALT_ROLE?}/update/${BASE_ROLE?}.yaml" \
        --ticket-id "${TICKET?}" \
        --filter "%${BASE_ROLE?}"
    ```

    <[
    TODO copy the command output here, starting from the Running step finalizers section
    ]>

    ### move release iam

    Прописать через PR нужную версию соли в `cluster-configs` в
    [testing](https://bb.yandex-team.ru/projects/CLOUD/repos/cluster-configs/browse/testing/base-role-releases/iam.yaml)
    окружении. (можно отредактировать и создать PR прямо в интерфейсе Bitbucket)

    > TODO_LINK_YOUR_PR: [PR](TODO_LINK_YOUR_PR)

    ### release iam to myt1

    ```bash
    TARGET_ENV="testing"
    SALT_ROLE="iam"
    BASE_ROLE="iam"
    yc-bootstrap --apply \
        --template "workflows/${TARGET_ENV?}/${SALT_ROLE?}/update/allaz.yaml" \
        --ticket-id "${TICKET?}" \
        --filter "%${BASE_ROLE?} iam-myt1.svc.cloud-testing.yandex.net"
    ```

    <[
    TODO copy the command output here, starting from the Running step finalizers section
    ]>

    ### release iam to all but myt1

    ```bash
    TARGET_ENV="testing"
    SALT_ROLE="iam"
    BASE_ROLE="iam"
    yc-bootstrap --apply \
        --template "workflows/${TARGET_ENV?}/${SALT_ROLE?}/update/allaz.yaml" \
        --ticket-id "${TICKET?}" \
        --filter "%${BASE_ROLE?} -iam-myt1.svc.cloud-testing.yandex.net"
    ```

    <[
    TODO copy the command output here, starting from the Running step finalizers section
    ]>

    ### unset [downtimes](https://juggler.yandex-team.ru/downtimes/)

    После всех проверок снимаем downtimes, если они ещё активны.


## pre-prod phase: internal-prestable

#### Шаблон для шага в релизном тикете

Копируем шаблон ниже в новый комментарий в релизный тикет.

Начинаем выполнять последовательно и после каждой операции фиксируем результат в этом же комментарии через его редактирование.

    ## pre-prod phase: internal-prestable

    ### Monitoring

    Пока не следим, т.к. нет графиков. Стенд в нестабильном состоянии.

    ### set [downtime](https://juggler.yandex-team.ru/downtimes/) on:

    Сейчас не влияет на процесс, т.к. нет alert-ов:
    ```
    host=yc_iam_svm_internal-prestable
    ```
    Стенд в нестабильном состоянии.

    ### Миграции

    Не забыть применить миграции, если они есть. См. "Подготовка к релизу" в инструкции.

    ### Параметры для yc-bootstrap

    Убеждаемся, что работаем во вкладке с переменными окружения `TICKET` и `SALT_VERSION`.

    ### move release versions

    Через Pull Request меняем версию роли iam-ya-internal-prestable и при необходимости common в [prod/base-role-releases/iam-internal-prestable.yaml](https://bb.yandex-team.ru/projects/CLOUD/repos/cluster-configs/browse/prod/base-role-releases/iam-internal-prestable.yaml)

    > TODO_LINK_YOUR_PR: [PR](TODO_LINK_YOUR_PR)

    ### release common to all

    ```bash
    TARGET_ENV="prod"
    SALT_ROLE="common"
    BASE_ROLE="iam-internal-prestable"
    yc-bootstrap --apply \
        --template workflows/${TARGET_ENV?}/${SALT_ROLE?}/update/${BASE_ROLE?}.yaml \
        --ticket-id ${TICKET?} \
        --filter %${BASE_ROLE?}
    ```

    <[
    TODO copy the command output here, starting from the Running step finalizers section
    ]>

    ### release iam to all

    ```bash
    TARGET_ENV="prod"
    SALT_ROLE="iam-internal-prestable"
    BASE_ROLE="iam-internal-prestable"
    yc-bootstrap --apply \
        --template workflows/${TARGET_ENV?}/${SALT_ROLE?}/update/allaz.yaml \
        --ticket-id ${TICKET?} \
        --filter %${BASE_ROLE?}
    ```

    <[
    TODO copy the command output here, starting from the Running step finalizers section
    ]>

    ### unset [downtimes](https://juggler.yandex-team.ru/downtimes/)

    После всех проверок снимаем downtimes, если они ещё активны.


## pre-prod phase: pre-prod

#### Шаблон для шага в релизном тикете

Копируем шаблон ниже в новый комментарий в релизный тикет.

Начинаем выполнять последовательно и после каждой операции фиксируем результат в этом же комментарии через его редактирование.

    ## pre-prod phase: pre-prod

    ### Monitoring

    Следим за,
    [сервисами](https://grafana.yandex-team.ru/d/iam-duty-deploy/iam-duty-deploy?orgId=1&from=now-3h&to=now&refresh=1m&var-appFilter=*_server&var-ds=Solomon%20Cloud%20Preprod&var-cluster=preprod&var-host=All),
    за выкатываемыми нодами
    [Indentity](https://grafana.yandex-team.ru/d/iam-identity_dd-identity-by-host/identity-by-hosts?orgId=1&from=now-3h&to=now&refresh=1m&var-ds=Solomon%20Cloud%20Preprod&var-cluster=preprod&var-host=All&var-method=All&var-path=All).

    ### set [downtime](https://juggler.yandex-team.ru/downtimes/) on:

    ```
    host=yc_iam_svm_preprod
    ```

    ### Миграции

    Не забыть применить миграции, если они есть. См. "Подготовка к релизу" в инструкции.

    ### Параметры для yc-bootstrap

    Убеждаемся, что работаем во вкладке с переменными окружения `TICKET` и `SALT_VERSION`.

    ### move release iam version

    ```bash
    TARGET_ENV="pre-prod"
    SALT_ROLE="iam"
    BASE_ROLE="iam"
    yc-bootstrap --apply \
        --template workflows/${TARGET_ENV?}/${SALT_ROLE?}/move-base-release/from-testing.yaml \
        --ticket-id ${TICKET?}
    ```

    <[
    TODO copy the command output here
    ]>

    ### release common to myt1

    ```bash
    TARGET_ENV="pre-prod"
    SALT_ROLE="common"
    BASE_ROLE="iam"
    yc-bootstrap --apply \
        --template workflows/${TARGET_ENV?}/${SALT_ROLE?}/update/${BASE_ROLE?}.yaml \
        --ticket-id ${TICKET?} \
        --filter "%${BASE_ROLE?} iam-myt1.svc.cloud-preprod.yandex.net"
    ```

    <[
    TODO copy the command output here, starting from the Running step finalizers section
    ]>

    ### release iam to myt1

    ```bash
    TARGET_ENV="pre-prod"
    SALT_ROLE="iam"
    BASE_ROLE="iam"
    yc-bootstrap --apply \
        --template workflows/${TARGET_ENV?}/${SALT_ROLE?}/update/allaz.yaml \
        --ticket-id ${TICKET?} \
        --filter "%${BASE_ROLE?} iam-myt1.svc.cloud-preprod.yandex.net"
    ```

    <[
    TODO copy the command output here, starting from the Running step finalizers section
    ]>

    ### check myt1 Identity

    Проверить обновлённый хост Identity.
    См. мониторинги и проверку логов.

    ### release common to all but myt1

    ```bash
    TARGET_ENV="pre-prod"
    SALT_ROLE="common"
    BASE_ROLE="iam"
    yc-bootstrap --apply \
        --template workflows/${TARGET_ENV?}/${SALT_ROLE?}/update/${BASE_ROLE?}.yaml \
        --ticket-id ${TICKET?} \
        --filter "%${BASE_ROLE?} -iam-myt1.svc.cloud-preprod.yandex.net"
    ```

    <[
    TODO copy the command output here, starting from the Running step finalizers section
    ]>

    ### release iam to all but myt1

    ```bash
    TARGET_ENV="pre-prod"
    SALT_ROLE="iam"
    BASE_ROLE="iam"
    yc-bootstrap --apply \
        --template workflows/${TARGET_ENV?}/${SALT_ROLE?}/update/allaz.yaml \
        --ticket-id ${TICKET?} \
        --filter "%${BASE_ROLE?} -iam-myt1.svc.cloud-preprod.yandex.net"
    ```

    <[
    TODO copy the command output here, starting from the Running step finalizers section
    ]>

    ### unset [downtimes](https://juggler.yandex-team.ru/downtimes/)

    После всех проверок снимаем downtimes, если они ещё активны.


## prod phase: internal-prod

#### Шаблон для шага в релизном тикете

Копируем шаблон ниже в новый комментарий в релизный тикет.
Начинаем выполнять последовательно и после каждой операции фиксируем результат в этом же комментарии через его редактирование.

    ## prod phase: internal-prod part 1 (myt1)

    ### Monitoring

    Следим за
    [сервисами](https://grafana.yandex-team.ru/d/iam-duty-deploy/iam-duty-deploy?orgId=1&from=now-3h&to=now&refresh=1m&var-cluster=prod-internal),
    за выкатываемыми нодами
    [Indentity](https://grafana.yandex-team.ru/d/iam-identity_dd-identity-by-host/identity-by-hosts?orgId=1&from=now-3h&to=now&refresh=1m&var-ds=Solomon%20Cloud&var-cluster=prod-internal&var-host=All&var-method=All&var-path=All).

    ### set [downtime](https://juggler.yandex-team.ru/downtimes/) on myt1:

    ```
    host=iam-ya-myt1.svc.cloud.yandex.net
    ```

    ### Миграции

    Не забыть применить миграции, если они есть. См. "Подготовка к релизу" в инструкции.

    ### Параметры для yc-bootstrap

    Убеждаемся, что работаем во вкладке с переменными окружения `TICKET` и `SALT_VERSION`.

    ### move release versions

    Через Pull Request меняем версию роли iam-ya и при необходимости common в [prod/base-role-releases/iam-ya.yaml](https://bb.yandex-team.ru/projects/CLOUD/repos/cluster-configs/browse/prod/base-role-releases/iam-ya.yaml)

    > TODO_LINK_YOUR_PR: [PR](TODO_LINK_YOUR_PR)

    ### release common to myt1

    ```bash
    TARGET_ENV="prod"
    SALT_ROLE="common"
    BASE_ROLE="iam-ya"
    yc-bootstrap --apply \
        --template workflows/${TARGET_ENV?}/${SALT_ROLE?}/update/${BASE_ROLE?}.yaml \
        --ticket-id ${TICKET?} \
        --filter "%${BASE_ROLE?} iam-ya-myt1.svc.cloud.yandex.net"
    ```

    <[
    TODO copy the command output here, starting from the Running step finalizers section
    ]>

    ### release iam to myt1

    ```bash
    TARGET_ENV="prod"
    SALT_ROLE="iam-ya"
    BASE_ROLE="iam-ya"
    yc-bootstrap --apply \
        --template workflows/${TARGET_ENV?}/${SALT_ROLE?}/update/allaz.yaml \
        --ticket-id ${TICKET?} \
        --filter "%${BASE_ROLE?} iam-ya-myt1.svc.cloud.yandex.net"
    ```

    <[
    TODO copy the command output here, starting from the Running step finalizers section
    ]>

    ### check myt1 Identity

    Проверить обновлённый хост Identity.
    См. мониторинги и проверку логов.

    ### unset [downtimes](https://juggler.yandex-team.ru/downtimes/)

    После всех проверок снимаем downtimes, если они ещё активны.


**Важно**!
К следующему шагу переходим через полдня.

Копируем шаблон ниже в новый комментарий в релизный тикет.
Начинаем выполнять последовательно и после каждой операции фиксируем результат в этом же комментарии через его редактирование.


    ## prod phase: internal-prod part 2 (all except myt1)

    ### re-check myt1 Indentity

    Ещё раз проверяем, что нет ошибок мониторингах.

    ### set [downtime](https://juggler.yandex-team.ru/downtimes/) on all:

    ```
    host=yc_iam_svm_internal-prod
    ```

    ### release common to all but myt1

    ```bash
    TARGET_ENV="prod"
    SALT_ROLE="common"
    BASE_ROLE="iam-ya"
    yc-bootstrap --apply \
        --template workflows/${TARGET_ENV?}/${SALT_ROLE?}/update/${BASE_ROLE?}.yaml \
        --ticket-id ${TICKET?} \
        --filter "%${BASE_ROLE?} -iam-ya-myt1.svc.cloud.yandex.net"
    ```

    <[
    TODO copy the command output here, starting from the Running step finalizers section
    ]>

    ### release iam to all but myt1

    ```bash
    TARGET_ENV="prod"
    SALT_ROLE="iam-ya"
    BASE_ROLE="iam-ya"
    yc-bootstrap --apply \
        --template workflows/${TARGET_ENV?}/${SALT_ROLE?}/update/allaz.yaml \
        --ticket-id ${TICKET?} \
        --filter "%${BASE_ROLE?} -iam-ya-myt1.svc.cloud.yandex.net"
    ```

    <[
    TODO copy the command output here, starting from the Running step finalizers section
    ]>

    ### unset [downtimes](https://juggler.yandex-team.ru/downtimes/)

    После всех проверок снимаем downtimes, если они ещё активны.


## prod phase: prod

### prod part 1

#### Шаблон для шага в релизном тикете

Копируем шаблон ниже в новый комментарий в релизный тикет.
Начинаем выполнять последовательно и после каждой операции фиксируем результат в этом же комментарии через его редактирование.

    ## prod phase: prod part 1 (myt1)

    ### Instructions
    **ВАЖНО!** Смотри [регламент подневной выкатки](./index.md):
    Начинаем через день после internal prod.

    ### Monitoring

    Следим за
    [сервисами](https://grafana.yandex-team.ru/d/iam-duty-deploy/iam-duty-deploy?orgId=1&from=now-3h&to=now&refresh=1m&var-appFilter=*_server&var-ds=Solomon%20Cloud&var-cluster=prod&var-host=All),
    за выкатываемыми нодами
    [Indentity](https://grafana.yandex-team.ru/d/iam-identity_dd-identity-by-host/identity-by-hosts?orgId=1&from=now-3h&to=now&refresh=1m&var-ds=Solomon%20Cloud&var-cluster=prod&var-host=All&var-method=All&var-path=All).

    ### set [downtime](https://juggler.yandex-team.ru/downtimes/) on:

    ```
    host=iam-myt1.svc.cloud.yandex.net
    ```

    ### Миграции

    Не забыть применить миграции, если они есть. См. "Подготовка к релизу" в инструкции.

    ### Параметры для yc-bootstrap

    Убеждаемся, что работаем во вкладке с переменными окружения `TICKET` и `SALT_VERSION`.

    ### move release iam version

    ```bash
    TARGET_ENV="prod"
    SALT_ROLE="iam"
    BASE_ROLE="iam"
    yc-bootstrap --apply \
        --template workflows/${TARGET_ENV?}/${SALT_ROLE?}/move-base-release/from-pre-prod.yaml \
        --ticket-id ${TICKET?}
    ```

    <[
    TODO copy the command output here
    ]>

    ### release common to myt1

    ```bash
    TARGET_ENV="prod"
    SALT_ROLE="common"
    BASE_ROLE="iam"
    yc-bootstrap --apply \
        --template workflows/${TARGET_ENV?}/${SALT_ROLE?}/update/${BASE_ROLE?}.yaml \
        --ticket-id ${TICKET?} \
        --filter %${BASE_ROLE?} host=iam-myt1.svc.cloud.yandex.net
    ```

    <[
    TODO copy the command output here, starting from the Running step finalizers section
    ]>

    ### release iam to myt1

    ```bash
    TARGET_ENV="prod"
    SALT_ROLE="iam"
    BASE_ROLE="iam"
    yc-bootstrap --apply \
        --template workflows/${TARGET_ENV?}/${SALT_ROLE?}/update/allaz.yaml \
        --ticket-id ${TICKET?} \
        --filter %${BASE_ROLE?} host=iam-myt1.svc.cloud.yandex.net
    ```

    <[
    TODO copy the command output here, starting from the Running step finalizers section
    ]>

    ### check myt1 Identity

    Проверить обновлённый хост Identity.
    См. мониторинги и проверку логов.

    ### unset [downtimes](https://juggler.yandex-team.ru/downtimes/)

    После всех проверок снимаем downtimes, если они ещё активны.


### israel part 1

#### Шаблон для шага в релизном тикете

Копируем шаблон ниже в новый комментарий в релизный тикет.
Начинаем выполнять последовательно и после каждой операции фиксируем результат в этом же комментарии через его редактирование.

    ## prod phase: israel part 1 (iam-il1-a1)

    ### Instructions
    **ВАЖНО!** Смотри [регламент подневной выкатки](./index.md):
    Начинаем через день после internal prod (после myt1 на prod).

    ### Monitoring

    Следим за
    [сервисами](https://grafana.yandex-team.ru/d/iam-duty-deploy/iam-duty-deploy?orgId=1&from=now-3h&to=now&refresh=1m&var-appFilter=*_server&var-ds=Solomon%20Cloud%20IL&var-cluster=israel&var-service=All&var-host=All),
    за выкатываемыми нодами
    [Indentity](https://grafana.yandex-team.ru/d/iam-identity_dd-identity-by-host/identity-by-hosts?orgId=1&from=now-3h&to=now&refresh=1m&var-ds=Solomon%20Cloud%20IL&var-cluster=israel&var-host=All&var-method=All&var-path=All).

    ### set [downtime](https://juggler.yandex-team.ru/downtimes/) on:

    ```
    host=iam-il1-a1.svc.yandexcloud.co.il
    ```

    ### Миграции

    Не забыть применить миграции, если они есть. См. "Подготовка к релизу" в инструкции.

    ### Параметры для yc-bootstrap

    Убеждаемся, что работаем во вкладке с переменными окружения `TICKET` и `SALT_VERSION`.

    ### move release iam version

    ```bash
    TARGET_ENV="israel"
    SALT_ROLE="iam"
    BASE_ROLE="iam"
    yc-bootstrap --apply \
        --template workflows/${TARGET_ENV?}/${SALT_ROLE?}/move-base-release/from-prod.yaml \
        --ticket-id ${TICKET?}
    ```

    <[
    TODO copy the command output here
    ]>

    ### release common to iam-il1-a1

    ```bash
    TARGET_ENV="israel"
    SALT_ROLE="common"
    BASE_ROLE="iam"
    yc-bootstrap --apply \
        --template workflows/${TARGET_ENV?}/${SALT_ROLE?}/update/${BASE_ROLE?}.yaml \
        --ticket-id ${TICKET?} \
        --filter %${BASE_ROLE?} host=iam-il1-a1.svc.yandexcloud.co.il
    ```

    <[
    TODO copy the command output here, starting from the Running step finalizers section
    ]>

    ### release iam to iam-il1-a1

    ```bash
    TARGET_ENV="israel"
    SALT_ROLE="iam"
    BASE_ROLE="iam"
    yc-bootstrap --apply \
        --template workflows/${TARGET_ENV?}/${SALT_ROLE?}/update/allaz.yaml \
        --ticket-id ${TICKET?} \
        --filter %${BASE_ROLE?} host=iam-il1-a1.svc.yandexcloud.co.il
    ```

    <[
    TODO copy the command output here, starting from the Running step finalizers section
    ]>

    ### check iam-il1-a1 Identity

    Проверить обновлённый хост Identity.
    См. мониторинги и проверку логов.

    ### unset [downtimes](https://juggler.yandex-team.ru/downtimes/)

    После всех проверок снимаем downtimes, если они ещё активны.

### prod part 2

**ВAЖНО!**
К этому шагу переходим **через день** после предыдущего.

Копируем шаблон ниже в новый комментарий в релизный тикет.
Начинаем выполнять последовательно и после каждой операции фиксируем результат в этом же комментарии через его редактирование.


    ## prod phase: prod part 2 (myt except myt1)

    ### re-check myt1 Identity

    Снова проверяем, что нет ошибок на графиках.

    ### set [downtime](https://juggler.yandex-team.ru/downtimes/) on:

    ```
    host=yc_iam_svm_prod
    ```

    ### release common to myt all but myt1

    ```bash
    TARGET_ENV="prod"
    SALT_ROLE="common"
    BASE_ROLE="iam"
    yc-bootstrap --apply \
        --template workflows/${TARGET_ENV?}/${SALT_ROLE?}/update/${BASE_ROLE?}.yaml \
        --ticket-id ${TICKET?} \
        --filter "%${BASE_ROLE?} @ru-central1-c -iam-myt1.svc.cloud.yandex.net"
    ```

    <[
    TODO copy the command output here, starting from the Running step finalizers section
    ]>

    ### release iam to myt all but myt1

    ```bash
    TARGET_ENV="prod"
    SALT_ROLE="iam"
    BASE_ROLE="iam"
    yc-bootstrap --apply \
        --template workflows/${TARGET_ENV?}/${SALT_ROLE?}/update/allaz.yaml \
        --ticket-id ${TICKET?} \
        --filter "%${BASE_ROLE?} @ru-central1-c -iam-myt1.svc.cloud.yandex.net"
    ```

    <[
    TODO copy the command output here, starting from the Running step finalizers section
    ]>

    ### unset [downtimes](https://juggler.yandex-team.ru/downtimes/)

    После всех проверок снимаем downtimes, если они ещё активны.


### israel part 2

К этому шагу переходим по завершении prod part 2.

Копируем шаблон ниже в новый комментарий в релизный тикет.
Начинаем выполнять последовательно и после каждой операции фиксируем результат в этом же комментарии через его редактирование.


    ## prod phase: israel part 2 (all but iam-il1-a1)

    ### re-check iam-il1-a1 Identity

    Снова проверяем, что нет ошибок на графиках.

    ### set [downtime](https://juggler.yandex-team.ru/downtimes/) on:

    ```
    host=yc_iam_svm_israel
    ```

    ### release common to myt all but iam-il1-a1myt1

    ```bash
    TARGET_ENV="israel"
    SALT_ROLE="common"
    BASE_ROLE="iam"
    yc-bootstrap --apply \
        --template workflows/${TARGET_ENV?}/${SALT_ROLE?}/update/${BASE_ROLE?}.yaml \
        --ticket-id ${TICKET?} \
        --filter "%${BASE_ROLE?} -iam-il1-a1.svc.yandexcloud.co.il"
    ```

    <[
    TODO copy the command output here, starting from the Running step finalizers section
    ]>

    ### release iam to myt all but iam-il1-a1

    ```bash
    TARGET_ENV="israel"
    SALT_ROLE="iam"
    BASE_ROLE="iam"
    yc-bootstrap --apply \
        --template workflows/${TARGET_ENV?}/${SALT_ROLE?}/update/allaz.yaml \
        --ticket-id ${TICKET?} \
        --filter "%${BASE_ROLE?} -iam-il1-a1.svc.yandexcloud.co.il"
    ```

    <[
    TODO copy the command output here, starting from the Running step finalizers section
    ]>

    ### unset [downtimes](https://juggler.yandex-team.ru/downtimes/)

    После всех проверок снимаем downtimes, если они ещё активны.

### prod part 3

**ВAЖНО!**
К этому шагу переходим **через день** после предыдущего.

Копируем шаблон ниже в новый комментарий в релизный тикет.
Начинаем выполнять последовательно и после каждой операции фиксируем результат в этом же комментарии через его редактирование.


    ## prod phase: prod part 3 (vla & sas)

    ### re-check myt & israel Identity

    Снова проверяем, что нет ошибок на графиках.

    ### set [downtime](https://juggler.yandex-team.ru/downtimes/) on all:

    ```
    host=yc_iam_svm_prod
    ```

    ### release common to vla all

    ```bash
    TARGET_ENV="prod"
    SALT_ROLE="common"
    BASE_ROLE="iam"
    yc-bootstrap --apply \
        --template workflows/${TARGET_ENV?}/${SALT_ROLE?}/update/${BASE_ROLE?}.yaml \
        --ticket-id ${TICKET?} \
        --filter %${BASE_ROLE?} @ru-central1-a
    ```

    <[
    TODO copy the command output here, starting from the Running step finalizers section
    ]>

    ### release iam to vla all

    ```bash
    TARGET_ENV="prod"
    SALT_ROLE="iam"
    BASE_ROLE="iam"
    yc-bootstrap --apply \
        --template workflows/${TARGET_ENV?}/${SALT_ROLE?}/update/allaz.yaml \
        --ticket-id ${TICKET?} \
        --filter %${BASE_ROLE?} @ru-central1-a
    ```

    <[
    TODO copy the command output here, starting from the Running step finalizers section
    ]>

    ### release common to sas all

    ```bash
    TARGET_ENV="prod"
    SALT_ROLE="common"
    BASE_ROLE="iam"
    yc-bootstrap --apply \
        --template workflows/${TARGET_ENV?}/${SALT_ROLE?}/update/${BASE_ROLE?}.yaml \
        --ticket-id ${TICKET?} \
        --filter %${BASE_ROLE?} @ru-central1-b
    ```

    <[
    TODO copy the command output here, starting from the Running step finalizers section
    ]>

    ### release iam to sas all

    ```bash
    TARGET_ENV="prod"
    SALT_ROLE="iam"
    BASE_ROLE="iam"
    yc-bootstrap --apply \
        --template workflows/${TARGET_ENV?}/${SALT_ROLE?}/update/allaz.yaml \
        --ticket-id ${TICKET?} \
        --filter %${BASE_ROLE?} @ru-central1-b
    ```

    <[
    TODO copy the command output here, starting from the Running step finalizers section
    ]>

    Проверить обновлённые хосты Identity.
    См. мониторинги во vla и sas.

    ### unset [downtimes](https://juggler.yandex-team.ru/downtimes/)

    После всех проверок снимаем downtimes, если они ещё активны.


