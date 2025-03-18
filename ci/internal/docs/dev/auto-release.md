## Авторелизы

### Дашборд в Соломоне
[Дашборд в Соломоне](https://solomon.yandex-team.ru/?project=ci&cluster=stable&dashboard=ci-auto_release)

### Мониторинги
Положение алерта в дажглере [на дашборде](https://juggler.yandex-team.ru/dashboards/ci):

{% cut "Скрин" %}

![алерт авторелиза в джагглере](img/auto-release-alert-at-dashboard.png =1200x)

{% endcut %}

### Что делать, когда сработал мониторинг?
1. Определяем, какой именно алерт сработал: ```dir-discovery``` или ```graph-discovery```
2. Заходим на дашборд: [прод](https://solomon.yandex-team.ru/?project=ci&cluster=stable&service=tms&dashboard=ci-auto_release&b=1h&e=), [тестинг](https://solomon.yandex-team.ru/?project=ci&cluster=testing&service=tms&dashboard=ci-auto_release&b=1h&e=).
  
    Мониторинг сработывает, когда ```discovery_delay (minutes)``` (верхний график) превышает определённый порог.

    Чтобы авторелизы работали, мы должны быть уверены, что все коммиты в транке (а авторелизы работают только в транке) были задискаверены. То есть среди коммитов, которые войдут в следующий релиз, не должно быть пропусков. В CI есть процесс, который двигает маркер "все коммиты до этого задикаверены" вверх по истории. Сработавший алерт значит, что обработка какого-то коммита застряла/слишком долго выполняется.

#### Причины отставания dir-discovery

1. Ошибка дискаверинга конфигов в коммите (базинга-таска **ProcessPostCommit** падает с ошибкой)

    {% cut "Решение" %}

    Выполняется базинга-таской **ProcessPostCommit** ([прод](https://ci-tms.in.yandex-team.ru/z/bazinga/onetime-jobs/processPostCommit), [тестинг](https://ci-tms-testing.in.yandex-team.ru/z/bazinga/onetime-jobs/processPostCommit)). В этом случае базинга-таска падает с ошибкой, реатраится и снова падает с ошибкой.

    **Рекомендуется**: починить обработку **ProcessPostCommit** или откатить наш релиз с багом (если это возможно).

    **НЕ рекомендуется** помечать подвисший коммит как задискаверенный, так как от этого может пострадать changelog релизов. Этот вариант фикса стоит согласовывать с [@andreevdm](https://staff.yandex-team.ru/andreevdm).  

    Пометить подвисший коммит как задискаверенный можно через базу. hash-коммита виден в базинга-задаче **ProcessPostCommit**. Нужно выполнить следующий запрос: 
    ```sql
    --!syntax_v1
    $id = 'xxxxx';
    update `main/CommitDiscoveryProgress` set dirDiscoveryFinished = true where id = $id;
    ```
    YQL к тестовой [базе](https://yql.yandex-team.ru/Operations/YHbTeAPTTiWGQp-x4sSEC2jzzpibD5ldVoHQZeZV404=), к продовой [базе](https://yql.yandex-team.ru/Operations/YHbUCyyLNXxKkJlKQ0NdMfHstVb0ZGKo8czuixt3gEg=).

    {% endcut %}

2. Дискаверинг конфигов в коммите не был выполнен (базинга-таска **ProcessPostCommit** не была создана) 

    {% cut "Решение" %}

    Редкий случай, может произойти при выкатке кода, который по каким-то причинам не запустил для коммита базинга-таску **ProcessPostCommit** ([прод](https://ci-tms.in.yandex-team.ru/z/bazinga/onetime-jobs/processPostCommit), [тестинг](https://ci-tms-testing.in.yandex-team.ru/z/bazinga/onetime-jobs/processPostCommit)). Или же **ProcessPostCommit** был удалён через админку, или закончилось кол-во ретраев.
    
    - В базе выполняем запрос: [прод](https://yql.yandex-team.ru/Operations/YK6DvvMBw877Wfj5l01uNF5bzpAoxXCTaJywuj9oIys=), [тестинг](https://yql.yandex-team.ru/Operations/YK6DkPMBw877Wfjg9ZbjTMmXKuI_iPbkVmaTQpmiHrA=). На выходе получаем три результата:
      - **Result #1** содержит последний обработанный коммит и его потомка (не родителя). Этот коммит можно было узнать и из лога базинга крон-таски `discoveryProgressCheckerCron` ([прод](https://ci-tms.in.yandex-team.ru/z/bazinga/tasks/discoveryProgressCheckerCron), [тестинг](https://ci-tms-testing.in.yandex-team.ru/z/bazinga/tasks/discoveryProgressCheckerCron)). Потомок коммита может быть `null`. когда CI ещё не узнал о потомке (например, когда не пришло событие из ЛогБрокера от Арка)
      - **Result #2** проверяет, есть ли коммит в таблице `main/Commit`
      - **Result #3** проверяет, есть ли запись о прогрессе дискаверинга конфигов в таблице `main/CommitDiscoveryProgress`
      
      Если понадобится вставить отсутствующую запись в `main/CommitDiscoveryProgress`, то можно сделать так: [тестинг](https://yql.yandex-team.ru/Operations/YKvLDBJKfbwxFbJwHBp82k3an-6aRjHnOyDJcYG4IUQ=).

      Запись в `main/CommitDiscoveryProgress` может отсутствовать, когда базинга-таска **ProcessPostCommit** вообще не была созадана. Проверить это можно, елси найти время коммита и отлистать все выполненные задачи через админку базинги.
      
      Можно ещё посмотреть логи в [проде](https://deploy.yandex-team.ru/stages/ci-tms-stable/logs?deploy-unit=tms&query=message%3D308ab33fae5bb783d75ea0653f3ce1ff4211bb5d%3B), в [тестинге](https://deploy.yandex-team.ru/stages/ci-tms-testing/logs?deploy-unit=tms&query=message%3D308ab33fae5bb783d75ea0653f3ce1ff4211bb5d%3B).

    {% endcut %}

#### Причины отставания graph-discovery

На дашборде, открытом на предыдущем шаге, внизу находим график ```GRAPH discovery_running_sandbox_tasks```. Если максимальное время выполнения sandbox-задачи растёт и превысило порог, то проблема в sandbox-задаче, иначе проблема в CI, который долго обрабатывает результаты sandbox-задачи.

1. Sandbox-задача, вычитающая графы, выполняется долго

    {% cut "Решение" %}

    В норме должны выполняться минут за 15, порог срабатывания алерта - минут 40.
    - Находим СБ-задачу: когда алерт сработал [для прода](https://sandbox.yandex-team.ru/tasks?children=true&author=robot-ci&owner=CI_GRAPH_DISCOVERY&status=EXECUTE%2CQUEUE&order=%2Bid&limit=20) или [для тестинга](https://sandbox.yandex-team.ru/tasks?children=true&author=robot-ci-testing&owner=CI_GRAPH_DISCOVERY&status=EXECUTE%2CQUEUE&order=%2Bid&limit=20). Сортировкой находим минимальный ```id``` - это задача выполняется дольше всего.
    - Скидываем в slack в канал ```tier0-graph-discovery```, где есть [v-korovin](https://staff.yandex-team.ru/v-korovin). Просим посмотреть.

    {% note warning %}

    Во время обновления Distbuild'а sandbox-задачи таймаутятся. Узнать об обновлении ДБ можно по событию [в инфре](https://infra.yandex-team.ru/timeline?preset=YerLTPA8B34&autorefresh=false&status=all&filter=&fullscreen=false) (Arcadia DistBuild: Production).

    {% endnote %}

    {% endcut %}

2. CI долго обрабатывает результаты sandbox-задачи

    {% cut "Решение" %}

    В этом случае заводим подзадачу к [st/CI-2002](https://st.yandex-team.ru/CI-2002) на исследование причины долгой обработки результата. В тикете указываем:
    - прикладываем ссылку на долго работающую базинга-таску
    - прикладываем ссылку на sandbox-задачу (номер есть в описании базинги таски)
    - желательно пересохранить ресурс ```AFFECTED_TARGETS``` из sandbox-задачи, так как у него ttl 2 дня. Можно скачать и загрузить через ```ya upload --mds --ttl 90 путь/до/файла```
  
    Может случиться так, что sandbox-задача в тестинге никогда не сможет быть завершена. Тогда нужно:
    - пометить коммит, как задискаверенный, и предотвратить бесконечные попытки перезапуска sandbox-задачи. Коммит можно взять из описания sandbox-задачи:
      ```sql
      --!syntax_v1
      $id = 'xxx';
      update `main/CommitDiscoveryProgress` set graphDiscoveryFinished = true where id = $id;
      delete from `main/GraphDiscoveryTask` where commitId = $id;
      ```
      YQL к тестовой [базе](https://yql.yandex-team.ru/Operations/YHbhK794hnvrCSsRF-_gpCSq733zdoogyr7gJaZ2EeQ=), к продовой [базе](https://yql.yandex-team.ru/Operations/YHbhXr94hnvrCSssLzPgDY1Q-45WRdOXiTalmPTp8q4=).
    - остановить sandbox-задачу через UI интерфейс sandbox'а.

    {% endcut %}

3. Sandbox-задачи не могут отработать в результате ошибки в самой задаче (бесконечные падения в exception с последующим ретраем).

    {% cut "Решение" %}

    По поводу фикса задач обращаться к [v-korovin](https://staff.yandex-team.ru/v-korovin) (например в slack канале ```tier0-graph-discovery```). 
  
    Вероятно сломанные задачи необходимо будет пересоздать. Тогда нужно:
    - Определить список отравленных задач (это может быть диапазон ревизий ```rightSvnRevision```, список ```sandboxTaskId```, список ```commitId``` и тд.):
      ```sql
      --!syntax_v1
      select commitId, sandboxTaskId from `main/GraphDiscoveryTask` where rightSvnRevision >= 8227536 AND rightSvnRevision <=8227793;
      ```

       {% note warning %}

       Обязательно сохранить к себе список (скачать) ```commitId``` перед дальнейшими шагами.

       {% endnote %}

    - После сохранения списка коммитов на перезапуск необходимо удалить старые записи из базы:
      ```sql
      --!syntax_v1
      delete from `main/GraphDiscoveryTask` where rightSvnRevision >= 8227536 AND rightSvnRevision <=8227793;
      ```
    - Теперь можно пересоздать задачи для graphDiscovery на выбранных коммитах через grpc-ручку: 
      - Настраиваем нужные переменные:
        ```bash
        # в примере хост и tvm ключ продовые
        TARGET_HOST="ci-api.in.yandex-team.ru:4221" 
        TVM_SERVICE_TICKET=$(ya tool tvmknife get_service_ticket sshkey --src 2018912 --dst 2018912 | sed  's/:/\\:/g')
        ```
      - Выполняем запрос (для каждого коммита):
        ```bash
        PAYLOAD='branch:"trunk",right_hash:"xxx"'; grpc_cli call --metadata=x-ya-service-ticket:${TVM_SERVICE_TICKET} ${TARGET_HOST} admin.GraphDiscoveryAdminService/ScheduleGraphDiscovery ${PAYLOAD}
        ```
    - При необходимости остановить sandbox-задачи через UI интерфейс sandbox'а (или через API). Но так как после удаления задач из базы они перестанут ретраиться, можно просто дождаться пока они все упадут.

    {% endcut %}

### Устройство авторелизов

Верхнеуровневое описание авторелизов (которое со временем может устареть):
1. CI узнаёт от Арка о новом коммите, узнаёт через ЛогБрокер или крон таску `PullTrunkCron`
2. CI создаёт запись в таблице `main/CommitDiscoveryProgress` для отслеживания прогресса дискаверинга коммитов
3. CI для коммита запускает в базинге две onetime-таски:
    - `ProcessPostCommit`, которая выполняет dir-discovery и записывает прогресс в таблицу `main/CommitDiscoveryProgress`
    - `GraphDiscoveryStart`, которая запускает sandbox-задачу для поиска затронутых build-зависимостей
4. В базинге есть крон-таска `GraphDiscoveryResultReadinessCron`, которая:
    - Следит за готовностью sandbox-задач, выполняющих graph-discovery. Задача одним rest-запросом получает статусы многих sandbox-задач.
    - Для успешных создаёт onetime базинга-таску `GraphDiscoveryResultProcessor`, которая обрабатывает результаты sandbox-задачи и записывает прогресс в таблицу `main/CommitDiscoveryProgress`
    - Для неуспешных создаёт onetime базинга-таску `GraphDiscoveryRestart`, которая перезапускает sandbox-задачу
5. В базинге есть крон-таска `DiscoveryProgressCheckerCron`, которая двигает указатель "последний обработанный коммит". Точнее, признак "для этого коммита и всех его предков выполнен дискаверинг".
    Указатель хранится в `main/KeyValue`:
    - в `(ns=DiscoveryProgressChecker.dirDiscoveryLastProcessed,key=trunk)`
    - в `(ns=DiscoveryProgressChecker.graphDiscoveryLastProcessed,key=trunk)`

    Для коммита признак хранится в `main/CommitDiscoveryProgress` в столбцах `dirDiscoveryFinishedForParents`, `graphDiscoveryFinishedForParents`.    
6. В базинге есть крон-таска `AutoReleaseCron`, которая выполняет запуски авторелизов.
