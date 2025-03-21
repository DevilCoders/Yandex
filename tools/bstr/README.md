# Краткое описание

На принимающей стороне нужно запустить *bstr pull* - это демон, который будет скачивать все новые версии нужных файлов в указанную директорию.

На раздающей стороне нужно запустить *bstr push*, указав путь к файлу, который нужно раздать. Программа будет ждать наступление одного из двух событий - или когда станет известно, что файл уже есть на *quorum* хостов, либо когда наступит таймаут.

Для функционирования bstr нужно передать yt token, так как для коммуникации между раздающей и принимающей частями программы используется Кипарис.

Группа раздачи определяется параметром *-r*, который должен быть одинаковым для приемников, и для раздающего. Фактически, это просто путь в файловой системе Кипариса.

Приемники могут слушать разный набор файлов из раздачи(к ним попадают только те файлы, которые переданы через опции *—-file*, *—-list*), раздающий всегда раздает один файл(для простоты, вам никто не мешает параллельно запустить несколько раздач).

Программа поддерживает следующий набор фич:
 * Ограничение по размеру старых версий файла на приемнике(да, bstr хранит столько версий файла, сколько сможет, для быстрого отката на старые данные)
 * Запуск произвольной команды по прибытии нового файла
 * Раздающий может указать приоритет файла для приемника
 * Раздающий может форсировать раздачу более старого файла, чем есть на приемниках
 * Поддержка O_DIRECT
 * Ограничение числа одновременных закачек
 * Ограничение скорости закачек
 * Раздающий умеет поднимать в YT skynet copier в своем контейнере

# Примеры использования

https://st.yandex-team.ru/SUGGEST-475
https://st.yandex-team.ru/BSSERVER-1290

# Подробное описание

```
./bstr push --help
./bstr pull --help
```

# Мониторинги
Для того чтобы включить мониторинги, необходимо добавить *--enable-monitoring* в аргументы команды.
И тогда в отдельном потоке запускается агент для пулл-схемы в Соломон (порт агента по умолчанию - 5005, можно указать другой через *--agent-port*).

Ручка для пулл запросов - **/sensors**

При [настройке сервиса](https://wiki.yandex-team.ru/solomon/howtostart/) в Соломоне рекомендуется выставить
**grid = 1s** и включить **Add timestamp args** для округления времени метрик по указанной сетке.
Иначе, будет работать округление по интервалу. Об этом можно прочитать [здесь](https://wiki.yandex-team.ru/users/guschin/public/drafts/solomon/the-grid/#pull-sxemasbuferizaciejjperedachejjneskolkixtsvzaprose).


Список метрик для **bstr pull**:
  - *in_progress.percent* - сколько процентов от общего размера было загружено
  - *in_progress.bytes* - сколько байтов от общего размера файла было загружено
  - *in_progress.speed* - текущая скорость загрузки
  - *completed.retry_attempts* - общее количество ретраев в процессе загрузки блоков файла
  - *completed.errors* - если по итогу загрузки все прошло успешно - то пишется 0, в противном случае 1

Список меток - *host*, *filename*, *mode = pull | push*.
