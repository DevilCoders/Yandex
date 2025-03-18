## Releasing
Про то как катить релиз


1. [Cборка релиза](#сборка-релиза)
1. [Откат релиза](#откат-релиза)

## Сборка релиза
1. Завести релизный тикет по шаблону `CONFIGS_API_RELEASE`. (Подключить себе шаблон можно, перейдя по [ссылке][3] и нажав "ВКЛ")
1. Собрать информацию для релизного тикета
`svn up`
`svn log -r 3997283:HEAD svn+ssh://arcadia.yandex.ru/arc/trunk/arcadia/antiadblock/configs_api/`
где 3997283 - номер ревизии предыдущего релиза **+1** [фильтр в стартрек][2]
svn log надо положить в описание релизного тикета в cut
1. Найти в testenv последний собраный пакет configs_api (последние со статусом "ок" и номер ревизии больше или равен вашему комиту: 
переходим в сэндбокс таску - кликаем на ОК, на закладке Resources ресурс с именем YA_PACKAGE). 
[джоба configs_api в testenv][1]  
Если есть исправления или дополнительные фичи, закоммиттить их в trunk, дождаться сборки, сделать svn up и обновить релизный тикет
 `svn log -r 3997283:HEAD svn+ssh://arcadia.yandex.ru/arc/trunk/arcadia/antiadblock/configs_api/`  
1. Пакет надо выкатить на [препрод инсталляцию админки](https://aabadmin11.n.yandex-team.ru), описание выкатки в самогон смотри в [samogon](../samogon/README.md)
1. После успешного приемочного тестирования, занести в релизный тикет id sandbox ресурса ([фронт для приемочного тестирования][4])
1. Деплоим пакет на [продакшен инсталляцию админки](https://aabadmin.n.yandex-team.ru)
1. зайти на [прод](https://antiblock.yandex.ru), создать и выкатить новый конфиг у партнера test
1. Зайти на [дашборд](https://ubergrep.yandex-team.ru./app/kibana#/visualize/edit/2f0c0d90-1ad7-11e8-97a1-0d1a97822c00?_g=()&_a=(filters:!(),linked:!f,query:(language:lucene,query:'partner:test%20AND%20action:%22config_updated%22'),uiState:(),vis:(aggs:!((enabled:!t,id:'1',params:(customLabel:Version),schema:metric,type:count),(enabled:!t,id:'2',params:(customInterval:'2h',extended_bounds:(),field:'@timestamp',interval:auto,min_doc_count:1),schema:segment,type:date_histogram),(enabled:!t,id:'3',params:(field:cfg_version,missingBucket:!f,missingBucketLabel:Missing,order:desc,orderBy:'1',otherBucket:!f,otherBucketLabel:Other,size:5),schema:group,type:terms),(enabled:!t,id:'4',params:(field:partner,missingBucket:!f,missingBucketLabel:Missing,order:desc,orderBy:'1',otherBucket:!f,otherBucketLabel:Other,row:!t,size:5),schema:split,type:terms)),params:(addLegend:!t,addTimeMarker:!f,addTooltip:!t,categoryAxes:!((id:CategoryAxis-1,labels:(show:!t,truncate:100),position:bottom,scale:(type:linear),show:!t,style:(),title:(),type:category)),grid:(categoryLines:!f,style:(color:%23eee)),legendPosition:right,seriesParams:!((data:(id:'1',label:Version),drawLinesBetweenPoints:!t,interpolate:linear,mode:stacked,show:true,showCircles:!t,type:area,valueAxis:ValueAxis-1)),times:!(),type:area,valueAxes:!((id:ValueAxis-1,labels:(filter:!f,rotate:0,show:!t,truncate:100),name:LeftAxis-1,position:left,scale:(mode:normal,type:linear),show:!t,style:(),title:(text:Version),type:value))),title:'Config%20versions',type:area)))
и убедиться, что конфиг успешно раскатился
1. В случае успеха, закрыть релизный тикет

### Графики балансеров админки (пока только стандартные):
1. [L3-балансер](https://grafana.yandex-team.ru/d/5kLM1tdWz/l3-vs-api-aabadmin-yandex-ru?orgId=1)
2. [L7-балансер](https://nanny.yandex-team.ru/ui/#/awacs/namespaces/list/api.aabadmin.yandex.ru/monitoring/common/) - можно смотреть отдельно по ДЦ или все графики вместе

## Откат релиза 

Если пошли ошибки, упали графики и пропала реклама, нужно принять решение об откате.

Безопасно можно откатится на предыдущую ревизию, если 
- в релизе с ошибкой не было миграций, или мигарции не меняли схему базы данных 
- в релизе не было новых параметров (новый параметр меняет схему конфигов и при откате пойдут ошибки валидации)
Если это так, можно найти последний успешный релиз в тикете в [фильтре][2] и выкатить его. id sandbox протестированных ресурсов должны быть в тикете.
Неудачный релиз нужно перевести в статус "не будет исправлено". После того как работоспособность сервиса восстановлена, нужно откатить код в trunk на известную работоспособную версию или вмерджить фикс.

Если же в неудачном релизе были несовместимые с предыдущем кодом миграции, то нужно чинить проблему в транке(или откатить все кроме миграций и дописать миграцию, возвращающую предыдущее состаяние бд) и катить новый релиз.

Если в транке найден новый код, блокирующий выпуск, ломающий другие фичи автор кода должен починить транк или откатить правки.
Ответственный за релиз должен оперативно оповещать разработчиков о проблемах с кодом.
Исправление бага в транке, т.е. "Красный" транк, имеет приоритет "Блокер".

[1]: https://beta-testenv.yandex-team.ru/project/antiadblock/job/PACKAGE_CONFIGS_API/history?limit=13
[2]: https://st.yandex-team.ru/ANTIADB/order:updated:false/filter?resolution=notEmpty()&type=12&components=38884
[3]: https://st.yandex-team.ru/settings/templates/issues?name=CONFIGS_API_RELEASE
[4]: https://preprod.antiblock.yandex.ru/
