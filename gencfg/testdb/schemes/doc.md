== group.yaml:name ==
Имя группы. Является уникальным идентификатором, то есть в рамках gencfg не может быть двух групп с одим и тем же именем. Кроме генерилки, имя используется в следующих местах:
 * в [Няне](https://wiki.yandex-team.ru/jandekspoisk/sepe/nanny/) и bsconfig-е для создания сервисов;
 * в skynet-е в калькуляторе блинова в виде "sky list G@MSK\_RESERVED";
 * в REST api генерилки для получения информации о группе;

Разрешенные символы в имени группы: [A-Z\_] . Желательно группу называть в следующем формате: {LOCATION}\_{PRJNAME}\_{ITYPENAME}, где
 * **LOCATION** - название локации (MSK/MAN/SAS);
 * **PRJNAME** - желательно, чтобы совпадало с именем проекта (или одного из проектов, если таковых у группы несколько);
 * **ITYPENAME** - instance type для группы;
== group.yaml:description ==
Описание группы. Оно нужно чтобы понять, зачем вообще эта группа нужна и как будет использоваться. Также, если группа была выделена по запросу в [startek](https://st.yandex-team.ru/), сюда нужно добавить имя таска.
== group.yaml:owners ==
Список владельцев группы. В качестве владельца может выступать либо логин сотрудника, либо имя группы на staff, например [yandex_mnt_sa_runtime_expertise](https://staff.yandex-team.ru/departments/yandex_mnt_sa_runtime_expertise/). Администраторы обладают следующими правами:
 * права на ssh/sudo на этой машине (разъезжаются через cauth после создания тага);
 * права в [golem](https://golem.yandex-team.ru/) на [выдачу прав](https://golem.yandex-team.ru/cauth/request_access.sbml) на машины этой группы (разъезжаются через cauth при создании тага);
 * права на модификацию/удаление группы в web-интерфейсе;

Также к администраторам будут обращаться с вопросами вроде "почему-то инстансы вашей группы ничего не делают", "ваши инстансы мешают другим сервисам", ... .

== group.yaml:managers ==
**POSSIBLY UNUSED**
Список менеджеров, имеющих отношение к этой группе. В настоящее время - это чисто информационное поле и не понятно, нужно ли оно вообще.
== group.yaml:developers ==
**POSSIBLY UNUSED**
Список разработчиков, имеющих отношение к этой группе. В настоящее время - это чисто информационное поле и не понятно, нужно ли оно вообще.
== group.yaml:watchers ==
Каждый раз при создании нового тага в gencfg вычисляется список изменений в группе по сравнению с предыдущим. Этот список изменений может включать:
 * изменения в карточке группы, например itype/ctype/...;
 * изменения в списке хостов (добавление/удаление хостов);
 * ...

Все изменения отправляются письмом владельцам (owners) группы. Эти изменения также посылаются людям из списка (watchers). Таким образом, watchers - это не администраторы, которые хотят быть в курсе изменений.

== group.yaml:access.sshers ==
**DOES NOT WORK**
Список логинов пользователей, у которых должен быть ssh-доступ на машину. В настоящее время не работает, поскольку не можем пробросить это в cauth.
List of users with ssh access granted
== group.yaml:access.sudoers ==
**DOES NOT WORK**
Список логинов пользователей, у которых должен быть парольный sudo на **все команды** на машинке (а также ssh-доступ на нее). В настоящее время не работает, поскольку не можем пробросить это в cauth.
== group.yaml:access.sudo_commands._list.users ==
**DOES NOT WORK**
== group.yaml:access.sudo_commands._list.command ==
**DOES NOT WORK**
== group.yaml:access.sudo_commands._list ==
**DOES NOT WORK**
Список пар (<**USER LIST**>, <**COMMAND**>) для людей, которым нужны определенные sudo-команды на машинах.
== group.yaml:access.sudo_commands ==
** DOES NOT WORK **
Список пар (<**USER LIST**>, <**COMMAND**>) для людей, которым нужны определенные sudo-команды на машинах.
== group.yaml:access ==
**DOES NOT WORK**
Дополнительные права, которые должны из gencfg протаскиваться на все машины группы через cauth. В настоящее время не работает, поскольку не можем пробросить это в cauth.
== group.yaml:host_donor ==
Эта опция применима только для slave-групп. Список машин в slave-группе может определятся одним из двух вариантов:
 * собственно список хостов (подмножество хостов master-группы). В этом случае **host_donor** не устанавливается.
 * такие же хосты, как и у **host_donor** группы. При этом **host_donor** группа должна быть либо master-группой, либо одним из ее slave-ом. Эта функциональность нужна для того, чтобы списки хостов связанных slave-групп (например, int-ов и базовых, которые расположены на одних машинах) менялись одновременно.
== group.yaml:intlookups ==
**READONLY**
Список intlookup-ов группы. Точнее говоря. список intlookup-ов, в которых есть инстансы данной группы. Это поле вычисляется автоматически и не может быть изменено. Оно может поменяться при операциях с intlookup-ами (создание/удаление intlookup-а с инстансами данной группы).
== group.yaml:on_update_trigger ==
**READONLY**
Триггер (python-функция), которая выполняется каждый раз при обновлении информации о группах. Такой триггер ставится только у специальных групп, содержимое которых вычисляется автоматически по содержимому других групп.

Например, группа [ALL_SEARCH](https://gencfg.yandex-team.ru/trunk/groups/ALL_SEARCH) содержит в себе хосты тех групп, у которых флаг **properties.nonsearch== False** . Соответсвенно, при изменении этого флага у любой группы, либо при изменении списка хостов группы с **properties.nonsearch == False** , содержимое группы [ALL_SEARCH](https://gencfg.yandex-team.ru/trunk/groups/ALL_SEARCH) нужно пересчитать.
== group.yaml:searcherlookup_postactions.custom_tier.enabled ==
Флаг, означающий, что для данной группы прописан какой-то **custom_tier**.
== group.yaml:searcherlookup_postactions.custom_tier.tier_name ==
Имя **одношардового** tier-а, который привязывается к группе.
Tier name (one of one-shard tiers)
== group.yaml:searcherlookup_postactions.custom_tier ==
Обычно tier-ы привязываются к группе с помощью intlookup-ов, в которых описано соответсвие инстансов и шардов. Однако для одношардовых tier-ов это не обязательно и эту связь можно описать, у казав **custom_tier** , который будет навешиваться на все инстансы группы.
== group.yaml:searcherlookup_postactions.shardid_tag.enabled ==
Флаг, означающий, что для данной группы мы добавляем таги с shardid.

== group.yaml:searcherlookup_postactions.shardid_tag.tag_prefix ==
**DEPRECATED**
Самый простой способ задать формат тага **shardid**: текстовая строка, которая является префиксом. Например, если задан префикс **OPT_shardid=** , то для инстансов с шардами 0,1,...,100 будут добавлены соотвественно таги OPT_shardid=0,OPT_shardid=1,...,OPT_shardid=100

== group.yaml:searcherlookup_postactions.shardid_tag.tag_format ==
Вариант задавать формат тага в виде строки-форматтера для python-а. При этом в качестве возможных аргументов для форматтера выступают:
 * **shard_id** - номер шарда (0, ..., <кол-во шардов>;
 * **slookup_shard_name** - имя шарда, которое протаскивается в searcherlookup.conf (определяется тем, что написано в описании tier-а);
Например, задав строку для 'OPT_shardid=%(shard_id)04d-%(slookup_shard_name)s' для (PlatinumTier0)[https://gencfg.yandex-team.ru/trunk/tiers/PlatinumTier0] (мы получим имена шардов OPT_shardid=0000-lp038-019-0000000000,OPT_shardid=0001-lp039-014-0000000000,...).

== group.yaml:searcherlookup_postactions.shardid_tag.tags_format ==
Второй способ задавать формат тага в виде форматтера для python. Позволяет задать список **shard_id** тагов (предыдущий способ дает возможность задать только один такой таг)/

== group.yaml:searcherlookup_postactions.shardid_tag.write_primus_name ==
**DEPRECATED**

== group.yaml:searcherlookup_postactions.shardid_tag ==
Обычно, когда мы генерим таги для инстансов, мы не указываем инфомрацию о том, какой шард обслуживает этот инстанс. Эта информация имеется только в секциях **ilookup/slookup** в searcherlookup.conf-е. Однако эта информация зачастую необходима на самом инстансе, например для **web refresh**, чтобы инстанс знал, какой кусок базы ему выкачивать.

Поскольку разным программам нужен разный формат для тага **shardid**, у нас есть несколько способов задать его, описываемых параметрами, относящимися к этой секции.

== group.yaml:searcherlookup_postactions.host_memory_tag.enabled ==
Флаг, означающий, что для данной группы мы добавляем таги с размером памяти на машине. 

== group.yaml:searcherlookup_postactions.host_memory_tag ==
Иногда параметры запуска инстанса могут зависеть от объема памяти на машине (например размер кэша в памяти может как-то зависеть объема памяти машины). Для ситуаций, когда людям лень прямо на машине вычислять объем ее памяти, мы сделали возможность протащить таг **ENV_HOST_MEMORY_SIZE=<host memory size>** .

== group.yaml:searcherlookup_postactions.gen_by_code_tags._list.code ==
Код для лямбда-функции, который приеняется к каждому инстанс. Например, добавить таг с размером диска хоста можно следующией фукнцией **lambda x: 'itag_disk_size_%s' % x.host.disk**.

== group.yaml:searcherlookup_postactions.gen_by_code_tags._list ==
Python lambda-func

== group.yaml:searcherlookup_postactions.gen_by_code_tags ==
Иногда пользователи хотят добавить таги, которые генерятся по уникальной логике (которая не применима нигде кроме одной-двух групп). Обычно эта логика может быть описана в виде функции, принимающей на вход инстанс. Собственно в текущей секции пользователь задает функцию, которая по инстансу будет генерировать таг, который будет установлен на этот инстанс.

== group.yaml:searcherlookup_postactions.copy_on_ssd_tag.enabled ==
Флаг, означающий, что для данной группы мы добавляем тэг необходимости копирования с **HDD** на **SSD**.

== group.yaml:searcherlookup_postactions.copy_on_ssd_tag ==
Поисковые машины зачастую имеют два дисковых раздела: **/db** - раздел из HDD-дисков, и **/ssd** - раздел из SSD-дисков. При этом база копируется на HDD-диски, и поисковая программа при старте использует эту базу. Такое решение обладает недостатками в ситуациях, когда поисковой программе нужно время от времени читать что-то с диска. При этом положить все на SSD не представляется возможным, поскольку все не влезет.

Мы решаем эту проблему следующим образом: мы копируем часть файлов (те, которые поисковая программа будет реально читать в процессе работы) на **SSD** . А понимаем, нужно ли копировать, по установленному тагу **itag_copy_on_ssd** . Этот таг устанавливается только на те машины, на которых есть и ssd, и hdd.

== group.yaml:searcherlookup_postactions.pre_in_pre_tags._list.seed ==
Строка - **random seed** для вычисления списка машин, на которые ставится таг. Если поменять **random seed** список машин заменится на псевдослучайный.

== group.yaml:searcherlookup_postactions.pre_in_pre_tags._list.tags ==
Список тагов, которые нужно установить.

== group.yaml:searcherlookup_postactions.pre_in_pre_tags._list.count ==
Количество машин, на которые должны быть установлены таги.

== group.yaml:searcherlookup_postactions.pre_in_pre_tags._list.filter ==
Произвольный фильтр (лямбда-функция для python), который ограничивает список машин, на которые может быть установлен таг. 

== group.yaml:searcherlookup_postactions.pre_in_pre_tags._list.exclude ==
Не устанавливать таги на те машины, на которых уже установлены заданные в **exclude** таги.

== group.yaml:searcherlookup_postactions.pre_in_pre_tags._list.intersect ==
Установить таги только на те машины, на которых уже установлены таги, заданные в **intersect** .

== group.yaml:searcherlookup_postactions.pre_in_pre_tags._list.affect_slave_groups ==
Установить таг также на инстансы всех slave-групп данной группы (на тех же хостах).

== group.yaml:searcherlookup_postactions.pre_in_pre_tags._list ==
Tag description with options

== group.yaml:searcherlookup_postactions.pre_in_pre_tags ==
Иногда для тестовых целей необходимо установить таг только не некоторую часть инстансов. Например, если мы тестируем новое ядро, то логично его раскатать сначала только на небольшую часть машин. Для этих целей мы сделали функцию, которая устанавливает заданный таг на заданное количество машин. Таг, устанавливаемы таким способом, обладает следующими свойствами:
 * если у группы больше одного инстанса на машину, он будет установлен либо на все инстансы машины, либо ни на одну;
 * при небольших изменениях списка машин группы список машин, на которых установлен таг, будет менять также очень мало;

Параметры, относящиеся к этой секции, описаны ниже.
List of tags, set on part of instances (stable to host add/removal)

== group.yaml:searcherlookup_postactions.replica_tags.enabled ==
Флаг, означающий, что для данной группы мы добавляем таги с номером реплики.

== group.yaml:searcherlookup_postactions.replica_tags.first_replica_tag ==
Иногда люди хотят, чтобы имя первой реплики было не **itag_replica_0**, а каким-то произвольным. Параметр **first_replica_tag** позволяет задать это имя.

== group.yaml:searcherlookup_postactions.replica_tags ==
Когда группа связана с каким-то tier-ом, каждому шарду соответствует какое-то количество инстансов (реплик). Для некоторых людей важно, чтобы они были упорядочены или как минимум важно выбрать какую-нибудь из реплик нулевой. Включение этой секции устанавливает на инстансы таги itag_replica_0, itag_replica_1, ... (для каждого шарда itag_replica_0 ставится ровно на один инстанс, то же самое про остальные реплики).

Включить эту секцию можно только для групп, которые связаны с каким-то tier-ом.

== group.yaml:searcherlookup_postactions.int_with_snippet_reqs_tag.enabled ==
**DEPRECATED**

== group.yaml:searcherlookup_postactions.int_with_snippet_reqs_tag ==
**DEPRECATED**

== group.yaml:searcherlookup_postactions.fixed_hosts_tags._list.tag ==
Имя тага, который устанавливается на указанные хосты.

== group.yaml:searcherlookup_postactions.fixed_hosts_tags._list.hosts ==
Список хостов, на которые нужно установить таг.

== group.yaml:searcherlookup_postactions.fixed_hosts_tags._list ==
Tags, set on fixed host list

== group.yaml:searcherlookup_postactions.fixed_hosts_tags ==
Иногда люди хотят установить какой-то таг на определенные хосты (хосты с определенными именами). Это не очень хорошая практика, поскольку машинки в группе могут легко поменяться, но тем не менее возможность сделать это есть.

Ниже описаны параметры для этой секции.

== group.yaml:searcherlookup_postactions.aline_tag.enabled ==
**DEPRECATED**
== group.yaml:searcherlookup_postactions.aline_tag ==
**DEPRECATED**
Добавить для всех инстансов таг **a_line_<имя датацентра>** .

== group.yaml:searcherlookup_postactions.conditional_tags._list.tags ==
Tag names
== group.yaml:searcherlookup_postactions.conditional_tags._list.filter ==
Filter on group instances
== group.yaml:searcherlookup_postactions.conditional_tags._list ==
Tags and filter func
== group.yaml:searcherlookup_postactions.conditional_tags ==
List of tags, added to group instance on certain condition
== group.yaml:searcherlookup_postactions.memory_limit_tags.enabled ==
add tags a_topology_cgset-memory.low_limit_in_bytes=... a_topology_cgset-memory.limit_in_bytes= and cgset_memory_recharge_on_pgfault_ to all instances
== group.yaml:searcherlookup_postactions.memory_limit_tags.low_limit_perc ==
Low limit in percents from 100
== group.yaml:searcherlookup_postactions.memory_limit_tags.upper_limit_perc ==
Upper limit in percents from 100
== group.yaml:searcherlookup_postactions.memory_limit_tags ==
Add tags for porto
== group.yaml:searcherlookup_postactions.snippet_ssd_instance_tag.enabled ==
Add tag snippet_ssd_instance_tag to snippet instances
== group.yaml:searcherlookup_postactions.snippet_ssd_instance_tag ==
Add tag, marking all instances, processing ssd requests
== group.yaml:searcherlookup_postactions ==
Group searcherlookup postactions
== group.yaml:tags.ctype ==
Cluster Type (a_ctype)
== group.yaml:tags.itype ==
Instance Type (a_itype)
== group.yaml:tags.prj ==
Project (a_prj)
== group.yaml:tags.metaprj ==
Meta Project (a_metaprj)
== group.yaml:tags.itag ==
Custom handmade tags (itag)
== group.yaml:tags ==
Group tags
== group.yaml:reqs.instances.memory ==
Maximal Memory Usage
== group.yaml:reqs.instances.power ==
Min Power Usage
== group.yaml:reqs.instances.disk ==
Min HDD Usage
== group.yaml:reqs.instances.ssd ==
Min SSD Usage
== group.yaml:reqs.instances.netlimit ==
Net limit on outgoing traffic
== group.yaml:reqs.instances.min_per_host ==
Min # Of Instances Per Host
== group.yaml:reqs.instances.max_per_host ==
Max # Of Instances Per Host
== group.yaml:reqs.instances.port ==
First Instance Port
== group.yaml:reqs.instances.port_step ==
Every next instance will have port number of prev instance plus Port Step
== group.yaml:reqs.instances ==
Requirements for group instance
== group.yaml:reqs.hosts.max_per_switch ==
Max Hosts Per Switch
== group.yaml:reqs.hosts.max_per_queue ==
Max Hosts Per Queue
== group.yaml:reqs.hosts.have_ipv4_addr ==
Hosts must have ipv4 addr
== group.yaml:reqs.hosts.have_ipv6_addr ==
Hosts must have ipv6 addr
== group.yaml:reqs.hosts.netcard_regexp ==
Python regex on model of network card
== group.yaml:reqs.hosts.memory ==
Minimal total host memory
== group.yaml:reqs.hosts.ndisks ==
Min Number Of Physical HDDs
== group.yaml:reqs.hosts.os ==
OS
== group.yaml:reqs.hosts.cpu_models ==
CPU Models
== group.yaml:reqs.hosts.except_cpu_models ==
Except CPU Models
== group.yaml:reqs.hosts.location.location ==
Permitted hosts locations
== group.yaml:reqs.hosts.location.dc ==
Permitted hosts data centers
== group.yaml:reqs.hosts.location.queue ==
Permitted hosts queues
== group.yaml:reqs.hosts.location ==
Location requirements
== group.yaml:reqs.hosts ==
Requirements for group hosts
== group.yaml:reqs.shards.min_power ==
Min Power Per Shard
== group.yaml:reqs.shards.min_replicas ==
Min Replicas Per Shard
== group.yaml:reqs.shards.max_replicas ==
Max Replicas Per Shard
== group.yaml:reqs.shards.equal_instances_power ==
If this field is set, all instances must have same power
== group.yaml:reqs.shards ==
Shard Requirements
== group.yaml:reqs ==
Group requirements
== group.yaml:legacy.masterBasePort ==
Use Master Base Port
== group.yaml:legacy.funcs.instanceCount ==
Instance Count Func
== group.yaml:legacy.funcs.instancePower ==
Modification is not implemented at the moment
== group.yaml:legacy.funcs.instancePort ==
Instances start port
== group.yaml:legacy.funcs ==
Information for instance generations
== group.yaml:legacy ==
Manual Groups Settings
== group.yaml:properties.expires ==
Group will be removed on expiration date. You can extend expiration date by specifying new date (<Sep 7, 2015> means group will be removed on 07.09.2015)  or number of day since today (<12> means group will be removed after 12 days from now).
== group.yaml:properties.ignore_cpu_audit ==
If true this group will not take part in host CPU usage audit. This value can be changed by gencfg admins only.
== group.yaml:properties.untouchable ==
If true group hosts can not be transferred to other groups or projects, this value can be changed by gencfg admins only
== group.yaml:properties.nonsearch ==
If true group hosts are non-search (with custom nalivka, authentification, access)
== group.yaml:properties.extra_disk_size ==
Extra disk size (in case we have instances on host)
== group.yaml:properties.extra_disk_size_per_instance ==
Extra disk size for every instance
== group.yaml:properties.extra_disk_shards ==
Number of bases per instances
== group.yaml:properties.share_master_ports ==
If true this slave group share port with master (isntances on same port as in master group)
== group.yaml:properties.yasmagent_prestable_group ==
if True all group hosts added to yasmagent prestable group
== group.yaml:properties.yasmagent_production_group ==
if True all group hosts added to yasmagent production group
== group.yaml:properties.fake_group ==
If true group instances are fake instances (should be excluded from all statistics)
== group.yaml:properties.unraisable_group ==
If true group instances are not raised by bsconfig/iss/porto/...
== group.yaml:properties.background_group ==
If true group treated as auxiliary group (golovan yasmagent, etc.)
== group.yaml:properties.export_to_cauth ==
Export group owners to cauth even if group is nonsearch
== group.yaml:properties.created_from_portovm_group ==
This field is set automatically
== group.yaml:properties ==
Group Properties
== group.yaml:recluster.current_stage ==
Current stage of recluster (should be <reclusterdone> or one of steps in alloc_hosts,generate_intlookups,...)
== group.yaml:recluster.cleanup._list.id ==
Comman Id (optional, should be uniq if specified)
== group.yaml:recluster.cleanup._list.command ==
Command to execute
== group.yaml:recluster.cleanup._list.prerequisites ==
List of commands to be completed before we can run this command
== group.yaml:recluster.cleanup._list ==
Command description
== group.yaml:recluster.cleanup ==
List of commands to cleanup this group
== group.yaml:recluster.alloc_hosts._list.id ==
Comman Id (optional, should be uniq if specified)
== group.yaml:recluster.alloc_hosts._list.command ==
Command to execute
== group.yaml:recluster.alloc_hosts._list.prerequisites ==
List of commands to be completed before we can run this command
== group.yaml:recluster.alloc_hosts._list ==
Command description
== group.yaml:recluster.alloc_hosts ==
List of commands needed to allocate hosts
== group.yaml:recluster.generate_intlookups._list.id ==
Comman Id (optional, should be uniq if specified)
== group.yaml:recluster.generate_intlookups._list.command ==
Command to execute
== group.yaml:recluster.generate_intlookups._list.prerequisites ==
List of commands to be completed before we can run this command
== group.yaml:recluster.generate_intlookups._list ==
Command description
== group.yaml:recluster.generate_intlookups ==
List of commands needed to generate intlookups
== group.yaml:recluster ==
List of commands to recluster this group
== group.yaml:triggers.on_add_host.method ==
Trigger function
== group.yaml:triggers.on_add_host ==
Trigger, performed every time host added to group
== group.yaml:triggers ==
Triggers, perfomed on various actions with groups
== group.yaml:reminders._list.message ==
Reminder message
== group.yaml:reminders._list.expires ==
Reminder expiration time
== group.yaml:reminders._list ==
Reminder
== group.yaml:reminders ==
Reminders (do not forget to do something with group in future)
