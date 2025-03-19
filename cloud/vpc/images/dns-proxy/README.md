Здесь содержится packer-конфигурация для образа dns-proxy. Dns-proxy используется в аппендиксе приватного облака,
чтобы проксировать DNS-запросы из оверлея и андерлея приватного облака в ns-cache.yandex.net
(и dns-cache.yandex.net в качестве резервного форвадрера), так как из приватного облака сетевого доступа напрямую до этих
DNS-серверов нет, а до аппендикса — есть.

В образ устанавливается DNS-сервер BIND с простой конфигурацией (см. `salt_bind/files/named.conf.options`). 
Он проксирует все запросы к DNS-серверам Яндекса.

## Сборка образа

Образ собирается на основе базового образа из [../base/](`../base`). Общие правила сборки таких образов описаны в [../README.md](`../README.md`).

Для сборки конкретно этого образа запустите следующие команды:

```
export FOLDER_ID="b1gn8lr1h94mi3lfq2qa"
export SUBNET_ID="e9bigio0vhk246euavkb"                  # Подсеть для создания инстанса. С доступом к IPv6-сети Яндекса и к облачному дисту.
export EXPORT_SUBNET_ID="e9bhamlvi44q4bpk8adv"           # Подсеть для экспорта образа в S3. Должна находиться в том же фолдере и в ней должен быть IPv4-доступ в интернет
export EXPORT_SERVICE_ACCOUNT_ID="aje0jeuv14jjbeqe4c4e"  # Сервисный аккаунт, от имени которого будет создана виртуалка для экспорта. Должен иметь роль editor на фолдер.
export EXPORT_STORAGE_PATH="s3://yc-vpc-packer-export/"  # Папка, в которую будет загружен образ в Yandex Object Storage
export ZONE_ID="ru-central1-a"
export YC_TOKEN=$(yc --profile=prod iam create-token)
export COMMIT_AUTHOR="andgein"

packer build ./packer.json
```


(Особенность: чтобы packer мог сходить на создаваемый инстанс, доступ на который есть только через `pssh`, надо либо настроить `pssh-agent`, 
либо запускать сборку образа на «сборочном хосте» (`pssh -A deploy.cloud.yandex.net`), оттуда есть доступ напрямую.)

Для запуска экспорта полученного образа в S3 нужен packer версии 1.6.0 или выше.


## Создание инстансов

После того, как инстанс будет загружен в Yandex Object Storage, стоит взять ссылку на него (типа https://storage.yandexcloud.net/yc-vpc-packer-export/dns-proxy.qcow2) и 
положить её в терраформ-рецепт из `bootstrap-templates/terraform/{testing,prod}/vpc.tf`. Этот рецепт создаст два (а в проде все три) dns-proxy из этого образа в нужном аппендиксе.
