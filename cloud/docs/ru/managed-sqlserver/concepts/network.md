# Сеть и кластеры БД

{% if audience != "internal" %}

При создании кластера вы можете:

* задать сеть для самого кластера;

* задать подсети для каждого из хостов кластера;

* запросить публичный IP-адрес для доступа к кластеру извне {{ yandex-cloud }}.

Вы можете создать кластер, не задавая подсети для хостов, если выбранная для каждого хоста зона доступности содержит ровно одну подсеть сети кластера.

{% else %}

Все кластеры создаются внутри сетевого макроса `_MSAASINTERNALNETS_`. Чтобы подключаться к созданной базе данных, запросите доступ в [Панчере](https://puncher.yandex-team.ru/). Для подключения к {{ MS }} в заявке нужно указать порт 3306.

{% endif %}

## Имя хоста и FQDN {#hostname}

Имя для каждого хоста в кластере {{ mms-short-name }} генерирует при его создании. Это имя будет являться доменным именем хоста (FQDN). Имя хоста и, соответственно, FQDN невозможно изменить.

{% if audience != "internal" %}

FQDN можно использовать для доступа к хосту в рамках одной облачной сети. Подробнее читайте в [документации сервиса {{ vpc-full-name }}](../../vpc/).

## Публичный доступ к хосту {#public-access-to-a-host}

Любой хост в кластере может быть доступен извне {{ yandex-cloud }}, если вы запросили публичный доступ при [создании кластера](../operations/cluster-create.md#create-cluster) или [изменении хоста](../operations/hosts.md#update). Чтобы подключиться к такому хосту, используйте его FQDN.

При удалении хоста с публичным FQDN соответствующий этому имени IP-адрес отзывается.

{% endif %}

## Группы безопасности {#security-groups}

{% include [sg-rules-limits](../../_includes/mdb/sg-rules-limits.md) %}

{% note tip %}

При подключении к кластеру из той же облачной сети, в которой он находится, не забудьте [настроить](../operations/connect.md#configure-security-groups) группы безопасности не только для кластера, но и для хоста, с которого выполняется подключение.

{% endnote %}

{% include [sg-rules-concept](../../_includes/mdb/sg-rules-concept.md) %}
