imdb_parsing_queue:
  file.managed:
    - name: /usr/lib/yandex-graphite-checks/enabled/imdb_parsing_queue.sh
    - source: salt://{{ slspath }}/files/imdb_parsing_queue.sh
    - makedirs: true
    - mode: 0750

films_ratings:
  file.managed:
    - name: /usr/lib/yandex-graphite-checks/enabled/films_ratings.sh
    - source: salt://{{ slspath }}/files/films_ratings.sh
    - makedirs: true
    - mode: 0750

{%- if grains['yandex-environment'] in ["production"] %}

{% set rdc = salt["grains.get"]("conductor:root_datacenter") %}
{% set host = [] %}
{% set hosts = ["iva-zmwqeu2gda36t74d.db.yandex.net", "man-p4alwhks4bo42e9n.db.yandex.net", "sas-qeawvfhou6whgqp3.db.yandex.net", "vla-w7c6c42tl9es4wys.db.yandex.net"] %}
{% for dc_host in hosts %}
{% if rdc in dc_host %}
{% set _ = host.append(dc_host) %}
{% endif %}
{% endfor %}

users_count:
  file.managed:
    - name: /usr/lib/yandex-graphite-checks/enabled/users_count.sh
    - source: salt://{{ slspath }}/files/users_count.sh
    - makedirs: true
    - mode: 0700
    - template: jinja
    - context:
        host: {{ host[0] }}

users_count_request_new_data:
  file.managed:
    - name: /etc/cron.d/users_count_request_new_data
    - contents: |
        0 * * * * root rm /tmp/users_count /tmp/users_count_kp_auth
{% endif %}

