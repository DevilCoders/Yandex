zk_client:
  lookup:
    configs:
      /etc/distributed-flock.json:
        host:
          {% if grains['yandex-environment'] in ['production','prestable'] %}
          - kp-zk01e.kp.yandex.net:2181
          - kp-zk01j.kp.yandex.net:2181
          - kp-zk01h.kp.yandex.net:2181
          {% elif grains['yandex-environment'] in ['testing','stress'] %}
          - kp-zk01i.tst.kp.yandex.net:2181
          - kp-zk01f.tst.kp.yandex.net:2181
          - kp-zk01h.tst.kp.yandex.net:2181
          {% endif %}
        logger:
          path: "/var/log/zk-flock.log"
          level: INFO
          zklevel: ERROR

