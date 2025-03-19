# e2e-tests

**Значение:** e2e тесты балансера завершаются ошибкой одного или более тестов.
**Воздействие:** Impact от потенциальной неработоспособности config/control/data plane балансера до минорных проблем в различных, не связанных напрямую с балансером, сервисах.
**Что делать:** Почитать логи выполнения тестов можно:
`sudo journalctl LOGGER_NAME=e2e -S -1h`
Дальше надо смотреть в причины падений. Если ошибки вида "quota exceeded" или "address space exhausted", то надо удалить артефакты от предыдущих запусков.
Подчистить логи можно с помощью настроенного [yc](https://wiki.yandex-team.ru/cloud/support/man/tools/#publicyandex.cloudcliyc). Например, удалить артефакты последних 12 часов для активированного профиля e2e:
`yc lb nlb list | perl -ane 'next unless $F[3] =~ /^lb-e2e-blncr-(\d+)/; next if $1 > (time() - 12 * 60 * 60); print "yc lb nlb delete $F[1] && yc lb tg delete $F[11]\n";' | bash -ex -`
Для настроек:
* [preprod](https://console-preprod.cloud.yandex.ru/folders/aoe2u22cs9gf36f0s362)
* [prod](https://console.cloud.yandex.ru/folders/b1ggsg2u72too6ln973u)
```
➜  ~  cat ~/.config/yandex-cloud/config.yaml
current: ylb-prod-e2e
profiles:
ylb-preprod-e2e:
token: xxxx
endpoint: api.cloud-preprod.yandex.net:443
cloud-id: aoec7so9h6ss21me30ij
folder-id: aoe2u22cs9gf36f0s362
ylb-prod-e2e:
token: xxxx
cloud-id: b1glc055qh454ciijpst
folder-id: b1ggsg2u72too6ln973u
```
Также логи можно найти в:
`sudo less /var/log/monrun/monrun.log`
Что бы выгрепать компьютные логи:
* Ищем на каких воркерах запускалась операция `pssh run-e -p 10 -a -H C@cloud_preprod_head "sudo journalctl -u yc-compute-worker -S -1h OPERATION_ID=$OPERATION_ID"` ??где посмотреть, если логи отротировались??
* Далее заходим на интресующий воркер и там `sudo journalctl -u yc-compute-worker -S -2h OPERATION_ID=$OPERATION_ID`
Сборка образов https://beta-testenv.yandex-team.ru/project/cloud-loadbalancer/timeline , код https://a.yandex-team.ru/arc/trunk/arcadia/cloud/load-balancer/selfhost/packer/e2e
