{% set fqdn = salt['pillar.get']('data:dbaas:fqdn', salt['grains.get']('fqdn', 'unknown')) %}
{% set my_dc = salt['grains.get']('ya:short_dc', fqdn[:3]) %}
{% set yandex_env = salt['pillar.get']('yandex:environment', 'dev') %}
{# This condition left here for future version deployments #}

include:
    - .common
    - .odyssey-pkg

/etc/default/odyssey:
    file.managed:
        - source: salt://{{ slspath }}/conf/odyssey-default-ubuntu

{# This kostyl is required for a non-fast operation MDB-7864#}
odyssey-reload2:
    cmd.run:
        - name: service odyssey reload
        - onchanges:
            - file: /etc/odyssey/ssl/server.key
            - file: /etc/odyssey/ssl/server.crt
            - file: /etc/odyssey/ssl/allCAs.pem
            - file: /etc/default/odyssey
        - onlyif:
            - service odyssey status

extend:
    odyssey-reload:
        cmd.run:
            - require:
                - service: odyssey

/lib/systemd/system/odyssey.service:
    file.managed:
        - source: salt://{{ slspath }}/conf/odyssey.service
        - template: jinja
        - onchanges_in:
            - module: systemd-reload
        - require_in:
            - service: odyssey

/lib/systemd/system/pgbouncer.service:
    file.absent:
        - onchanges_in:
            - module: systemd-reload

{% set connection_pooler = salt['pillar.get']('data:connection_pooler', 'odyssey') %}
{% if connection_pooler == 'odyssey' %}
/etc/logrotate.d/odyssey:
    file.managed:
        - source: salt://{{ slspath }}/conf/odyssey.logrotate
        - mode: 644
        - user: root
        - group: root
        - template: jinja
{% endif %}
