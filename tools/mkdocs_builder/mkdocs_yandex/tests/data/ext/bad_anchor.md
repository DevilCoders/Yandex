## Tutorial для начинающих
Можно пройти с использованием:

* Веб-интерфейса: [yql.yandex-team.ru/Tutorial](https://yql.yandex-team.ru/Tutorial/yt_01_Select_all_columns)
* Консольного клиента:
``` sh
$ svn co svn+ssh://arcadia.yandex.ru/arc/trunk/arcadia/yql/queries/tutorial
$ cd tutorial # далее заходим в подпапки по очереди и изучаем примеры с пояснениями
$ yql -i example.sql # пример запуска запроса из файла
$ yql -i example.sql -F example.py # запуск с приложенным Python скриптом
$ yql -i example.sql -U example.txt@$(cat example.txt.url) # прикладывание к запросу файла по URL
```
Не стоит бояться экспериментировать, в частности, попробовать использовать любые другие интересные вам данные в аналогичных запросах. По умолчанию все запросы выполняются на MapReduce кластерах от имени специального пользователя, у которого нет прав удалить что-либо чужое/важное/общественное.

<a name="feedback"></a>
## Обратная связь