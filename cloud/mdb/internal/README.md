# Приватные библиотеки MDB

Если вы хотите вынести какую-то из библиотек за пределы `cloud/mdb/internal`, обратитесь к @sidh.

### Краткое описание
* `app` - каркас приложения (to be DEPRECATED, but no replacement yet)
* `auth` - аутентификация/авторизация во внутренних яндексовых системах
* `cli` - каркас CLI
* `compute` - клиенты к облачным сервисам
* `conductor` - клиент к кондуктору
* `config` - загрузка конфигов (to be DEPRECATED, but no replacement yet)
* `crt` - клиент к сертификатору
* `crypto` - криптографические хелперы
* `dbteststeps` - хелперы для GODOG тестов работы с БД
* `dispenser` - клиент к диспенсеру
* `encodingutil` - хелперы для преобразования типов в json/yaml/etc
* `fs` - работа с файловой системой
* `godogutil` - хелперы для GODOG
* `grpcutil` - хелперы для gRPC
* `httputil` - хелперы для HTTP
* `leaderelection` - выбор лидера с периодической проверкой смены, реализации на redis, в планах на kubernetes.
* `logbroker` - всякое разное для работы с logbroker'ом
* `metadb` - клиент к metadb (to be DEPRECATED, but no replacement yet)
* `monrun` - работа с monrun
* `nacl` - поддержка NACl
* `optional` - опциональные значения
* `portoutil` - хелперы для работы с porto контейнерами
* `pretty` - красивое форматирование данных
* `prometheus` - работа с Prometheus
* `ready` - проверка работоспособности чего-либо
* `reflectutil` - всякое разное на рефлексии. Хорошо подумайте прежде чем использовать код внутри.
* `requestid` - работа с requestid
* `retry` - всё что нужно для организации ретраев чего-либо
* `saltapi` - клиент к salt-api of SaltStack
* `secret` - типа для секретных данных, которые не должны попадать в логи и другие подобные места
* `semerr` - ошибки несущие явную семантику (например, invalid argument или unimplemented)
* `sqlutil` - хелперы для работы с SQl-like базами
* `stringsutil` - хелперы для работы со строками
* `supervisor` - клиент к supervisorctl
* `testutil` - хелперы для тестов
* `tilde` - обработка `~`
* `valid` - комбинированые валидаторы значений

##### TODO
Структурировать пакеты, сейчас всё свалено в кучу.
