Авторизация
===========

Токены/пароли ни в коем случае не должны попасть в VCS. Поэтому все секреты - в [секретнице](https://wiki.yandex-team.ru/passport/yav-usage/), доступ к ним - через [ya vault](https://vault-api.passport.yandex.net/docs/#cli).

Для работы с секретницей следует получить [OAuth-токен](https://vault-api.passport.yandex.net/docs/#oauth) и положить его в файл `~/.yav/token` (именно там его будут искать скрипты).

В Облаке используется федеративная авторизация, нужно настроить профили prod/preprod/testing (скрипты полагаются на эти названия) по [инструкции](https://wiki.yandex-team.ru/users/dmitryrusanov/cloud/federationsinclouds/).

Секреты
=======

Для деплоя секретов внутрь VM используется [skm](https://wiki.yandex-team.ru/cloud/devel/platform-team/infra/skm/), при помощи которого можно зашифровать секрет приватным ключем, доступ к которому будет у сервисного аккаунта виртуалки. Зашифрованный секрет передается внутрь VM через сервис метаданных и автоматически расшифровывается skm, запущенным внутри гостя.

Изначально секреты лежат в секретнице, а ссылки на них прописаны в [конфигах skm](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/filestore/deploy/secrets). При помощи скрипта [skm.sh](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/filestore/deploy/skm.sh) можно сгенерировать бандл и задеплоить его через терраформ.

Сборка docker-контейнера
========================

Запускается обычный [ya package](https://docs.yandex-team.ru/devtools/package), который собирает бинарник и готовит образ [docker-контейнера](https://docs.docker.com/) с ним.

Цель сборки указывается в [pkg.json](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/filestore/deploy/docker/yandex-cloud-filestore-server/pkg.json), инструкции докера - в соответствующем [Dockerfile](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/filestore/deploy/docker/yandex-cloud-filestore-server/Dockerfile).

Собрать контейнер локально можно воспользовавшись скриптом [build-docker.sh](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/filestore/deploy/build-docker.sh). Контейнер будет загружен в Облачный репозиторий в каталоге [yc-nfs/common](https://console.cloud.yandex.ru/folders/b1gkgl0g8k32577lbdob/container-registry/registries/crpsohroskvc81pevas8/overview).

Для авторизации рекомендуется использовать [docker credential helper](https://cloud.yandex.ru/docs/container-registry/operations/authentication#cred-helper), для этого выполните команду: `yc container registry configure-docker --profile <имя профиля>`.

Деплоить контейнер можно при помощи [k8s](https://kubernetes.io/docs/) или же собрать на его основе образ диска и заводить виртуалки через сервис [Instance Groups](https://cloud.yandex.ru/docs/compute/concepts/instance-groups/).

Сборка compute-образа
=====================

Собирается образ при помощи [packer](https://www.packer.io/docs). В Облаке запускается VM, в которой выполняется настройка окружения. Далее образ диска этой виртуалки может использоваться как шаблон для создания новых VM.

Запустить сборку локально можно воспользовавшись скриптом [build-packer.sh](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/filestore/deploy/build-packer.sh). Образ будет создан в каталоге [yc-nfs/common](https://console.cloud.yandex.ru/folders/b1gkgl0g8k32577lbdob/compute?section=images).

Сам образ содержит [много всего](https://bb.yandex-team.ru/projects/CLOUD/repos/paas-images/browse/paas-base-g4/), но самое главное - [Static Pod](https://kubernetes.io/docs/tasks/configure-pod-container/static-pod/), в котором поднят собранный ранее контейнер с нашим бинарником.

Сборочный Цех
=============

Для сборки релиза, пригодного для деплоя в PROD, следует воспользоваться [Сборочным Цехом](https://wiki.yandex-team.ru/cloud/devel/assembly-workshop/welcome). Настройки проекта NBS в репозитории СЦ лежат [тут](https://bb.yandex-team.ru/projects/CLOUD/repos/aw-teamcity-settings/browse/.teamcity/NBS).

Есть несколько типов сборок:
* [Сборка deb-пакетов](https://teamcity.aw.cloud.yandex.net/buildConfiguration/NBS_Filestore_PackageBuild)
* [Сборка docker-контейнера](https://teamcity.aw.cloud.yandex.net/buildConfiguration/NBS_Filestore_DockerBuild)
* [Сборка compute-образа](https://teamcity.aw.cloud.yandex.net/buildConfiguration/NBS_Filestore_ComputeImageBuild)

Деплой при помощи Terraform
===========================

Желаемая конфигурация сервиса задается декларативным описанием. В процессе применения этой конфигурации [Terraform](https://www.terraform.io/docs/) будет пытаться привести конфигурацию от текущего состояния к ожидаемому, создавая/модифицируя соответствующие сущности - вроде VM, Instance Groups и пр.

Состояние последнего применения конфигурации сохраняется в S3 и не привязано к локальной машине. Для инициализации локального состояния необходимо вызвать скрипт [terraform-init.sh](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/filestore/deploy/terraform-init.sh).

Для применения конфигурации нужно использовать скрипт [terraform-apply.sh](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/filestore/deploy/terraform-apply.sh). Перед этим рекомендуется ознакомиться с грядущими изменениями при помощи скрипта [terraform-plan.sh](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/filestore/deploy/terraform-plan.sh).
