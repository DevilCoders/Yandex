#!/usr/bin/env bash

set -x

# Check that CPL is alive.
curl -i --cacert ../common/GPNInternalRootCA.crt \
  https://test.private-api.ycp.gpn.yandexcloud.net/

echo

ycp --profile=gpn platform alb virtual-host create -r- <<< '
http_router_id: fke835fqmh5jeputd8dh
name: e2e-test-vh
authority: [e2e-test.private-api.ycp.gpn.yandexcloud.net]
routes:
- name: main
  http:
    route: { backend_group_id: albambqu2br9n773rqrp } # admin
'

curl -i --cacert ../common/GPNInternalRootCA.crt \
  https://e2e-test.private-api.ycp.gpn.yandexcloud.net/ready

echo

ycp --profile=gpn platform alb virtual-host delete -r- <<< '
virtual_host_name: e2e-test-vh
http_router_id: fke835fqmh5jeputd8dh
'
