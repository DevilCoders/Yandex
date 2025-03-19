Утилита для вытаскивания информации о дисках на кластере. Выкладываем утилиту на какой-нибудь хост
кластера и запускаем. На выходе получаем JSON в который сложена информация о дисках.
Сейчас собирается следующая информация:
1. DiskId
2. BlocksCount
3. BlockSize
4. StorageMediaKind
5. CloudId
6. FolderId
7. MixedBlocksCount
8. MergedBlocksCount
9. FreshBlocksCount

usage: blockstore-query-stats [-h] [--monport MONPORT] [--port PORT]
                              [--path PATH] [--period PERIOD] [--log LOG]
                              [--resolve]
                              endpoint

positional arguments:
  endpoint           server endpoint

optional arguments:
  -h, --help         show this help message and exit
  --monport MONPORT  kikimr monitoring port
  --port PORT        server port
  --path PATH        scheme shard path
  --period PERIOD    query period
  --log LOG          log file
  --resolve          resolve tablet ids

Пример:
./blockstore-query-stats localhost | jq 'sort_by(.MixedBlocksCount)' | jq 'reverse'
