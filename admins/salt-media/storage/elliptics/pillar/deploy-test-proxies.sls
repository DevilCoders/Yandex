include:
  - units.nginx-proxy
  - units.mulcagate-nginx-secrets
  - units.ssl.test-proxy
  - units.mediastorage-proxy.proxy
  - units.secure

cluster: elliptics-test-proxies

elliptics-cache:
  101:
    path: '/srv/storage/2/1'
    size: 10737418240

ignore_conductor: True
