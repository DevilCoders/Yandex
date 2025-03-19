[Алерт в Solomon](https://solomon.yandex-team.ru/admin/projects/yandexcloud/alerts?text=Compute+node+token-agent), [Алерт в Juggler](https://juggler.yandex-team.ru/aggregate_checks/?query=namespace%3Dycloud%26service%3Dcompute-node-token-agent)

## compute-node-token-agent
Проверяет, что:
- сервис compute-node (Go и Python) имеет свежий IAM-токен (для общения с другими сервисами),
- нет ошибок при обращении к token-agent.

## Подробности
Загорится `WARN`, если:
- token-agent возвращает ошибки (например, `Unavailable`), такое может случаться (кратковременно), например, при релизах token-agent. В это время **не стоит перезапускать `yc-compute-node`** (т.к. токен кэшируется в памяти и после перезапуска пропадет, а новый запросить не получится),
- IAM-токен не обновляется больше 2 минут после наступления `refresh_at`.

Загорится `CRIT`, если токен не будет обновлен по истечении 10 минут после наступления `refresh_at`.

IAM-токен нужен для:
* всех походов в Compute Private API. Соответственно, без него нода перестанет принимать новые инстансы, а уже спущенные станут неуправляемыми через API.
* в перспективе - для взаимодействия с VPC Node. Соответственно, без токена не сможем даже переподнять упавший инстанс (будем ретраить, пока не появится).

У каждого IAM-токена есть:
- время выписки (`issued_at`)
- время истечения (`expires_at`, порядка 10 часов). После его наступления Compute Node потеряет возможность использовать токен;
- время обновления (`refresh_at = issued_at + 0.1 * (expires_at - issued_at)`). После его наступления клиент в Compute Node будет пытаться обновить токен через token-agent.


## Диагностика
- проверяем, что запущен token-agent на машине: `sudo systemctl status yc-token-agent`. Если нет, идем к `/duty iam`. Можно сразу глянуть `sudo tail /var/log/yc/token-agent/access.log /var/log/yc/token-agent/token-agent.log` на наличие ошибок (и сдать их дежурному),
- если сервис запущен и выглядит живым, смотрим подробнее в логи ноды: `sudo journalctl -u yc-compute-node --since -30min | grep -i token`,
- действуем по обстоятельствам,
- _в самом крайнем случае в Compute Private API есть возможность выключить требование авторизации клиентов. В compute-node - есть возможность выключить походы в token-agent (`iam_token_source = none`)._

## Ссылки
- [CLOUD-46754](https://st.yandex-team.ru/CLOUD-46754)
- [CLOUD-66144](https://st.yandex-team.ru/CLOUD-66144)
- [Рекомендации по обновлению токенов из IAM Cookbook](https://docs.yandex-team.ru/iam-cookbook/2.authentication_and_authorization/service_tokens)
- [Документация по token-agent](https://wiki.yandex-team.ru/cloud/iamrm/services/token-agent)
- Можно обращаться к @l2k1, @simonov-d, @vbalakhanov
