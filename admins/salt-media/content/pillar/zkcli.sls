zk_client:
  lookup:
    configs:
      /etc/distributed-flock.json:
        host:
          {% if grains['yandex-environment'] in ['production','prestable'] %}
          - zuke01j.content.yandex.net:2181
          - zuke01f.content.yandex.net:2181
          - zuke01h.content.yandex.net:2181
          {% elif grains['yandex-environment'] in ['testing', 'stage'] %}
          - zuke01f.tst.content.yandex.net:2181
          - zuke01i.tst.content.yandex.net:2181
          - zuke01h.tst.content.yandex.net:2181
          {% else %}
          - zuke01h.tst.content.yandex.net:2181
          - zuke01i.tst.content.yandex.net:2181
          - zuke01f.tst.content.yandex.net:2181
          {% endif %}
