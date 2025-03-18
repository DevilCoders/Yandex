## Описание стеджей
[Документация Deploy](https://wiki.yandex-team.ru/deploy/docs/)
Подняты три стейджа:
1. [develop](https://deploy.yandex-team.ru/stage/antiadb-front-develop) - 1 инстанс во VLA (0.25CPU, 1GB RAM, 5GB HDD)
2. [preprod](https://deploy.yandex-team.ru/stage/antiadb-front-preprod) - 1 инстанс во VLA (0.25CPU, 1GB RAM, 5GB HDD)
3. [production](https://deploy.yandex-team.ru/stage/antiadb-front-production) - по 1 инстансу во VLA, SAS в режиме MultiClusterReplicaSet (1CPU, 4GB RAM, 5GB HDD)
На каждом инстансе дополнительно потребляется 0.4CPU, 0.5GB RAM, 3GB HDD для логгирования


## Обновление стеджей (обновляется версия docker-образа)
1. Develop обновляется на каждый пулл-реквест
2. Preprod обновляется на каджый пуш в develop ветку
3. Production обновляется только при выкатке релиза


## Откат стейджа к предыдущей ревизии
1. Переходим на страницу History конкретного стейджа
2. Выбираем необходимую ревизию, жмем кнопку Apply, далее Apply with changes
![](https://jing.yandex-team.ru/files/dridgerve/deploy_revert_stage_1.png)
3. Далее открывается страница с конфигом, жмем Update
4. Открывается дифф спецификации, проверям, что изменились необходимые параметры (в частности тег docker-образа) и жмем Deploy
![](https://jing.yandex-team.ru/files/dridgerve/deploy_revert_stage_2.png)
5. Спецификация раскатится с новой ревизией


## Редактирование конфига стейджа
1. Переходим на страницу с конфигом конкретного стейджа и жмем Edit
![](https://jing.yandex-team.ru/files/dridgerve/deploy_stage_edit_1.png)
2. Обновить тег docker-образа (при необходиомости) можнол в разделе NodeJSBox
![](https://jing.yandex-team.ru/files/dridgerve/deploy_stage_edit_2.png)
3. Узнать тег можно на странице с пулл-реквестом в github
![](https://jing.yandex-team.ru/files/dridgerve/deploy_front_info.png)
4. В разделе NodeJSBoxWorkload можно обновить или добавить новые переменные окружения
![](https://jing.yandex-team.ru/files/dridgerve/deploy_stage_edit_3.png)
5. После редактирования, жмем Update, проверям diff и жмем Deploy
6. Не все настройки можно изменить, например изменить ДЦ, в котором живет стейдж не получится, так как текущий Endpoint set используется в настройках балансера


## Балансер
Поднят [L7-балансер](https://nanny.yandex-team.ru/ui/#/awacs/namespaces/list/antiblock.yandex.net/show/) в Awacs (по 1 минимальному инстансу во VLA, SAS)
Трафик между стейджами фронта распределяется на уровне [апстримов](https://nanny.yandex-team.ru/ui/#/awacs/namespaces/list/antiblock.yandex.net/upstreams/list/)
Выписан внутренний сертификат на домены %%antiblock.yandex.ru, *.antiblock.yandex.ru%%
