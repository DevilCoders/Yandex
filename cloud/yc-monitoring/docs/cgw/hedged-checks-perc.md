# hedged-checks-perc

**hedged чек** - вспомогательный чек, который отправляется вслед основному, если на тот мы не получили ответа за половину таймаута
**Значение:** hc-node получают существенный процент результатов чеков в следствии успешности hedged чека.
**Воздействие:** Пользователи видят таймауты при запросах к балансерам, которые иногда проходят при ретраях.
**Что делать:**
[График количества чеков](https://solomon.cloud.yandex-team.ru/?project=yandexcloud&cluster=cloud_prod_ylb&service=healthcheck_node&l.host=%21cluster&l.name=hedged_checker_primary_request_start&l.env=prod&graph=auto&transform=differentiate&b=1w&e=)
[График количества запущеных hedged чеков](https://solomon.cloud.yandex-team.ru/?project=yandexcloud&cluster=cloud_prod_ylb&service=healthcheck_node&l.host=%21cluster&l.name=hedged_checker_backup_request_start&l.env=prod&graph=auto&transform=differentiate&b=1w&e=)
[График успешных hedged чеков](https://solomon.cloud.yandex-team.ru/?project=yandexcloud&cluster=cloud_prod_ylb&service=healthcheck_node&l.host=%21cluster&l.name=hedged_checker_backup_request_win&l.env=prod&graph=auto&transform=differentiate&b=1w&e=)
* Посмотреть проводились ли какие-то работы у netinfra, сети или в самом балансере.
* Гадаем по графикам
