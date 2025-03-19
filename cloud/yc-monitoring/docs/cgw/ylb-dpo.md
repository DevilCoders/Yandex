# ylb-*-dpo

**Значение**: в fib VPP не найдены записи, соответствующие балансерным правилам
* ylb-v6-ycss-dpo ylb-v4-ycss-dpo ylb-v4-private-ycss-dpo : наличие DPO для VIP для трафика `client->vip`, отсутствие недопустимо
* ylb-v4-algonat44-dpo ylb-v4-private-algonat44-dpo ylb-v6-algonat66-dpo: наличие дефолтного маршрута через algonat для трафика `rip->client`, отсутствие недопустимо
* ylb-v4-algonat46-hc-dpo ylb-v4-private-algonat46-hc-dpo: наличие маршрута до hc-node в RIP VRF, отсутствие недопустимо
* ylb-v6-unicast-hc-dpo: наличие маршрута до hc-node по IPv6, отсутствие недопустимо
* ylb-v4-public-enabled-rip-dpo ylb-v4-private-enabled-rip-dpo ylb-v6-public-enabled-rip-dpo: наличие маршрута от contrail до RIP для трафика `hc->rip` и `client->ylb->rip`, отсутствие недопустимо
* ylb-v4-public-rip-dpo ylb-v4-private-rip-dpo ylb-v6-public-rip-dpo: наличие маршрута от contrail до RIP для трафика `hc->rip` и `client->ylb->rip`, отсутствие допустимо, балансеры могут существовать без реалов, но слишком большое количество указывает на проблемы
**Воздействие**: потенциально проливаем трафик мимо ycss/algonat/контрейла
**Что делать**:
1. Убедиться, что это не флап т.к. fib dump и ylb dump могут разойтись по времени
2. Если проблема воспроизводится на отдельном хосте, посмотреть на недавние релизы/рестарты VPP и выполнить `sudo yavpp-restart`
3. Проверить наличие force enabled статусов, они могут приводить к enabled правилам без реалов
4. Если проблема глобальная/зональная, проверить, нет ли рассинхрона между контрейлом и балансерными правилами (сравнивать правила, vrf=1, rip-vrf aka vrf+1000, vip-vrf aka vrf+513000, анонсы от oct)

