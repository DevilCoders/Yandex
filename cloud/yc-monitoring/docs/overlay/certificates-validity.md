[Алерт certificates-validity в джаглере](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcertificates-validity)

## Что проверяет

- срок действия и цепочку для указанных в конфиге сертификатов (используются для аутентификации в contrail-api, cassandra)

- горит жёлтым за 31 день до истечения срока действия и красным за 7 дней

## Если загорелось

- [перевыпустить сертификаты](https://wiki.yandex-team.ru/cloud/devel/sdn/contrail-secrets-update/), чей срок действия подходит к концу