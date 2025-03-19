# lb-node-unknown-rips

**Значение**: на lb-node приехало правило, для которого она на знает, как анонсировать (или как снимать анонс) rip'а (специфик или subnet). Эта проверка, фактически, подможество https://wiki.yandex-team.ru/cloud/devel/loadbalancing/monitorings/#lb-node-announces , но берёт инфу из Solomon, а не с хоста.
**Воздействие**: через эту lb-node не будет ходить healthcheck трафик для данного rip'а или его subnet'а.
**Что делать**: на lb-node можно узнать, какой rip проблемный:
`curl 0x0:4050/debug/gobgp`
Дальше будут два списка: not announced rips - rip'ы, для которые не знаем, как анонсировать и not withdrawn - rip'ы, которые не знаем, как удалять. Список известных сетей можно посмотреть в секции gobgp.rip-announces, gobgp.rip-defaults:
`sudo cat /etc/yc/loadbalancer-node/config.yaml`
если сети rip'а действительно нет, то нужно понять, откуда он появился. Возможно, дёрнуть duty tool info по rip'у и найти логи в API, откуда он взялся. Возможно, найти хвосты поможет /duty vpc-api. Если это честный IPv6 rip, то, возможно не хватает IPv6 сети в конфиге lb-node, тут может помочь /duty netinfra.
