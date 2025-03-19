Здесь лежат packer-конфигурации для образов, используемых в команде VPC. Список образов:

- `base` — базовый образ, собранный из Ubuntu 20.04 LTS. Поддерживает два сетевых интерфейса (IPv4 + IPv6 и IPv6), идёт с установленными ssh, cauth, hbf и osquery. За софтом ходит в облачный и яндексовый репозитории.
- `dns-proxy` — образ для DNS-прокси в аппендиксе приватного облака
- `accounting` — образ для accounting сервиса виртуальной сети
- `mr-prober/base` — базовый образ для виртуальных машин в проекте активного мониторинга сетевого датаплейна [Mr. Prober](https://wiki.yandex-team.ru/cloud/devel/sdn/mrprober/). Построен на основе образа `base`.
- `mr-prober/web-server` — отличается от `mr_prober/base` установленным веб-сервером nginx.
- `mr-prober/router` — тестовый образ для ВМ-маршрутизаторов, не имеет кастомизаций Mr.Prober, в частности, докера
- `mr-prober/agent` — агент для Mr. Prober
- `mr-prober/api` — Mr. Prober API
- `mr-prober/creator` — Mr. Prober Creator
- `monops` - образ для Monops Web
- `e2e/nlb-target` - образ для e2e тестов, используется в качестве реала балансера. [Про e2e тесты vpc-api](https://wiki.yandex-team.ru/cloud/devel/sdn/cfg/testing/e2e/).

# Сборка образов

## Полуавтоматически на deploy.cloud.yandex.net

Сгенерировать профиль и скопировать `packer.json` и директорию `salt`, запустив на своём компьютере:
```
./prepare-deploy-image.sh monops
```

Следовать инструкциям скрипта.

## Вручную

Сборка образов по умолчанию происходит в продовом Облаке в фолдере [`yc-tools`](https://console.cloud.yandex.ru/folders/b1gn8lr1h94mi3lfq2qa), так как в нём есть все необходимые сети и дырки.

Для сборки образа нужно почти всегда определить следующие переменные окружения:

```
export FOLDER_ID="b1gn8lr1h94mi3lfq2qa"                  # Идентификатор фолдера `yc-tools`.
export SUBNET_ID="e9bigio0vhk246euavkb"                  # Подсеть для создания инстанса. С доступом к IPv6-сети Яндекса и к облачному дисту.
export ZONE_ID="ru-central1-a"                           # Зона, в которой будет создан инстанс.
export YC_TOKEN=$(yc --profile=prod iam create-token)
export COMMIT_AUTHOR="andgein"
export SERVICE_ACCOUNT_ID="aje89rf6e10u9d9ucun1"         # Сервисный аккаунт, от имени которого будет создана виртуалка для сборки образа. Используется, чтобы скачать докер-образы из Container Registry, например

```

По умолчанию готовый образ окажется в указанном фолдере (в случае `yc-tools` — [здесь](https://console.cloud.yandex.ru/folders/b1gn8lr1h94mi3lfq2qa/compute?section=images)). Такой образ
уже можно использовать для создания виртуалок, однако только в продовом Облаке. Чтобы создать аналогичный образ на других стендах, он экспортируется в продовый S3 и получает свою ссылку.

Для этого в packer-конфигурациях образов, которые нужно уметь так экспортировать, есть дополнительная секция `post-processors` с процессором типа `yandex-export`. Этот процессор
создаёт ещё одну виртуальную машину в указанном фолдере, ставит туда нужный ему софт, монтирует к себе диск, созданный из экспортируемого образа, и копирует в S3 (Yandex Object Storage). Для работы
этого процессора нужно определить ещё несколько переменных:

```
export EXPORT_SUBNET_ID="e9bhamlvi44q4bpk8adv"                                   # Подсеть для экспорта образа в S3. Должна находиться в том же фолдере и в ней должен быть IPv4-доступ в интернет (Egress NAT подойдёт)
export EXPORT_SERVICE_ACCOUNT_ID="aje0jeuv14jjbeqe4c4e"                          # Сервисный аккаунт, от имени которого будет создана виртуалка для экспорта. Должен иметь роль editor на фолдер.
export EXPORT_STORAGE_PATH="s3://yc-vpc-packer-export/"                          # Папка, в которую будет загружен образ в Yandex Object Storage
```

В фолдере `yc-tools` уже создан сервисный акканут `aje0jeuv14jjbeqe4c4e` (имя `vpc-s3-uploader`), ему выданы нужные роли: https://console.cloud.yandex.ru/folders/b1gn8lr1h94mi3lfq2qa?section=service-accounts.

В том же фолдере есть S3-бакет `yc-vpc-packer-export`, в котором хранятся уже экспортированные для иморта на другие стенды образы: https://console.cloud.yandex.ru/folders/b1gn8lr1h94mi3lfq2qa/storage/bucket/yc-vpc-packer-export.

Для запуска, собственно, сборки образа, запустите packer:

```
packer build ./packer.json
```

Три важных особенности:
1. Чтобы packer мог сходить на создаваемый инстанс, доступ на который есть только через `pssh`, надо либо настроить `pssh-agent`,
либо запускать сборку образа на «сборочном хосте» (`pssh -A deploy.cloud.yandex.net`), оттуда есть доступ напрямую.
2. Для экспорта полученного образа в S3 нужен packer версии 1.6.0 или выше.
3. Начиная с packer 1.6.6 для экспорта нужен доступ по ssh на виртуальную машину, поэтому желательно использовать сборочный хост, у которого есть доступ и сеть `e9bigio0vhk246euavkb`

## Создание инстансов

После того, как инстанс будет загружен в S3, стоит взять ссылку на него (типа https://storage.yandexcloud.net/yc-vpc-packer-export/dns-proxy/2020-10-01.qcow2) и
положить её в терраформ-рецепты, которые хранятся в двух местах:
- `https://bb.yandex-team.ru/projects/CLOUD/repos/bootstrap-templates/browse/terraform/{testing,prod}/vpc.tf`
- `../terraform-configs/`
