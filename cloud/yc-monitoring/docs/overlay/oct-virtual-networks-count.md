[Алерт oct-virtual-networks-count в джаглере](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Doct-virtual-networks-count)

## Что проверяет

Что количество объектов `virtual-network` в базе контрейла не достигло опасного предела.

## Если загорелось

Следует подумать о [дематериализации сетей](https://wiki.yandex-team.ru/cloud/devel/sdn/duty/dematerialization/).

Дальнейший рост кол-ва сетей может привести к:

- долгим reconfig-ам `contrail-named` и нарушению работы DNS (не так актуально последнее время)

- проблемам в контрол-плейне