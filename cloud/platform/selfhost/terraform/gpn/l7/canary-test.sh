#!/usr/bin/env bash

set -x

# Check that staging is alive.
curl -i --cacert ../common/GPNInternalRootCA.crt \
  https://staging.private-api.ycp.gpn.yandexcloud.net/

echo

ycp --profile=gpn-canary platform alb virtual-host create -r- <<< '
http_router_id: fke8jcoh6f86tm0slii4
name: e2e-test-vh
authority: [e2e-test.staging.private-api.ycp.gpn.yandexcloud.net]
routes:
- name: main
  http:
    route: { backend_group_id: albambqu2br9n773rqrp } # admin
'

curl -i \
  -H Host:e2e-test.staging.private-api.ycp.gpn.yandexcloud.net \
  --cacert ../common/GPNInternalRootCA.crt \
  https://staging.private-api.ycp.gpn.yandexcloud.net/ready

echo

ycp --profile=gpn-canary platform alb virtual-host delete -r- <<< '
virtual_host_name: e2e-test-vh
http_router_id: fke8jcoh6f86tm0slii4
'
