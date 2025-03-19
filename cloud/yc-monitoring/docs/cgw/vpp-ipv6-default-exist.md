# vpp-ipv6-default-exist

Работает только на балансерах до CLOUD-40139. Проверяет, что в fib установлен маршрут ::/0.
Может гореть при рестарте vpp, если маршрут ещё не получен в rib.
**Если долго после рестарта горит**
1. Проверяем наличие маршрута в rib. Пример:

```
(PREPROD)svartapetov@loadbalancer-node-sas2:~$ gobgp global rib -a ipv6-mpls | grep ::/0
*> ::/0                      [2]        37.140.141.89        200350               3d 00:15:11 [{Origin: i} {Med: 1000} {Communities: 65000:6806}]
*  ::/0                      [2]        37.140.141.89        200350               3d 00:15:08 [{Origin: i} {Med: 1000} {Communities: 65000:6806}]
*  ::/0                      [2]        37.140.141.89        200350               3d 00:15:07 [{Origin: i} {Med: 1000} {Communities: 65000:6806}]
*  ::/0                      [2]        37.140.141.89        200350               3d 00:14:59 [{Origin: i} {Med: 1000} {Communities: 65000:6806}]
*  ::/0                      [2]        37.140.141.89        200350               04:27:59   [{Origin: i} {Med: 1000} {Communities: 65000:6806}]
```

Если маршрут есть, то скорее всего, это репро CLOUD-45321, лечится рестартом vpp с дрейном.
2. Если маршрутов нет, можно проверить, что они есть в adj-in от рефлекторов. Пример:

```
(PREPROD)svartapetov@loadbalancer-node-sas2:~$ gobgp neigh 37.140.141.80 adj-in -a ipv6-mpls | grep ::/0
0   ::/0                                [2]        37.140.141.89        200350                               3d 00:18:14 [{Origin: i} {Communities: 65000:6806}]
```

Если маршруты есть в adj-in, то могут быть проблемы с community, или в политиках gobgp, или в мозгах gobgp. Если маршрута нет или есть, но у него отличается community от 65000:6806 см [ip6 в разделе reflector:import](https://bb.yandex-team.ru/projects/CLOUD/repos/salt-formula/browse/pillar/roles/cloudgate.sls) или [V6DEFROUTE здесь](https://wiki.yandex-team.ru/cloud/devel/netinfra/bgp-communities/#6500065xxmarshrutyrelevantnyekkejjsamgibridnyxoblakov), то нужно призвать дежурных netinfra, и выяснить, почему отсутствует согласованное комьюнити, В противном случае, нужно проверить политики gobgp на наличие import нужного comunity. Пример:

```
(PREPROD)svartapetov@loadbalancer-node-sas2:~$ gobgp policy comm
NAME                          COMMUNITY
api-import                    65535:65282
cgw-from-oct-bounced-allowed  65402:400
cgw-to-oct-origined-all       65402:*
cloud_border-export           65000:9004
reflector-export              65000:9004
reflector-import              65000:6804
65000:6806       <- here it is
upstream4-export              13238:35150
13238:35999
65000:35999
upstream4-import              13238:35130
13238:35132
```

Если и тут всё на месте, скорее всего, проблема в gobgp, и может помочь рестарт vpp c дрейном анонсов.
