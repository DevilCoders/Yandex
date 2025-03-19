include:
  - nocdev-mysql


storage:
  {% if grains['host'] == 'red-srv01' %}
  zfs_device: /dev/md2
  {% endif %}

mysync_conductor_group: nocdev-mysql

remove_yandex_netconfig: False
docker:
  upgrade: False
  legacy: False
  daemon_opts:
    debug: False
    userland-proxy: False
    ipv6: True
    fixed-cidr-v6: fd00:ff55:a5ea::/48
    log-driver: journald

docker-auth:
  {%- set token = salt.yav.get('sec-01eh1xvv6c07x3ejz6y5pecz5x[noc-gitlab-registry-token]')|load_json %}
  username: {{token.key}}
  password: {{token.token}}


rttestctl:
  secrets: {{ salt.yav.get('sec-01e57g77d1q5bjwxybd3xh7d3q')|json}}
  rt-cert: {{ salt.yav.get('sec-01g546ssz4b9g3ctgn2q8x98qv[7F001D2EDE954AA0BDFF5195C50002001D2EDE_key_cert]') | json }}
  cvs-secrets: {{ salt.yav.get('sec-01en5eg5tcx7hbtbkayr70ezek')|json}}
  arc-secret: {{ salt.yav.get('sec-01f00xrpf9sfcd6fjrdrzw8tj3[ci.token]') | json }}
  cluster:
    # %nocdev-staging
    iva-rt-staging2: { cname: n5.test.racktables.yandex-team.ru, weight: 100 }
    sas-rt-staging2: { cname: n6.test.racktables.yandex-team.ru, weight: 100 }
    sas-rt-staging3: { cname: n7.test.racktables.yandex-team.ru, weight: 100 }
    vla-rt-staging1: { cname: n9.test.racktables.yandex-team.ru, weight: 100 }
    vla-rt-staging2: { cname: n10.test.racktables.yandex-team.ru, weight: 100 }

    # %nocdev-test-staging
    rt-test: { cname: n1.test.racktables.yandex-team.ru, weight: 100 } # qyp
    vla-rt-staging: { cname: n2.test.racktables.yandex-team.ru, weight: 100 } # qyp
    iva-rt-staging1: { cname: n4.test.racktables.yandex-team.ru, weight: 100 }
    sas-rt-staging1: { cname: n3.test.racktables.yandex-team.ru, weight: 100 }

    # 
    red-srv01: { cname: n99.test.racktables.yandex-team.ru, weight: 100 }
  consul:
    {% if grains['yandex-environment'] == 'development' %} 
    service_name: "pre.test.racktables.yandex-team.ru"
    {% elif grains['host'] == 'red-srv01' %}
    service_name: "red.racktables.yandex-team.ru"
    {% else %}
    service_name: "test.racktables.yandex-team.ru"
    {% endif %}

salt_cert_expires_path: '/rt-zpool/rt/secrets/cert.pem'

{% if grains['host'] == 'red-srv01' %}
recreate_env: True
{% endif %}

