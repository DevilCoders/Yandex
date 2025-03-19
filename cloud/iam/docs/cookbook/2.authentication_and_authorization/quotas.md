В каждом сервисе, поддерживающем квоты, должно быть реализовано [API квот](https://bb.yandex-team.ru/projects/CLOUD/repos/cloud-go/browse/private-api/yandex/cloud/priv/quota/quota.proto).

Названия квот должны быть указаны в [репозитории](https://bb.yandex-team.ru/projects/CLOUD/repos/cloud-go/browse/private-api/yandex/cloud/priv/)
в файлах `quotas.yaml` в каталоге сервиса рядом с его прото-спеками.

{% note info %}

Сейчас требование указания квот в файлах quotas.yaml чисто формальное, гарантировать, что сервисы действительно отдают
только квоты, задекларированные в этих файлах, нельзя. Однако после появления [генеренных с метаданных файлов с константами](https://st.yandex-team.ru/CLOUD-44401),
не нужно будет указывать названия квот в двух местах (в коде и в yaml-файлах).

{% endnote %}

Формат файла:
```yaml
quotas:
  - resource-manager.folders.count
  - iam.resourceAccessBindings.count
  ...
```

Квоты именуются по схеме `<service>.<quotaType>.<metricType>`. Примеры:

- compute.instances.count
- compute.cores.count
- compute.gpus.count
- compute.operations.rate
- serverless.functions.count
- resource-manager.folders.count
- nbs.hdd.size
- iam.groupNesting.count

`<service>` можно взять из файлов описаний сервисов в [прото-спеках](https://bb.yandex-team.ru/projects/CLOUD/repos/cloud-go/browse/private-api/yandex/cloud/priv/services.yaml).

```yaml
services:
  resource-manager:
    name: Yandex Resource Manager
    description: Управление ресурсами в каталогах и облаках
  storage:
    name: Yandex Object Storage
    description: Масштабируемое хранилище данных
    aliases: [s3]
  ...
```

`<quotaType>` — определяется спецификой сервиса. На текущий момент должна удовлетворять шаблону `[a-zA-Z0-9-]+`.

`<metricType>` — одно из следующих значений:

- count — количество в штуках
- size — размер (по-умолчанию можно трактовать как размер в байтах)
- rate — запросов в единицу времени (по-умолчанию можно трактовать как «в секунду»)

{% note info %}

Формат названия квот автоматически валидируется (проверяется название сервиса и тип метрики). PR'ы с изменениями файлов
quotas.yaml проходят обязательное ревью сотрудников подразделения [IAM & Resource Management](https://staff.yandex-team.ru/departments/yandex_exp_9053_dep17654/).

{% endnote %}
