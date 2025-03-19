Полная информация про скрипт, его использование и пререквизиты для него: https://wiki.yandex-team.ru/cult-admin/tulza-dlja-zavedenija-anstejjblov-v-avakse/

Example usage:
```
ya make && ./awacs_stage_inserter --config config.json
```

Запускаем программу с путём до конфига, либо при помощи опций командной строки.

То есть данная команда
```
./awacs_stage_inserter --action insert_unstable --endpoint-set quiz-and-questionnaire-api_unstable.pr-1234 --namespace kp-nonprod-balancer.kp.yandex.net --datacenters man --fqdns pr-1234-quiz-and-questionnaire-api.unstable.kp.yandex.net --domain unstable.kp.yandex.net --guids 61458 31277 --users robot-kp-java
```

Эквивалентна запуску программы со следующем конфигом:
```
{
    "action": "insert_unstable",
    "endpoint_set": "quiz-and-questionnaire-api_unstable.pr-1234",
    "namespace": "kp-nonprod-balancer.kp.yandex.net",
    "datacenters": ["man"],
    "fqdns": ["pr-1234-quiz-and-questionnaire-api.unstable.kp.yandex.net"],
    "domain": "unstable.kp.yandex.net",
    "guids": ["61458", "31277"],
    "users": ["robot-kp-java"]
}
```

При этом !!аргументы командной строки оверрайдят аргументы из конфига!!. Поэтому, можно добавлять опции по необходимости, прямо из командной строки, например `./awacs_stage_inserter --config /home/coldmind/tmp/config.json` 


Есть функционал добавления не только лишь анстейбла, но и обычного стейджа (action `insert_stage`)

Пример config.json
```
{
    "action": "insert_stage",
    "endpoint_set": "film-list-api_production.deploy_unit",
    "namespace": "kp-backend-prod.kp.yandex.net",
    "datacenters": ["man", "sas", "iva"],
    "fqdns": ["film-list-api.kp.yandex.net", "film-list-api-deploy.kp.yandex.net"],
    "domain": "film-list-api.kp.yandex.net",
    "guids": ["85904"],
    "users": ["robot-kp-java"],
    "wait": "False",
    "abc_id": 645
}
```

Создаст бекенды
* https://nanny.yandex-team.ru/ui/#/awacs/namespaces/list/kp-backend-prod.kp.yandex.net/backends/list/film-list-api_production_deploy_unit_iva/show/
* https://nanny.yandex-team.ru/ui/#/awacs/namespaces/list/kp-backend-prod.kp.yandex.net/backends/list/film-list-api_production_deploy_unit_man/show/
* https://nanny.yandex-team.ru/ui/#/awacs/namespaces/list/kp-backend-prod.kp.yandex.net/backends/list/film-list-api_production_deploy_unit_sas/show/

Создаст апстрим
* https://nanny.yandex-team.ru/ui/#/awacs/namespaces/list/kp-backend-prod.kp.yandex.net/upstreams/list/film-list-api_production_deploy_unit/show/

Создаст домен
* https://nanny.yandex-team.ru/ui/#/awacs/namespaces/list/kp-backend-prod.kp.yandex.net/domains/list/film-list-api.kp.yandex.net/show/

Закажет серт
* https://nanny.yandex-team.ru/ui/#/awacs/namespaces/list/kp-backend-prod.kp.yandex.net/certs/list/film-list-api.kp.yandex.net/show/


DNS нужно будет прописать самому.
