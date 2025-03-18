#
#
#

* решается задача обработки bgp сигнализации доступного набора маршрутов
  и формирования соответствующих таблиц маршрутизации, туннелей и правил
  обработки пакетов: ip route, ip link, ip rule. Предполагаем что в сигнализации
  в свойствах префиксов есть нужные для обработки значения атрибутов: linkid
  fwmark, community, gateway (next hop) и прочее. Внешние соответствия получаются
  через rt (уточнить для чего это надо).

  [1] [https://st.yandex-team.ru/TRAFFIC-11002]

* решается для двух сценариев (1) транизиты (2) туннели для раздачи холодного
  трафика.

* процесс A

         # watcher
         (1) bgp session handling
         (2) current bgp messages watching and processing
         (3) full state holding
         (4) pushing configuration updates into B
                  (4.1) partial modifications as bgp messages arrive
                  (4.2) periodical/event full state modification

* процесс B

         # configurator
         (1) polling full state from process A
         (2) reading current ip objects state 
         (3) syncronizing (A:4.1) or (A:4.2) modifications with (B:2)
                  (full or partitional modifications)
         (4) wating for pushing updates

* два режима запуска: (1) systemd (2) stand-alone process
* два режима мониоринга: (1) juggler (2) solomon
* для rtc контейнеров неясно что нужно ещё, точно нужен logrotate

Вопросы:

* нужно ли разносить процессы A и B отдельно на разные ноды? это даст
  возможность "отучить" часть A отдельно от контейнеров. В контейнере
  останется только B.

* что делать когда RR недоступен bgp сессии не установлены, варианты: (1) приводить
  систему в zero configuration state: сносить все туннели, роуты и проч (2) все оставлять
  как есть, вполне возможно что RR например убился.

* конфигурация bgp пиров, see cmd/config/config.go

         bgp:
           # m3 daemon establishes bgp session with a list
           # of peer defined below, there are also some 
           # specific settings to control bgp processes

           # local and peer as, local router-id
           as: 13238
           peer-as: 13238
           router-id: "5.45.193.133"

           # a list of peers, assuming that remote port
           # is the same for all of them
           #peers: [ "87.250.233.160", "141.8.136.223" ]
           peers: [ "2a02:6b8:0:202b::252" ]

           # remote and local ports for bgp sessions
           remote-port: 179

           # local-address could be empty
           #local-address: "2a02:6b8:b010:a4fc::120"

           # local-port, bgp itself starts, could
           #  be set to -1?
           local-port: -1

           # passive or active session, for mpls
           # transit we have here "false"
           passive: false

           # m3 process start a number of routines, one
           # is bgp session handling if enabled, if not
           # it could be started from command line
           enabled: true

* конфигурация режима работы m3:

         network:
           # options to set network configurations, e.g. mode
           # container/host. "container" means that m3 runs on
           # dom0 host and controls containers inside dom0
           mode: "host"

           # two possible classes to manage: "transits" and
           # "tunnels"
           #
           # (a) "transits" manages rules and tables with some 
           # fallback defaults in tables (if mpls lables and
           # routes are deleted from configuration)

           # (b) "tunnels" establishes rules, tables and links
           # as tunnles from local address to next-hops. m3
           # has internal auto-configuration and for regions
           # dom0 nodes **forcefully** set class to "transits"
           class: "tunnels"

           # options for container mode control if mode
           # is set to "contaner" m3 filters a type of
           # container to control
           container:
             # type of containers to control over m3
             types: [ "proxy", "strm" ]

* конфигурация tunnles:

           # options for "tunnels" class configuration
           tunnels:
             # ifname/device prefix for tunnels links
             # object devices
             device-prefix: "strm"

             # table proto filtering for "routes", in
             # tunnels mode historically, no any specific
             # proto was established
             proto: "all"

             # local-address fot tunnels, is it possible
             # to detect it automatically?
             local-address: "2a02:6b8:b010:a4fc::120"
             #local-address: "2a02:6b8:c20:1ce:0:584:d6c0:0"

             # encap-dport sets a remote encap
             # destination port
             encap-dport: 6635

             # route table multiplication factor
             route-table-multiplier: 1000

* конфигурация transits:

             # options from "transits" class
             transits:

               # transits have more sophisticated method
               # it established tables with predefined, see
               # [1] /etc/iproute2/rt_protos
               proto: "xorp"

               # route table multiplication factor
               route-table-multiplier: 1

               # transits should generate a route table
               # with specfied gateway in destination
               # network for ip4 and ip6

               gateway-ip4: "37.9.93.158"
               gateway-ip6: "2a02:6b8:0:70c::1"

               # encap mpls routes should have a metric of '100'
               # while all the rest in specific table '1000'
               metric-encap: 100
               metric-default: 1000

               # mtu and advmss for default routes
               mtu-default-ip6: 1450
               mtu-default-ip4: 1450

               advmss-default-ip6: 1390
               advmss-default-ip4: 1410

               # specific routes inside Yandex networks
               # should have jumbo mtu set on
               mtu-jumbo-ip6: 8900
               mtu-jumbo-ip4: 8900

               advmss-jumbo-ip6: 8840
               advmss-jumbo-ip4: 8860

               # specific overrides
               overrides:
                 "kiv":
                    gateway-ip4: "37.9.93.158"
                    gateway-ip6: "2a02:6b8:0:70c::1"

                 "rad":
                    gateway-ip4: "37.9.93.190"
                    gateway-ip6: "2a02:6b8:0:70d::1"

* command line interface:

             # стартуем bgp сессию с пиром из конфигурации
             # или указанным в параметре peer и мониторим
             # сообщения, команда используется для дебага
             # нужно убедится что m3 сервис на этом хосте не
             # запущен (так как они переиспользуют одну конфигурацию
             # по умолчанию)
             m3 mgmt bgp start --debug --timeout=30

             # посмотреть что находится в мониторинге, 
             # получаем rib и смотрим всё ли ок (параметры
             # и конфигурация мониторинга задаются в конфиге)
             m3 status --json --debug
             m3 monitor run m3-mpls-bgp --debug

             # запуск "вручную" синхронизации one shot:
             # ждёт bgp сессии и формирует изменения в network
             m3 mgmt sync --debug  --timeout=20 [--dry-run] 
 
             # flush всех network конфигураций 
             m3 mgmt flush --debug  [--dry-run] [--log=stdout]

             # смотрим статус mpls network objects, то есть
             # все ли нужные links, routes и rules есть
             # в ip links/routes/rules (так же как сделано 
             # для bgp/paths.
             m3 monitor run m3-mpls-objects --debug

             # отгружать данные в solomon (метрики из monitoring)
             # events/actions количество actions сгенерированных [1],
             # [2], [3]

             # нужно сделать server side sync + время на синхронизацию
             # bgp, то есть не синхронизируем есть time bgp < 60?
             # вызывается либо периодически либо по event (когда меняются
             # роуты, как сделать fifo, через очередь channels?

             # systemd m3 service file (standart mode)

             # RTC mode: пока не очень ясно как коллеги будут 
             # деплоить, мониторить и управлять этими штуками, 
             # приведу все команды которые сейчас есть
 
             # RTC: deploy - binary and config
             [~] ls -l /usr/bin/m3
             -rwxr-xr-x 1 root root 30460144 Nov 15 13:36 /usr/bin/m3

             [~] ls -l /etc/m3/m3.yaml
             -rw-r--r-- 1 root root 5186 Nov 15 13:40 /etc/m3/m3.yaml

             # RTC: starting m3
             [~] m3 server start --detach --debug

             # RTC: stopping m3
             [~] m3 server stop --detach --debug --log=stdout

             # RTC logging, очень много логов на первом этапе 
             # пока собираем первые результаты, в теории можно запустить через
             # rsyslog - но я не вижу чтобы в контейнере был rsyslog запущенный
             [~] cat /etc/m3/m3.yaml  | grep "log:"
             log: "/var/log/m3/m3.log"

             # посмотреть версию и может быть какой-то еще 
             # дополнительной информации 
             [~] m3 version --log=stdout
             INFO[2020/11/15 - 13:54:37.509] [7876]:[1] m3: '3.1-dev', build date: '2020-11-15T11:45:59+0300',
                  compiler: 'gc' 'go1.14.4', running at 'strm-cold-cache-test-3.vla.yp-c.yandex.net' 

             # посмотреть конфигурацию (по идее это /etc/m3/m3.yaml плюс
             # некоторые runtime параметры)
             [~] m3 config show  --log=stdout
             runtime:
               version: 3.1-dev
               date: 2020-11-15T11:45:59+0300
               ....

             # RTC: сротировать log, делает простой mv logfile -> logfile.old
             # также ротация работает и периодически в "00:01"
             [~] m3 server logrotate --debug

* backlog

             # network: для транзитов есть networks-default которые идут мимо mpls
             #      как это делать для туннелей и надо ли

* графички для сигналов: actions, sessions, network objects

  [1] [https://solomon.yandex-team.ru/?project=cdn&cluster=cdn-hx&service=hx]

