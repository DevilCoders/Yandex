# logshatter | logshatter-host | logshatter-cluster

**Значение:** Сломалась запись логов в logshatter, logshatter - проблема с живостью логшаттера, logshatter-host - проблема только на одном хосте, logshatter-cluster - в случае проблем на всем кластере хостов logshatter
**Воздействие:** Затруднена отладка и анализ сттистики по логам
**Что делать:**  [описание от маркета](https://wiki.yandex-team.ru/market/development/health/logshatter/)
1. Посмотреть в логах `/var/log/yandex/logshatter/logshatter.log`
2. Почитать description в проверке, если что-то написано про ошибки чтения из logbroker, то смотреть в [чате логброкера](https://telegram.me/joinchat/BvmbJED8I3aGqqtk_ssAbg), не ведутся ли работы
3. Если в description про отставание и нет записей про работы в logbroker, и в логах выше все ок,
4. Посмотреть в console состояние clickhouse ([preprod](https://console-preprod.cloud.yandex.ru/folders/aoetgn18qrahnvaldbc3/managed-clickhouse/cluster/e4ue6f2st2vuh6qrost9?section=monitoring) [prod](https://console.cloud.yandex.ru/folders/b1gcc8kvpne1dl3f8i0q/managed-clickhouse/cluster/c9qhej15oj834d30taek?section=monitoring)).
5. Если потеряли связь с zookeeper или зукипер периодически флапает, то сделать `sudo systemctl restart logshatter`
6. Если рестарт логшаттера не помог, то сделать `sudo systemctl restart zookeeper`
Если пишет что не может писать в clickhouse и в мониторах mdb видно что место кончилось или подходит к концу, надо дропнуть партицию.
1. На машине logshatter'а: `sudo -i; clickhouse-client; SELECT distinct table, sum(bytes) FROM system.parts GROUP BY table` - определяем таблицы, которые занимают больше всего места
2. `SELECT partition, name, active FROM system.parts WHERE table = '<таблица занимающая много места>' order by partition desc`, дальше находим самую старую партицию, достаточно хранить данные за последнюю неделю
3. Дропаем `alter table <таблица из логов, где место кончилось> drop partition '2019-07-10'`
