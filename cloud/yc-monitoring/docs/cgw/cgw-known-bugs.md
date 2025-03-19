# cgw-known-bugs

Запускает несколько проверок, при этом делая **только один** запуск дампа `vpp` и `gobgp rib`. Если `vpp` не отвечает, то краснеет.
В рамках проверки собирается `fib-dump` и откладывается в файлик в tmpfs.
**Известные проблемы**
- CLOUD-45197 
  пока не решён тикет, после рестарта не хватает директории для дампов. Проверка горит WARN-ом. Для рекавери нужно завести недостающую директорию, либо прокатить релиз.
   a. **vpp-ipv6-default-exist**
Работает только на балансерах до CLOUD-40139. Проверяет, что в fib установлен маршрут ::/0. 
Может гореть при рестарте vpp, если маршрут ещё не получен в rib.
**Если долго после рестарта горит**
1. Проверяем наличие маршрута в rib . Пример:

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

Если и тут всё на месте, скорее всего, проблема в `gobgp`, и может помочь рестарт `vpp` c дрейном анонсов.

   b. **cgw-fibsync-...**
Бывает `cgw-fibsync-v4` и `cgw-fibsync-v6`.
**Что проверяет**
Воспроизведение CLOUD-16040, CLOUD-37017.
Может загораться при рестарте `vpp` на балансерах из-за низкой скорости прокачивания маршрутов до vpp. 
На проверку настроен авторекавери.
См. также https://wiki.yandex-team.ru/cloud/devel/cloudgate/interconnect/#iskljuchenieupstreamcicizmonitoringavsluchaepodkljuchenijaodnojjploshhadkisneskolkimivrf

   c. **unset-...-vpp**
Бывает `unset-v4-vpp` и `unset-v6-vpp`.
**Что проверяет**
Возникновение CLOUD-9782. После CLOUD-11298 не воспроизводилось.

   d. **arp-ipv4-...-vpp**
Бывает `arp-ipv4-v4-vpp` и `arp-ipv4-v6-vpp`.
Проверяет возникновение CLOUD-10441. Иногда стреляет на виртуалках, из которых не было ни одного пакета. 

   e. **faulty-default-...-vpp**
Бывает `faulty-default-v4-vpp` и `faulty-default-v6-vpp`.
**Что проверяет**
CLOUD-11152.
На самом деле проверка устарела, и загорается в случае, когда cgw получает из upstream маршрут, который мы сами подняли. Обычно сопровождается некорректной работы пользователя.
В случае загорания, нужно посмотреть на проблемный маршрут в vpp, если он есть и имеет 2 метки в стеке, значит это репро.
Для v6 /64 нужно понять, откуда взялся маршрут, как вариант, netinfra могла переслать с какой-нибудь железки.
Для cgw-dc, нужно найти пользователя по rt/привязанному тикету в конфиге в bootstrap-templates, и связаться с ними через support, @alex-limonov или архитекторов.

   f. **label16-...-vpp**
Бывает `label16-v4-vpp` и `label16-v6-vpp`.
**Что проверяет**
Повторение CLOUD-11158. Так как в контрейле увеличили начальную метку на 21 вместо 16, больше не стреляет (оставлен на память).
