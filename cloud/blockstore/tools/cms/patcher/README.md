# Утилита для применения похостовых патчей конфигов

## Приведение похостовых конфигов к актуальному состоянию

`./blockstore-patcher -c prod/myt/cluster_config.json -vv -p prod/myt/patch.json -m 'sync patch'`

## Если набор хостов с индивидуальными конфигами не менялся

`
./blockstore-patcher -c prod/myt/cluster_config.json -vv -p prod/myt/patch.json --only-patched-hosts -m 'sync patch'
`

С `--only-patched-hosts` обновлены будут только конфиги хостов из patch.json - это существенно быстрее, чем прогон для всего кластера
