{% set yaenv = grains['yandex-environment'] %}

include:
  - templates.disable_rp_filter
  - {{ slspath }}.monrun
  - templates.certificates
  - templates.youtube-dl
  - templates.ffmpeg
  - common.graphite_to_solomon

/etc/yandex/cert/partners-cert:
  file.managed:
    - user: root
    - group: root
    - mode: 0644
    - makedirs: True
    - source: salt://{{slspath}}/files/partners-cert

/etc/yandex/cert/java-all.keystore:
  file.managed:
    - user: root
    - group: root
    - mode: 0644
    - makedirs: True
    - source: salt://{{slspath}}/files/java-all.keystore

{% if yaenv == 'testing' %}
/etc/nginx/ssl/kino-backoffice.yandex-team.ru.crt:
  file.symlink:
    - target: /etc/nginx/ssl/wc.tst.kinotv.yandex.net.pem
    - require:
      - file: /etc/nginx/ssl/wc.tst.kinotv.yandex.net.pem

/etc/nginx/ssl/kino-backoffice.yandex-team.ru.key:
  file.symlink:
    - target: /etc/nginx/ssl/wc.tst.kinotv.yandex.net.pem
    - require:
      - file: /etc/nginx/ssl/wc.tst.kinotv.yandex.net.pem
{% endif %}
