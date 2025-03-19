{% from "components/greenplum/map.jinja" import gpdbvars with context %}
{% set yandex_env = salt['pillar.get']('yandex:environment', 'dev') %}

{% if salt['pillar.get']('data:autorecovery:enable', True) %}

/etc/cron.d/gp_autorecovery:
    file.managed:
        - source: salt://{{ slspath }}/conf/cron.d/gp_autorecovery
        - template: jinja
        - mode: 644

/usr/local/yandex/gp_autorecovery.py:
    file.managed:
        - source: salt://{{ slspath }}/conf/gp_autorecovery.py
        - user: {{ gpdbvars.gpadmin }}
        - group: {{ gpdbvars.gpadmin }}
        - template: jinja
        - mode: 755
        - require:
            - file: /usr/local/yandex
            - sls: components.greenplum.install_greenplum
{% else %}
/etc/cron.d/gp_autorecovery:
    file.absent

/usr/local/yandex/gp_autorecovery.py:
    file.absent

{% endif %}

{% if salt['pillar.get']('data:master_autorecovery:enable', False) %}

/etc/cron.d/gp_master_autorecovery:
    file.managed:
        - source: salt://{{ slspath }}/conf/cron.d/gp_master_autorecovery
        - template: jinja
        - mode: 644

/usr/local/yandex/gp_master_autorecovery.py:
    file.managed:
        - source: salt://{{ slspath }}/conf/gp_master_autorecovery.py
        - user: {{ gpdbvars.gpadmin }}
        - group: {{ gpdbvars.gpadmin }}
        - template: jinja
        - mode: 755
        - require:
            - file: /usr/local/yandex
            - sls: components.greenplum.install_greenplum
{% else %}
/etc/cron.d/gp_master_autorecovery:
    file.absent

/usr/local/yandex/gp_master_autorecovery.py:
    file.absent

{% endif %}
