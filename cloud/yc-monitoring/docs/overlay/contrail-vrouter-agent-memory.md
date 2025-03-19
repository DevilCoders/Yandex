[Алерт contrail-vrouter-agent-memory в джаглере](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcontrail-vrouter-agent-memory)

Проверяет потребляемую память (RSS) процессом `contrail-vrouter-agent`.

Данные берутся из Solomon.

Нужен, чтобы заметить утечку заранее, до OOM.

Текущий лимит памяти для `contrail-vrouter-agent` - 20Gb.

При потреблении 60% от лимита загорается WARN.

При потреблении 90% от лимита загорается CRIT.

**Workaround** - перезапустить `contrail-vrouter-agent`.

Если есть массовая утечка, рекомендуется (до вывоза исправленной версии) постепенный контролируемый рестарт агентов ([пример](https://st.yandex-team.ru/CLOUDOPS-1107))