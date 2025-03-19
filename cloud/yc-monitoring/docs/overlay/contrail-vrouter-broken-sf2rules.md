[Алерт contrail-vrouter-broken-sf2rules в джаглере](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcontrail-vrouter-broken-sf2rules)

## Что проверяет

Отсутствие сломанных ACL entry, мешающих корректному построению дерева правил super-flow-v2+
[Подробности в CLOUD-59958 →](https://st.yandex-team.ru/CLOUD-59958#5fca6ba4091691018c183dc5). Может ломать датаплейн.

В большинстве случаев агент сам может определять broken rules и автоматически отключать super-flow-v2+

## Если загорелось

- Посмотреть `yc-contrail-introspect-vrouter acl get ACL_UUID` - есть ли дублирующийся UUID (который загорелся)

   - Если есть, это ошибка в vpc-api: две разные ACL entry имеют одинаковый uuid: [тикет для привлечения внимания](https://st.yandex-team.ru/CLOUD-59962)

   - Если нет, это рейс в contrail-vrouter-agent, нужно собрать дамп `yc-contrail-introspect-vrouter collect -t operdb` и попробовать перезагрузить агента и призвать, наругав, @sklyaus

- Посмотреть в сообщении задетые (dependees) ACL и посмотреть `yc-contrail-introspect-vrouter sf2rule ACL_ENTRY_KEY -f json | jq '.[].entry.dependees'` какие интерфейсы задеты (с ключом, начинающимся на `itf:`), в `yc-contrail-introspect-vrouter ITF_UUID` проверить включён ли на интерфейсе `super-flow-v2.1`
