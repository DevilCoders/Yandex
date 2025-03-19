Конфигурация для подготовки образа и снапшота диска для билд агентов, используемых в пуле YandexCloud.


Чтобы запустить сборку локально нужно:
0. Настроить `yc` с профилем `preprod` для получения iam токена с правами на folder `service-images`, как настроить для работы с федерацией читать [тут](https://clubs.at.yandex-team.ru/ycp/3101), либо выставить переменную `YC_TOKEN`
0. Настроить `ssh` для работы с сессионными ключами через bastion2, читать [тут](https://wiki.yandex-team.ru/cloud/yubikey/#3.opensshnerekomenduem) и [тут](https://wiki.yandex-team.ru/cloud/security/services/bastion2/openssh-client/)
0. Установить `packer`
0. Установить `arc`, читать [тут](https://doc.yandex-team.ru/arc/setup/arc/install.html), либо выставить переменную `COMMIT_REVISION` с номером, которой будет использоваться в имени образа
0. Запустить `./run-packer.py`, поддерживает передачу разных опций, см. `./run-packer.py --help`

Чтобы обновить секреты в сборке:
0. Определить переменную `YAV_TOKEN` для получения секретов из yav
0. Определить `YC_TOKEN` с правами до ключа kms, например воспользовавшись sa с build-agent'а:
  * sudo su -l robot-yc-ci
  * yc --profile preprod iam create-token
0. Поправить `skm.yaml`, добавив необходимый секрет
0. Запустить `update-secrets.sh`
0. Закомитить изменения в `files/skm-bundle.yaml`

**TODO**:
0. Таск по обновлению секретов?

Command for debug:
`PACKER_LOG=1 SSH_BASTION_USER=${USER} YC_ZONE=ru-central1-a SSH_USER=${USER} COMMIT_REVISION=${REVISION} YC_SUBNET_ID=buc16cn643nsi2ann5ro YC_ENDPOINT=api.cloud-preprod.yandex.net:443 YC_FOLDER_ID=aoem8bhifb86itsejqlb packer build --debug --on-error=ask build-agent-image.json`
