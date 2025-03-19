### Как настроить окружение для работы локально
1. Настроить `ssh` для работы с сессионными ключами через bastion2, читать [тут](https://wiki.yandex-team.ru/cloud/yubikey/#3.opensshnerekomenduem) и [тут](https://wiki.yandex-team.ru/cloud/security/services/bastion2/openssh-client/)
2. Настроить `ya` для получения секретов, должен быть определен `YAV_TOKEN`
3. Настроить зеркало терраформ провайдеров, как описано тут: https://clubs.at.yandex-team.ru/ycp/4790
4. Запустить `terraform init -backend-config="secret_key=$(ya vault get version ver-01e9d91yf5qmcq891em025rwgr -o s3_secret_access_key 2>/dev/null)"`

### Как переналить агента
Через [teamcity таск](https://teamcity.yandex-team.ru/project/Cloud_Selfhost_BuildAgentsProject) указав fqdn агента и тикет по переналивке.

Руками локально:
1. Disable агента в teamcity с ожиданием окончания сборок:
  `./files/disable_tc_agent.py --fqdn build-agent-vla6.bootstrap.cloud-preprod.yandex.net --pool-id 147 --ticket-id ${TICKET_ID}`
2. Удалить вм:
`yc --profile=preprod-tca compute instance delete build-agent-vla6`
3. Удалить агента из teamcity:
  `./files/disable_tc_agent.py --fqdn build-agent-vla6.bootstrap.cloud-preprod.yandex.net --pool-id 147 --ticket-id ${TICKET_ID} --delete`
4. Пересоздать:
    * при обновлении образа и диска:
   `terraform apply -var-file="user.tfvars" -target="ycp_compute_instance.build-agents[\"build-agent-vla6\"]" -target="ycp_compute_disk.build-agent-data-disk[\"build-agent-vla6\"]" -target="null_resource.provision-build-agents[\"build-agent-vla6\"]"`
    * при пересоздании как есть:
    `terraform apply -var-file="user.tfvars"`

Секреты для билд агентов [тут](https://yav.yandex-team.ru/secret/sec-01esnghb8jnv363mnvqcp58few/explore/versions)

Чтобы обновить секреты в сборке:
1. Определить переменную `YAV_TOKEN` для получения секретов из yav
2. Определить `YC_TOKEN` с правами до ключа kms, например воспользовавшись sa с build-agent'а
3. Поправить `skm.yaml`, добавив необходимый секрет
4. Запустить `update-secrets.sh`
5. Закомитить изменения в `files/skm-md.yaml`

###TODO
1. брать данные для образа и снапшота из результатов работы packer'а
