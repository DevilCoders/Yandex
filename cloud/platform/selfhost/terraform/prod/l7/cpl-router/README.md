# Private API controlplane balancer (CPL)

L3 balancer: https://console.cloud.yandex.ru/folders/b1gvgqhc57450av0d77p/load-balancer/network-load-balancers/b7rrqpami0of73r8dfs0

Conductor: https://c.yandex-team.ru/groups/cloud_prod_l7-cpl

## Howto update server cert

1) Request the new cert here https://crt.yandex-team.ru/certificates?cr-form=1

 - CA: InternalCA
 - Extended Validation, ECC - not needed
 - TTL: 365 days
 - ABC: ycl7
 - Hosts: (add new hosts here)

Hosts:

```
    private-api.ycp.cloud.yandex.net,
    *.private-api.ycp.cloud.yandex.net,
    *.front-intprod.cloud.yandex.ru,
    cloud-backoffice.cloud.yandex.ru,
    backoffice.cloud.yandex.ru,
    gw.db.yandex-team.ru,
    *.mk8s-masters.private-api.ycp.cloud.yandex.net
```

2) Wait for a letter with the new cert.

3) Update cert files:

```
../yav2cm.sh sec-01ekaefwe4atb8ar1z3ezcy2m6 update cpl fpqd0esoafiebucbiffm
```

4) Check the cert:

    $ openssl s_client -showcerts -connect cpl-router-ig-vla1.vla.ycp.cloud.yandex.net:443 </dev/null \
      | openssl x509 -serial -dates -ext subjectAltName -nocert
