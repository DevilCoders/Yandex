# drain-status

**Значение:** lb-nod'а задрэйнена, не должна обслуживать трафик хэлсчеков
**Воздействие:** не обслуживает трафик от hc-nod, если загорится во всей зоне, то перестанут работать hc-nod'ы, если в decription `lb-node drained, not processing external traffic` то сняты анонс ipv4 випов наружу
**Что делать:**
1. `no announce to hc network for` проверить что yc-loadbalancer-node в ping ручку отвечает ок `curl -v 0x0:4050/ping`
2. не горит других мониторов локально по vpp  `monrun -w`
3. Если message проверки включает в себя `lb-node drained, not processing external traffic`, то либо остался после рестарта vpp, либо кто-то специально задрэйнил ноду, **уточнить в чате** сам флаг `/tmp/node_drained`
