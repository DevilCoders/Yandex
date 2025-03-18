Как собирать и деплоить:
1. заходим в папку /arcadia/antiadblock/monrelay/samogon
2. выполняем `ya package package.json`
3. выполняем команду<br/>
    `ya upload --ttl inf _имя_собранного_пакета_`<br/>
пример того как выглядит имя собранного пакета: <b>aab-monrelay.trunk@4569435.tar.gz</b> 
4. команда выведет айди загруженного сендбокс ресурса, его надо запомнить, вывод будет выглядить так, в этом выводе айди ресурса 862820194:<br/>
Created resource id is 862820194
5. заходим сюда [Deploy](https://ui-deploy.n.yandex-team.ru/update/monrelay/0 "Deploy") и в поле Package вставляем айдишник загруженного на предыдущем шаге ресурса, например, sbr:862820194
6. за ходом раскатки пакета можно следить по этой ссылке https://monrelay.n.yandex-team.ru/chart 


Продакшен контур самогона доступен по адресу: https://monrelay.n.yandex-team.ru
