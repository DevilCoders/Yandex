## identity-role-access-matrix

Этот документ описывает формат хранения данных о well-known сущностях IAM'а.

Пермишены и сервисные роли хранятся в отдельном каталоге для сервиса, который ими управляет. 
Например, пермишен `compute.instances.start` может быть задан в файле `compute/permissions.yaml`, а роль `resource-manager.clouds.member` — в файле `resource-manager/roles.yaml`.
Внутри своего каталога команда сервиса может организовать данные как угодно, одним файлом или несколькими, положить их в одном каталоге или раскидать по поддиректориям. 
Главное - соблюдать следующее соглашение об именах файлов.
* `permissions.yaml`
* `roles.yaml`
* `resources.yaml`
* `stages.yaml`
* `scopes.yaml`
* `clients.yaml`
* `restrictions.yaml`

В каждом файле можно сослаться на сущность из любого другого файла — точно так же, как если бы сущности были описаны рядом, явно ссылаться на файл не нужно.

Теперь подробнее о каждом типе имени файла.

### permissions.yaml - права

```yaml
permissions:
  iam.accessBinding.delete:  # имя пермишена
    # Описание роли на английском для документации. Сейчас никак не используется и, вероятно, будет удалено.
    description: Delete access binding.
    
    # Стейдж. Чаще всего это GA.
    # Список стейджей лежит в `stages.yaml`.
    stage: GA
    
    # Видимость пермишена: может быть public или internal.
    # Связана с видимостью ролей: internal пермишены не должны входить в public роли.
    visibility: public
    
    # Здесь задаются условия, когда пермишен может действовать.
    allowedWhen:
      cloud: # Указание разрешённых состояний облака, это старый, но на данный момент основной механизм.
             # Он будет полностью заменён новым механизмом блокировок/ограничений.
             # Пермишн действует, если облако в статусе ACTIVE или BLOCKED_BY_BILLING. 
             # В другом статусе — например, BLOCKED — пермишен запрещён.
        status: [BLOCKED_BY_BILLING, ACTIVE]
      restriction: 
        - billSuspend # Действует, даже при наличии ограничения blockPermissions.billSuspend
    deniedWhen:
      restrictions:
        - sanctions # Не действует, при наличии ограничения blockPermissions.sanctions
```

### roles.yaml - роли

```yaml
roles:
   example.editor:  # название роли
      name: Example Editor # Текстовое описание роли. Сейчас не используется и служит скорее в качестве комментария.
     
     # Видимость роли: может быть public или internal. Роли public видят пользователи, а internal роли — нет.
     # Public роль не должна включать в себя internal-пермишены, сейчас это warning при компиляции, 
     # который не даст вмержить pull request.
     visibility: public
     
     # Минимальный тип ресурса, на который можно назначить роль.
     resourceType: resource-manager.folder
     # Эту роль можно назначить на фолдер или на клауд, но нельзя на SA или на биллинг-аккаунт.
     # Такая роль может содержать пермишены, у которых resourceType фолдер или какой-нибудь вложенный в него ресурс,
     # но не может содержать никакие другие пермишены.
       
     # Пермишены, входящие в роль.
     # Параметр можно не указывать, если роль не включает никаких пермишенов напрямую.
     permissions:
       - example.things.edit
       - example.things.manage
       # Есть сокращённая форма записи:
       - example.item.{create,update,delete}
       # Фигурные скобки можно использовать в любом месте записи:
       # `sample.{horses,mice,chickens}.{feed,pet}` тоже можно сказать,
       # эта запись разресолвится в 6 пермишенов.
       # (Но лучше таким не злоупотреблять.)
     
     # Другие роли, входящие в состав этой.
     # Параметр можно не указывать, если не нужно инклюдить никакие другие роли.
     includedRoles:
       - example.viewer
       - horse.whisperer
     # Роль `example.editor` содержит все пермишены из `example.viewer` и `horse.whisperer`, с совпадающим resourceType.
     # `includedRoles` работает транзитивно: если в определении `example.viewer` тоже инклюдятся какие-то роли,
     # то их пермишены входят и в `example.editor`.
       
     # OAuth скоупы также могут быть использованы для включения пермишнов.
     scopes:
       - yc.example.read
       - yc.example.write
     
     # В итоге получается, что в роль `example.editor` входят пермишены:
     # * `example.things.edit`, `example.things.manage`,
     # * `example.item.create`, `example.item.update`, `example.item.delete`,
     # * все пермишены, которые входят в роли `example.viewer` и `horse.whisperer`,
     # * все пермишены, которые входят в скоупы `yc.example.read`, `yc.example.write`. 
     
   # Ещё одна роль.
   example.viewer:
     # Эта роль используется в роли `example.editor` выше.
     # Но это не значит, что `example.viewer` в файле должна идти после `example.editor` —
     # можно расположить их хоть как или вообще положить в разные файлы.
     ...
```

Роли задаются аддитивно: можно создать роль "`viewer` плюс `compute.editor` плюс `iam.serviceAccounts.create`", но нельзя задать "`viewer` минус `billing.viewer`" или "все пермишены `serverless.*.*`, кроме `serverless.*.delete`". Это сделано специально, чтобы при добавлении новой роли/пермишена вся система вела себя более предсказуемо.

Некоторые роли помечены как "псевдороли", у них есть поле `pseudorole: true`. Они существуют только на этапе компиляции и нужны для наследования в "настоящих" ролях.
Сейчас из них, как правило, составляются примитивные роли типа `viewer`, `editor`.

### resource.yaml

```yaml
resources:
  resource-manager.cloud: # название ресурса
    parents: [root, organization-manager.organization]
    # Пермишн, который проверяется при просмотре биндингов на ресурсе этого типа в консоли.
    accessBindingsListingPermission: resource-manager.clouds.listAccessBindings
```
Файл содержит типы ресурсов, на которые можно выдавать роли субъектам.
Названия ресурсов используются при валидации названий прав и ролей.
В поле parents указывается, в какие ресурсы-контейнеры может входить текущий ресурс. 
Это влияет на возможность включения пермишна в роль - ресурс пермишна должен совпадать с ресурсом роли или его "родителем".

### stages.yaml - стейджи, они же альфа-флаги

Флаги описаны в виде списка строчных идентификаторов, на которые могут ссылаться пермишны.

Если для пермишна указан флаг отличный от GA, то при авторизации будет дополнительная проверка,
что этот флаг включён на облаке, в котором находится ресурс.

```yaml
stages:
  - GA
  - TEST_ALPHA
  - RELEASE_CANDIDATE
```
### scopes.yaml

```yaml
scopes:
  openid:
    service: oauth
    name: ''
    description: 'аутентифицировать пользователя и получить его sub (subjectId)'
    visibility: public
    permissions: []
  yc.iam.clouds.manage:
    service: iam
    name: 'удалять облако и редактировать его настройки'
    description: ''
    visibility: internal
    permissions:
      - iam.clouds.delete
      - iam.clouds.setUserListingSetting
```

### clients.yaml

```yaml
  yc.oauth.console:
    name: 'Yandex.Cloud Console'
    # Как и с ролями, есть три способа включения пермишнов в OAuth клиента: 
    # напрямую, через роль и через скоуп.
    permissions:
      - certificate-manager.providers.use
    includedRoles:
      - resource-manager.clouds.owner
    scopes:
      - yc.organization-manager.federations.manage
```

#### Настройки OAuth клиента в env файлах

Так как настройки oauth клиентов отличаются на разных стендах, то они определяются в env файлах (подробнее о них ниже)

```yaml
  yc.oauth.console:
    authorized_grant_types: [ authorization_code ]
    client_scopes: [ openid ]
    auto_approve_scopes: [ openid ]
    # sha256 пароля клиента в hex представлении 
    client_secret_sha256: f5f8ca2847207eb5d40fe099eee2b16c0834644c66e4668d2b6422f10e16f28e
    redirect_uris: # список допустимых значений для callback uri
      - https://console.cloud.yandex.ru/auth/callback
      - https://console.cloud.yandex.com/auth/callback
```

### restrictions.yaml - типы блокировок

Блокировками можно дополнительно ограничивать авторизацию на ресурсах.

```yaml
restrictions:
  blockPermissions:
    billSuspend:
      servicesToStop: ["*"] # Остановить все сервисы после того, как эта блокировка добавлена на ресурс.
      resourcesToStop: []
      stopDelay: PT0S # Задержка между добавлением блокировки и остановкой сервисов
      deletionInitiationInterval: P57D
      deletionDelay: P3D
      denyAllPermissionsByDefault: true # Позволять действие, только если в секции пермишна `allowedWhen` явна указан этот тип блокировок.
    elasticsearchSanctions:
      servicesToStop: ["managed-elasticsearch"]
      stopDelay: P7D
      denyAllPermissionsByDefault: false # Запрещать действие, только если в секции пермишна `deniedWhen` явно указан этот тип блокировок. 
```

## Env files

Некоторые метаданные IAM отличаются на разных стендах. Например, публичность (видимость) ролей или настройки OAuth клиентов.

Подробнее - в [yandex/cloud/prv/env/README.md](https://bb.yandex-team.ru/projects/CLOUD/repos/cloud-go/browse/private-api/yandex/cloud/priv/env/README.md)

## Tooling

Базовые инструменты для работы с метамоделью IAM доступны в [ycp](https://bb.yandex-team.ru/projects/CLOUD/repos/cloud-go/browse/api/ycpcli/pkg/commands/custom/iamcustom/iammeta).

Для проверки того, что yaml'ы написаны верно, есть CI конфигурация [IAM_Metadata_ValidateIamMetadata](https://teamcity.aw.cloud.yandex.net/viewType.html?buildTypeId=IAM_Metadata_ValidateIamMetadata).
Она запускается на каждый PR и пишет в него комментарий с проблемами, которые надо исправить, и с планом изменений, которые влечёт за собой данный PR.

## Naming conventions

* Resources - `<service>.<resource>`
* Public permissions - `<service>.<resources>.<action>`
* Public roles - `<service>.<resource>.<role>`
* Internal OAuth scopes - `yc.<service>.<resources>.<action>`
* OAuth Clients - `yc.oauth.<app>`
