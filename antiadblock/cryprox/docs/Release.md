1. [Cборка релиза](#сборка-релиза)
1. [Откат релиза](#откат-релиза)

## Сборка релиза

    Все фичи должны выходить с переключателем в админке. Пока не выкачена админка, фича не аппрувится.
    Без переключателя может выходить багфикс, когда о нем точно известно, что он ничего не сломает

1. Завести релизный тикет по шаблону `CRYPROX_RELEASE`. (Подключить себе шаблон можно, перейдя по [ссылке][8] и нажав "ВКЛ")
1. Собрать информацию для релизного тикета  
`svn up`  
`svn log -r 3997283:HEAD svn+ssh://arcadia.yandex.ru/arc/trunk/arcadia/antiadblock/cryprox/`  
где 3997283 - номер ревизии предыдущего релиза **+1** [фильтр в стартрек][5]  
Результат выполнения `svn log` надо положить в описание релизного тикета в `cut`
1. Найти в testenv последний собранный контейнер uni (последний со статусом "OK").  
    * [джоба uni в testenv][9]
Если есть исправления или дополнительные фичи, закоммиттить их в trunk, дождаться сборки, сделать svn up и обновить релизный тикет
1. Выкатить контейнеры на тестинг [Описание выкатки в облаках](/arc/trunk/arcadia/antiadblock/cryprox/docs/Nanny.md)
1. Протестировать сборку вручную:
    * Зайти на https://test.avto.ru, http://wwwy.avito.ru, https://testing.tv.yandex.ru и других крупных партнеров
    * Посмотреть ошибки в [соломон][4] и [ubergrep][3]
    (При этом новые фичи нужно включить в админке)
1. После успешного приемочного тестирования, занести в релизный тикет тэг протестированного докер-образа
1. Выкатить на production
    * [Дэшборд в соломоне][7]
    * [Ubergrep][6]
1. В случае успеха, закрыть релизный тикет кнопкой "Закрыть"


## Откат релиза 

Если пошли ошибки, упали графики и пропала реклама, нужно принять решение об откате на предыдущую версию.</br>
Найти последний успешный релиз в тикете в [фильтре][5] и выкатить его.</br>
Тэг протестированного докер-образа должен быть в тикете.</br>
Неудачный релиз нужно перевести в статус "не будет исправлено".</br>

После того как работоспособность сервиса восстановлена, нужно откатить код в trunk на известную работоспособную версию или вмерджить фикс.</br>
Если в транке найден новый код, блокирующий выпуск, ломающий другие фичи, не закрытый переключателем и т.д. автор кода должен починить транк или откатить правки.</br>
Ответственный за релиз должен оперативно оповещать разработчиков о проблемах с кодом.</br>
Исправление бага в транке, т.е. "Красный" транк, имеет приоритет "Блокер".</br>

[3]: https://ubergrep.yandex-team.ru/app/kibana#/discover?_g=(refreshInterval:(display:Off,pause:!f,value:0),time:(from:now-4h,mode:quick,to:now))&_a=(columns:!(_source),index:ca3084c0-e40b-11e7-ac23-1debda798e41,interval:auto,query:(language:lucene,query:'exception:in'),sort:!('@timestamp',desc))
[4]: https://solomon.yandex-team.ru/?cluster=cryprox-test&project=Antiadblock&service=cryprox_actions&l.http_code=5xx&l.host=*&l.http_host=*&l.service_id=*&graph=auto
[5]: https://st.yandex-team.ru/ANTIADB/order:updated:false/filter?type=release&status=closed&resolution=1&components=33372
[6]: https://ubergrep.yandex-team.ru/app/kibana#/discover?_g=(refreshInterval:(display:Off,pause:!f,value:0),time:(from:now-4h,mode:quick,to:now))&_a=(columns:!(_source),index:'72d0d380-e1e8-11e7-ac23-1debda798e41',interval:auto,query:(language:lucene,query:'exception:in'),sort:!('@timestamp',desc))
[7]: https://solomon.yandex-team.ru/?cluster=cryprox-prod&project=Antiadblock&service=antiadblock_nginx&dashboard=Antiadblock_Overall
[8]: https://st.yandex-team.ru/settings/templates/issues?name=CRYPROX_RELEASE
[9]: https://testenv.yandex-team.ru/?screen=job_history&database=antiadblock&job_name=BUILD_ANTIADBLOCK_UNI_PROXY_DOCKER
