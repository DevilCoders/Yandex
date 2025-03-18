# Тасклеты помогающие в работе TrafficTeam

## Тасклет ItsDnscache(projects/tt/its_dnscache)
Нужен для работы с ITS над canary_dns_cache_{vla,sas}, production_dns_cache_{sas,vla,man} в контуре adm-nanny.yandex-team.ru.
Выставляет в `controls/config` json переданный в `its_value`
Пример конфигурации тасклета:
```yaml
run-its-prestable:
  task: projects/tt/its_dnscache
  title: Run its change
  needs: start_dummy
  stage: build
  input:
    config:
      token_yav_key: "nanny-token"
      its_value: "{\"delete_yp_cache\": ${flow-vars.yp-drop}}"
      container_filter: "^infra/dnscache/canary-(sas|vla).*$"
      its_url: "infra/dnscache/"
```


`token_yav_key: "nanny-token"` - имя ключа внутри YAV хранилища(который конфигурится на уровне всего пайплайна)
`its_value: "{\"delete_yp_cache\": false}"` - - особенность этого json, что для its это строка, но которая передаётся внутри json и которая всё равно проверяется json парсером(такая вот наша инфраструктура), поэтому все кавычки внутри должны быть экранированы
`container_filter: "^infra/dnscache/canary-(sas|vla).*$"` - фильтрует нужные нам сервисы-ручки, среди тех, что вернулись из `its_url`, сюда можно подставить всё что скомпилирует python re.compile
`its_url: "infra/dnscache/"` - урл, который вернёт список всех доступных нам ручек, а те что нам надо изменить мы отфильтруем `container_filter`


Исходники тут https://a.yandex-team.ru/arc/trunk/arcadia/noc/traffic/dns/ci/tasklets/its_dnscache

## Тасклет DnsTgNotification(projects/tt/dns_tg_noticication)
Нужен для отправки произвольных сообщений в произвольный ТГ канал чере nyanbot
Пример конфигурации тасклета:
```yaml
send_tg_message:
  task: projects/tt/dns_tg_notification
  title: Send message
  needs: start_dummy
  stage: build
  input:
    config:
      msg: "Hello TG!"
      chat_id: "-1001558284078"
      changelog_enable: true
```

chat_id - можно получить через nyanbot пригласив его на канал и сказав `/chatid@ugc_media_Bot` 
changelog_enable - значение True генерирует changelog добавляет его к сообщению(большой ченджлог в ТГ не придёт)
