[Алерт в Solomon](https://solomon.yandex-team.ru/admin/projects/yandexcloud/alerts?text=solomon-agent+push)

## Описание
Горит, если есть проблемы с отправкой метрик в Соломон через Push. Push-ем
отправляются видимые пользователю метрики по использованию инстанса.
В случае CRIT в пользовательских графиках могут появляться дыры -
с расследованием надо поторопиться.

![](https://jing.yandex-team.ru/files/simonov-d/CloudUserMetrics.png)

## Подробности
Пользовательские метрики мы отправляем в Облачные инсталляции Solomon (Cloud PROD, Cloud PRE-PROD, Cloud ISRAEL).

Что может пойти не так:
- кончилась квота (например, на кол-во метрик в шарде - такое бывает для больших облаков),
```
Mar 25 13:22:07 vla04-3ct17-5a.cloud.yandex.net solomon-agent[285083]: 2022-03-25 13:22:07.396 WARN  {pusher.cpp:561}: Shard{project=b1gfcpod5hbd1ivs7dav; cluster=b1gd42o63plo2n3sd46j; service=compute} [rid=454b2b03e4163295f3eb2431c2f56bbd] failed to send data to https://solomon.cloud.yandex-team.ru:443/api/v2/push with code HTTP_REQUEST_ENTITY_TOO_LARGE: {"status":"QUOTA_ERROR","errorMessage":"more than 1000000 metrics in shard","sensorsProcessed":70}
```
- слишком много метрик в запросе,
- проблемы с аутентификацией.
- проблемы с сетью, в этом случае в логах агента можно найти WARN вида
```
2022-03-25 13:42:34.618 WARN  {pusher.cpp:558}: Shard{project=yc.vpc.monitoring; cluster=yc.vpc.mr-prober; service=compute} [rid=de9b916a8ef159b72e0dbc3ba423ef83] failed to send data to https://solomon.cloud-preprod.yandex-team.ru:443/api/v2/push: Resolving timed out after 5001 milliseconds
```
- коллизии (должно починиться в рамках SOLOMON-7909)
```
022-03-24 11:11:20.351 WARN  {pusher.cpp:561}:
Shard{project=aoevcpnp2dmogo6u33ka; cluster=aoe2fnrknrpgsho3chc5; service=compute} [rid=9b68cd733b616e92871550df6a15a46b] failed to send data to https://solomon.cloud-preprod.yandex-team.ru:443/api/v2/push with code HTTP_BAD_REQUEST:{"type":"INVALID_ARGUMENT","message":"cluster with id \"aoevcpnp2dmogo6u33ka_aoe2fnrknrpgsho3chc5\" already exist with label name equal to\"lockbox\", but expected label name \"aoe2fnrknrpgsho3chc5\"","code":400}
```

## Диагностика
- Посмотреть логи solomon-agent: `journalctl -u solomon-agent --since -1h | grep -i push`
- [Инструкция от команды Solomon](https://wiki.yandex-team.ru/solomon/agent/troubleshooting/)
- [Форма запроса на увеличение квоты шарда](https://forms.yandex-team.ru/surveys/28780/)
- Если уперлись в квоту на пользовательском шарде, прилинковать MONITORINGREQ-тикет на увеличение квоты к [CLOUD-97013: Не мониторится превышение квот пользовательскими шардами](https://st.yandex-team.ru/CLOUD-97013)
- Если в логах агента ничего не находится, сходите по ссылке в Solomon-алерт, а из него на графики `dataPusher.Ok` и `dataPusher.Error`. Возможно, пропали `Ok`-и. [MONSUPPORT-1596: solomon-agent перестал пушить метрики](https://st.yandex-team.ru/MONSUPPORT-1596)

**Памятка по заполненю формы (на увеличение квоты шарда):**
- **инсталляция**: выбираем облачную (`solomon.cloud.*` либо `solomon.cloud-preprod.*`)
- **идентификатор проекта**: для пользовательских метрик это `cloud_id`, поискать/проверить можно [здесь](https://solomon.cloud.yandex-team.ru/admin) (Cloud Prod Solomon)
- **идентификатор шарда**: найдите ваш шард во вкладке `Shards`. [Пример для облака b1gfcpod5hbd1ivs7dav](https://solomon.cloud.yandex-team.ru/admin/projects/b1gfcpod5hbd1ivs7dav/shards) (Cloud Prod Solomon). Имеет вид `<CLOUD_ID>_<FOLDER_ID>_compute`.
