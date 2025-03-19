# lb-node-announces

**Значение:** на lb-node приехало правило, для которого она не знает, как анонсировать (или как снимать) анонс rip'а/vip'а. Эта проверка, фактически, дубликат https://wiki.yandex-team.ru/cloud/devel/loadbalancing/monitorings/#lb-node-unknown-vips и https://wiki.yandex-team.ru/cloud/devel/loadbalancing/monitorings/#lb-node-unknown-rips , но берёт информацию с хоста, а не из Solomon.
**Воздействие:** не будем принимать пользовательский трафик, или не будут ходить healthcheck'и.
**Что делать:** на lb-node можно узнать список проблемных анонсов:
```
curl -s 0:4050/debug/gobgp
total unknown not announced vips: 0
total unknown not withdrawn vips: 0
total unknown not announced rips: 0
total unknown not withdrawn rips: 0
total rips with failed sanity check: 16
[00] failed sanity check: fc00::402:0:ac10:23
[01] failed sanity check: fc00::3e02:0:ac10:6
[02] failed sanity check: fc00::3e02:0:ac10:27
[03] failed sanity check: fc00::a102:0:ac10:67
[04] failed sanity check: fc00::8207:0:a80:3
[05] failed sanity check: fc00::8207:0:a80:16
[06] failed sanity check: fc00::3e02:0:ac10:15
[07] failed sanity check: fc00::a102:0:ac10:66
[08] failed sanity check: fc00::c502:0:c0a8:12
[09] failed sanity check: fc00::1301:0:ac10:d
[10] failed sanity check: fc00::a102:0:ac10:65
[11] failed sanity check: fc00::3c01:0:ac10:10
[12] failed sanity check: fc00::3c01:0:ac10:23
[13] failed sanity check: fc00::c502:0:c0a8:1e
[14] failed sanity check: fc00::3c01:0:ac10:e
[15] failed sanity check: fc00::3c01:0:ac10:1e
```
Тут есть списки - с unknown not announced/withdrawn vips/rips. Vip'ы - означает, что не можем зааносировать и не ходит трафик, rip'ы - означает, что не можем анонсировать в hc сеть. Failed sanity check - эти rip'ы и не будут работать, так как у них странная конфигурация (IPv4 vip/IPv6 rip или rip из диапазона hc сетей, например).
Смотрим в конфиг:
`sudo cat /etc/yc/loadbalancer-node/config.yaml`
Если не хватает vip'а в gobgp.announces, то нужно пойти к vpc-api и понять, откуда он взялся. Если это новый fip bucket - добавить его в конфиг и прокатить релиз. Если не хватает rip, то тоже с помощью vpc-api нужно разобраться, откуда он появился и нет ли ошибки в фильтре конфигурации lb-node.
